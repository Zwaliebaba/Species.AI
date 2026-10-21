#include "pch.h"

#include "CmoWriter.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <string>
#include <string_view>

namespace MeshKit
{

namespace
{

constexpr std::size_t CMO_TEXTURE_SLOTS = 8;
constexpr std::array<float, 16> IDENTITY{1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F};
constexpr std::array<Surface, 3> SURFACE_ORDER{Surface::Facet, Surface::Team, Surface::Unlit};
constexpr std::string_view SOCKET_PREFIX = "socket_";
constexpr std::string_view COLLISION_MATERIAL = "collision";
constexpr std::uint32_t OPAQUE_ALPHA = 255;
constexpr std::size_t MAX_VERTICES = 65536; // the indices are 16-bit

std::string_view NameOf(Surface _surface) noexcept
{
  switch (_surface)
  {
  case Surface::Team:
    return "team";
  case Surface::Unlit:
    return "unlit";
  default:
    return "facet";
  }
}

/// Little-endian output.
class Output
{
public:
  void U8(std::uint8_t _value)
  {
    m_bytes.push_back(static_cast<std::byte>(_value));
  }

  void U16(std::uint16_t _value)
  {
    U8(static_cast<std::uint8_t>(_value & 0xFF));
    U8(static_cast<std::uint8_t>(_value >> 8));
  }

  void U32(std::uint32_t _value)
  {
    for (std::size_t i = 0; i < 4; ++i)
    {
      U8(static_cast<std::uint8_t>((_value >> (8 * i)) & 0xFF));
    }
  }

  void F32(float _value)
  {
    U32(std::bit_cast<std::uint32_t>(_value));
  }

  template <std::size_t Count> void Floats(const std::array<float, Count>& _values)
  {
    for (const float value : _values)
    {
      F32(value);
    }
  }

  // A string as the reader takes it: its length in UTF-16 code units with its NUL, then the units. The kit's names are
  // ASCII.
  void String(std::string_view _text)
  {
    U32(static_cast<std::uint32_t>(_text.size() + 1));
    for (const char c : _text)
    {
      U16(static_cast<std::uint16_t>(static_cast<unsigned char>(c)));
    }
    U16(0);
  }

  [[nodiscard]] std::vector<std::byte> Take() noexcept
  {
    return std::move(m_bytes);
  }

private:
  std::vector<std::byte> m_bytes;
};

std::array<float, 3> Rounded(const Point& _point) noexcept
{
  return {static_cast<float>(_point[0]), static_cast<float>(_point[1]), static_cast<float>(_point[2])};
}

// The facet normal of the rounded corners, made as the content build makes it (AST-076): binary64, rounded once.
std::array<float, 3> FacetNormal(const std::array<std::array<float, 3>, 3>& _corners)
{
  std::array<double, 3> e{};
  std::array<double, 3> f{};
  for (std::size_t axis = 0; axis < 3; ++axis)
  {
    e[axis] = double{_corners[1][axis]} - double{_corners[0][axis]};
    f[axis] = double{_corners[2][axis]} - double{_corners[0][axis]};
  }
  const std::array<double, 3> cross{e[1] * f[2] - e[2] * f[1], e[2] * f[0] - e[0] * f[2], e[0] * f[1] - e[1] * f[0]};
  const double length = std::sqrt(cross[0] * cross[0] + cross[1] * cross[1] + cross[2] * cross[2]);
  if (!(length > 0.0))
  {
    throw std::logic_error("a triangle loses its area when its corners are rounded to binary32");
  }
  return {static_cast<float>(cross[0] / length), static_cast<float>(cross[1] / length), static_cast<float>(cross[2] / length)};
}

std::array<float, 16> Translation(const std::array<float, 3>& _offset) noexcept
{
  std::array<float, 16> matrix = IDENTITY;
  matrix[12] = _offset[0];
  matrix[13] = _offset[1];
  matrix[14] = _offset[2];
  return matrix;
}

void WriteMaterial(Output& _out, std::string_view _name)
{
  _out.String(_name);
  _out.Floats(std::array<float, 4>{0.2F, 0.2F, 0.2F, 1.0F}); // ambient
  _out.Floats(std::array<float, 4>{1.0F, 1.0F, 1.0F, 1.0F}); // diffuse: white, as the content build requires
  _out.Floats(std::array<float, 4>{0.0F, 0.0F, 0.0F, 1.0F}); // specular
  _out.F32(1.0F);                                            // specular power
  _out.Floats(std::array<float, 4>{0.0F, 0.0F, 0.0F, 1.0F}); // emissive
  _out.Floats(IDENTITY);                                     // texture-coordinate transform
  _out.String("");                                           // pixel shader
  for (std::size_t slot = 0; slot < CMO_TEXTURE_SLOTS; ++slot)
  {
    _out.String("");
  }
}

void WriteBone(Output& _out, std::string_view _name, std::int32_t _parent, const std::array<float, 3>& _offset)
{
  _out.String(_name);
  _out.U32(static_cast<std::uint32_t>(_parent));
  _out.Floats(Translation({-_offset[0], -_offset[1], -_offset[2]})); // inverse bind pose
  _out.Floats(Translation(_offset));                                 // bind pose
  _out.Floats(Translation(_offset));                                 // local transform
}

void WriteMesh(Output& _out, const MeshModel& _model, std::size_t _index, bool _skeleton)
{
  const ModelNode& node = _model.nodes[_index];
  _out.String(node.name);

  // Materials: the surfaces the node uses, in a fixed order, or the one collision material.
  std::vector<Surface> surfaces;
  for (const Surface surface : SURFACE_ORDER)
  {
    if (std::ranges::any_of(node.triangles, [surface](const Triangle& _t) { return _t.surface == surface; }))
    {
      surfaces.push_back(surface);
    }
  }
  if (_model.collision && !node.triangles.empty())
  {
    _out.U32(1);
    WriteMaterial(_out, COLLISION_MATERIAL);
  }
  else
  {
    _out.U32(static_cast<std::uint32_t>(surfaces.size()));
    for (const Surface surface : surfaces)
    {
      WriteMaterial(_out, NameOf(surface));
    }
  }
  _out.U8(_skeleton && _index == 0 ? 1 : 0);

  // Submeshes: one a material, over the node's triangles grouped by material in their recipe order.
  std::vector<const Triangle*> ordered;
  std::vector<std::pair<std::uint32_t, std::uint32_t>> runs; // first triangle, count
  if (_model.collision)
  {
    for (const Triangle& triangle : node.triangles)
    {
      ordered.push_back(&triangle);
    }
    if (!ordered.empty())
    {
      runs.emplace_back(0, static_cast<std::uint32_t>(ordered.size()));
    }
  }
  else
  {
    for (const Surface surface : surfaces)
    {
      const auto first = static_cast<std::uint32_t>(ordered.size());
      for (const Triangle& triangle : node.triangles)
      {
        if (triangle.surface == surface)
        {
          ordered.push_back(&triangle);
        }
      }
      runs.emplace_back(first, static_cast<std::uint32_t>(ordered.size()) - first);
    }
  }
  if (ordered.size() * 3 > MAX_VERTICES)
  {
    throw std::logic_error(_model.file + ": the node " + node.name + " has more corners than 16-bit indices reach");
  }
  _out.U32(static_cast<std::uint32_t>(runs.size()));
  for (std::uint32_t r = 0; r < runs.size(); ++r)
  {
    _out.U32(r); // material
    _out.U32(0); // index buffer
    _out.U32(0); // vertex buffer
    _out.U32(runs[r].first * 3);
    _out.U32(runs[r].second);
  }

  // One index buffer and one vertex buffer, three corners a triangle, when the node has any.
  const bool geometry = !ordered.empty();
  _out.U32(geometry ? 1 : 0);
  if (geometry)
  {
    _out.U32(static_cast<std::uint32_t>(ordered.size() * 3));
    for (std::size_t i = 0; i < ordered.size() * 3; ++i)
    {
      _out.U16(static_cast<std::uint16_t>(i));
    }
  }
  std::array<float, 3> low{};
  std::array<float, 3> high{};
  bool any = false;
  _out.U32(geometry ? 1 : 0);
  if (geometry)
  {
    _out.U32(static_cast<std::uint32_t>(ordered.size() * 3));
    for (const Triangle* triangle : ordered)
    {
      const std::array<std::array<float, 3>, 3> corners{Rounded(triangle->corners[0]), Rounded(triangle->corners[1]),
                                                        Rounded(triangle->corners[2])};
      const std::array<float, 3> normal = FacetNormal(corners);
      const std::uint32_t color = std::uint32_t{triangle->color.red} | (std::uint32_t{triangle->color.green} << 8) |
                                  (std::uint32_t{triangle->color.blue} << 16) | (OPAQUE_ALPHA << 24);
      for (const std::array<float, 3>& corner : corners)
      {
        _out.Floats(corner);
        _out.Floats(normal);
        _out.Floats(std::array<float, 4>{});
        _out.U32(color);
        _out.Floats(std::array<float, 2>{});
        for (std::size_t axis = 0; axis < 3; ++axis)
        {
          low[axis] = any ? std::min(low[axis], corner[axis]) : corner[axis];
          high[axis] = any ? std::max(high[axis], corner[axis]) : corner[axis];
        }
        any = true;
      }
    }
  }
  _out.U32(0); // no skinning buffers

  // Extents: the centre of the bounds, the largest distance of a corner from it, the minimum and the maximum.
  double radius = 0.0;
  for (const Triangle* triangle : ordered)
  {
    for (const Point& corner : triangle->corners)
    {
      const std::array<float, 3> rounded = Rounded(corner);
      double squared = 0.0;
      for (std::size_t axis = 0; axis < 3; ++axis)
      {
        const double offset = double{rounded[axis]} - (double{low[axis]} + double{high[axis]}) / 2.0;
        squared += offset * offset;
      }
      radius = std::max(radius, std::sqrt(squared));
    }
  }
  for (std::size_t axis = 0; axis < 3; ++axis)
  {
    _out.F32(static_cast<float>((double{low[axis]} + double{high[axis]}) / 2.0));
  }
  _out.F32(static_cast<float>(radius));
  _out.Floats(low);
  _out.Floats(high);

  if (!(_skeleton && _index == 0))
  {
    return;
  }
  _out.U32(static_cast<std::uint32_t>(_model.nodes.size() + _model.sockets.size()));
  for (const ModelNode& bone : _model.nodes)
  {
    WriteBone(_out, bone.name, -1, {});
  }
  for (const ModelSocket& socket : _model.sockets)
  {
    const auto parent = std::ranges::find(_model.nodes, socket.node, &ModelNode::name) - _model.nodes.begin();
    WriteBone(_out, std::string(SOCKET_PREFIX) + socket.name, static_cast<std::int32_t>(parent), Rounded(socket.position));
  }
  _out.U32(0); // no animation clips
}

} // namespace

std::vector<std::byte> WriteCmo(const MeshModel& _model)
{
  if (_model.nodes.empty())
  {
    throw std::logic_error(_model.file + ": a CMO file holds at least one mesh");
  }
  const bool skeleton = _model.nodes.size() > 1 || !_model.sockets.empty();
  Output out;
  out.U32(static_cast<std::uint32_t>(_model.nodes.size()));
  for (std::size_t n = 0; n < _model.nodes.size(); ++n)
  {
    WriteMesh(out, _model, n, skeleton);
  }
  return out.Take();
}

} // namespace MeshKit
