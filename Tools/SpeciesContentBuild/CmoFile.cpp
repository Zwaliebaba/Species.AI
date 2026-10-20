#include "pch.h"

#include "CmoFile.h"

#include <bit>
#include <cmath>
#include <format>
#include <limits>
#include <set>
#include <string_view>

namespace ContentBuild
{

namespace
{

constexpr std::size_t NO_FAULT = std::numeric_limits<std::size_t>::max();
constexpr std::size_t MATERIAL_BYTES = 132; // four colours, the specular power and the UV transform
constexpr std::size_t VERTEX_BYTES = 52;
constexpr std::size_t SKINNING_VERTEX_BYTES = 32;
constexpr std::size_t SUBMESH_BYTES = 20;
constexpr std::size_t BONE_MATRICES_BYTES = 4 + 3 * 64;
constexpr std::size_t KEYFRAME_BYTES = 72;
constexpr std::size_t CLIP_HEADER_BYTES = 12;

// Where the checks after reading find what they name.
struct MeshOffsets
{
  std::size_t name = 0;
  std::vector<std::size_t> submeshes;
  std::vector<std::size_t> indexData;
  std::vector<std::size_t> bones;
  std::size_t skinning = 0;
  std::uint32_t skinningBuffers = 0;
  std::size_t clips = 0;
  std::uint32_t clipCount = 0;
};

/// Reads the file front to back. The first read that would leave the file is check 1's failure; a malformed string and
/// a non-finite float are kept, by their first offset, for checks 2 and 5.
class Parser
{
public:
  explicit Parser(std::span<const std::byte> _bytes) noexcept
    : m_bytes(_bytes)
  {
  }

  [[nodiscard]] bool Failed() const noexcept
  {
    return !m_failure.empty();
  }
  [[nodiscard]] const std::string& Failure() const noexcept
  {
    return m_failure;
  }
  [[nodiscard]] std::size_t At() const noexcept
  {
    return m_at;
  }
  [[nodiscard]] std::size_t StringFault() const noexcept
  {
    return m_stringFault;
  }
  [[nodiscard]] std::size_t FloatFault() const noexcept
  {
    return m_floatFault;
  }
  [[nodiscard]] std::size_t Remaining() const noexcept
  {
    return m_bytes.size() - m_at;
  }

  // A count, which must leave room for that many elements of at least _elementBytes each.
  std::uint32_t Count(std::size_t _elementBytes, std::string_view _what)
  {
    const std::size_t at = m_at;
    const std::uint32_t count = U32();
    if (!Failed() && std::uint64_t{count} * _elementBytes > Remaining())
    {
      Fail(at, std::format("the count of {} {} leads past the end of the file", count, _what));
      return 0;
    }
    return count;
  }

  std::uint8_t U8()
  {
    return Need(1) ? static_cast<std::uint8_t>(m_bytes[m_at++]) : 0;
  }

  std::uint32_t U32()
  {
    if (!Need(4))
    {
      return 0;
    }
    std::uint32_t value = 0;
    for (std::size_t i = 0; i < 4; ++i)
    {
      value |= static_cast<std::uint32_t>(m_bytes[m_at + i]) << (8 * i);
    }
    m_at += 4;
    return value;
  }

  std::uint16_t U16()
  {
    if (!Need(2))
    {
      return 0;
    }
    const auto value =
      static_cast<std::uint16_t>(static_cast<std::uint32_t>(m_bytes[m_at]) | (static_cast<std::uint32_t>(m_bytes[m_at + 1]) << 8));
    m_at += 2;
    return value;
  }

  float F32()
  {
    const std::size_t at = m_at;
    const float value = std::bit_cast<float>(U32());
    if (!std::isfinite(value) && m_floatFault == NO_FAULT)
    {
      m_floatFault = at;
    }
    return value;
  }

  void Floats(std::span<float> _values)
  {
    for (float& value : _values)
    {
      value = F32();
    }
  }

  // A string: its length in UTF-16 code units, its terminating NUL included, then the units.
  std::string String()
  {
    const std::size_t at = m_at;
    const std::uint32_t units = Count(2, "UTF-16 code units");
    std::u16string text;
    for (std::uint32_t i = 0; i < units && !Failed(); ++i)
    {
      text.push_back(static_cast<char16_t>(U16()));
    }
    if (Failed())
    {
      return {};
    }
    std::string utf8;
    bool wellFormed = !text.empty() && text.back() == u'\0';
    for (std::size_t i = 0; wellFormed && i + 1 < text.size(); ++i)
    {
      char32_t point = text[i];
      if (point == 0 || (point >= 0xDC00 && point <= 0xDFFF))
      {
        wellFormed = false;
        break;
      }
      if (point >= 0xD800 && point <= 0xDBFF)
      {
        if (i + 2 >= text.size() || text[i + 1] < 0xDC00 || text[i + 1] > 0xDFFF)
        {
          wellFormed = false;
          break;
        }
        point = 0x10000 + ((point - 0xD800) << 10) + (static_cast<char32_t>(text[i + 1]) - 0xDC00);
        ++i;
      }
      Append(utf8, point);
    }
    if (!wellFormed && m_stringFault == NO_FAULT)
    {
      m_stringFault = at;
    }
    return utf8;
  }

  void Fail(std::size_t _at, std::string_view _message)
  {
    if (m_failure.empty())
    {
      m_failure = std::format("byte {}: {} (ADR-019 check 1)", _at, _message);
    }
  }

private:
  bool Need(std::size_t _bytes)
  {
    if (Failed())
    {
      return false;
    }
    if (_bytes > Remaining())
    {
      Fail(m_at, std::format("the file ends where {} more bytes are read", _bytes));
      return false;
    }
    return true;
  }

  static void Append(std::string& _utf8, char32_t _point)
  {
    if (_point < 0x80)
    {
      _utf8.push_back(static_cast<char>(_point));
    }
    else if (_point < 0x800)
    {
      _utf8.push_back(static_cast<char>(0xC0 | (_point >> 6)));
      _utf8.push_back(static_cast<char>(0x80 | (_point & 0x3F)));
    }
    else if (_point < 0x10000)
    {
      _utf8.push_back(static_cast<char>(0xE0 | (_point >> 12)));
      _utf8.push_back(static_cast<char>(0x80 | ((_point >> 6) & 0x3F)));
      _utf8.push_back(static_cast<char>(0x80 | (_point & 0x3F)));
    }
    else
    {
      _utf8.push_back(static_cast<char>(0xF0 | (_point >> 18)));
      _utf8.push_back(static_cast<char>(0x80 | ((_point >> 12) & 0x3F)));
      _utf8.push_back(static_cast<char>(0x80 | ((_point >> 6) & 0x3F)));
      _utf8.push_back(static_cast<char>(0x80 | (_point & 0x3F)));
    }
  }

  std::span<const std::byte> m_bytes;
  std::size_t m_at = 0;
  std::string m_failure;
  std::size_t m_stringFault = NO_FAULT;
  std::size_t m_floatFault = NO_FAULT;
};

void ReadMaterial(Parser& _in, CmoMaterial& _material)
{
  _material.name = _in.String();
  _in.Floats(_material.ambient);
  _in.Floats(_material.diffuse);
  _in.Floats(_material.specular);
  _material.specularPower = _in.F32();
  _in.Floats(_material.emissive);
  _in.Floats(_material.uvTransform);
  _material.pixelShader = _in.String();
  for (std::string& texture : _material.textures)
  {
    texture = _in.String();
  }
}

void ReadMesh(Parser& _in, CmoMesh& _mesh, MeshOffsets& _offsets)
{
  _offsets.name = _in.At();
  _mesh.name = _in.String();
  _mesh.materials.resize(_in.Count(4 + MATERIAL_BYTES, "materials"));
  for (CmoMaterial& material : _mesh.materials)
  {
    ReadMaterial(_in, material);
  }
  _mesh.skeleton = _in.U8() != 0;
  _mesh.submeshes.resize(_in.Count(SUBMESH_BYTES, "submeshes"));
  for (CmoSubmesh& submesh : _mesh.submeshes)
  {
    _offsets.submeshes.push_back(_in.At());
    submesh.material = _in.U32();
    submesh.indexBuffer = _in.U32();
    submesh.vertexBuffer = _in.U32();
    submesh.startIndex = _in.U32();
    submesh.primitiveCount = _in.U32();
  }
  _mesh.indexBuffers.resize(_in.Count(4, "index buffers"));
  for (std::vector<std::uint16_t>& buffer : _mesh.indexBuffers)
  {
    buffer.resize(_in.Count(2, "indices"));
    _offsets.indexData.push_back(_in.At());
    for (std::uint16_t& index : buffer)
    {
      index = _in.U16();
    }
  }
  _mesh.vertexBuffers.resize(_in.Count(4, "vertex buffers"));
  for (std::vector<CmoVertex>& buffer : _mesh.vertexBuffers)
  {
    buffer.resize(_in.Count(VERTEX_BYTES, "vertices"));
    for (CmoVertex& vertex : buffer)
    {
      _in.Floats(vertex.position);
      _in.Floats(vertex.normal);
      _in.Floats(vertex.tangent);
      vertex.color = _in.U32();
      _in.Floats(vertex.textureCoordinates);
    }
  }
  _offsets.skinning = _in.At();
  _offsets.skinningBuffers = _in.Count(4, "skinning buffers");
  for (std::uint32_t b = 0; b < _offsets.skinningBuffers && !_in.Failed(); ++b)
  {
    const std::uint32_t vertices = _in.Count(SKINNING_VERTEX_BYTES, "skinning vertices");
    for (std::size_t v = 0; v < vertices * (SKINNING_VERTEX_BYTES / 4) && !_in.Failed(); ++v)
    {
      (void)_in.U32();
    }
  }
  _in.Floats(_mesh.extents);
  if (!_mesh.skeleton)
  {
    return;
  }
  _mesh.bones.resize(_in.Count(4 + BONE_MATRICES_BYTES, "bones"));
  for (CmoBone& bone : _mesh.bones)
  {
    _offsets.bones.push_back(_in.At());
    bone.name = _in.String();
    bone.parent = static_cast<std::int32_t>(_in.U32());
    _in.Floats(bone.inverseBindPose);
    _in.Floats(bone.bindPose);
    _in.Floats(bone.localTransform);
  }
  _offsets.clips = _in.At();
  _offsets.clipCount = _in.Count(4 + CLIP_HEADER_BYTES, "animation clips");
  for (std::uint32_t c = 0; c < _offsets.clipCount && !_in.Failed(); ++c)
  {
    (void)_in.String();
    (void)_in.F32();
    (void)_in.F32();
    const std::uint32_t keys = _in.Count(KEYFRAME_BYTES, "keyframes");
    for (std::size_t k = 0; k < keys * (KEYFRAME_BYTES / 4) && !_in.Failed(); ++k)
    {
      (void)_in.U32();
    }
  }
}

} // namespace

std::expected<CmoFile, std::string> ReadCmoFile(std::span<const std::byte> _bytes)
{
  Parser in(_bytes);
  CmoFile file;
  std::vector<MeshOffsets> offsets;
  file.meshes.resize(in.Count(4, "meshes"));
  offsets.resize(file.meshes.size());
  for (std::size_t m = 0; m < file.meshes.size() && !in.Failed(); ++m)
  {
    ReadMesh(in, file.meshes[m], offsets[m]);
  }
  // Check 1: every count led to data inside the file, and reading ends exactly at its end.
  if (!in.Failed() && in.Remaining() != 0)
  {
    in.Fail(in.At(), std::format("{} bytes follow the last mesh", in.Remaining()));
  }
  if (in.Failed())
  {
    return std::unexpected(in.Failure());
  }
  if (file.meshes.empty())
  {
    return std::unexpected(std::string("byte 0: the file holds no mesh (ADR-019 check 1)"));
  }
  // Check 2.
  if (in.StringFault() != NO_FAULT)
  {
    return std::unexpected(
      std::format("byte {}: a string is not UTF-16 of its stated length ending in its one NUL (ADR-019 check 2)", in.StringFault()));
  }
  // Checks 3 and 4.
  for (std::size_t m = 0; m < file.meshes.size(); ++m)
  {
    const CmoMesh& mesh = file.meshes[m];
    for (std::size_t s = 0; s < mesh.submeshes.size(); ++s)
    {
      const CmoSubmesh& submesh = mesh.submeshes[s];
      const bool exists = submesh.material < mesh.materials.size() && submesh.indexBuffer < mesh.indexBuffers.size() &&
                          submesh.vertexBuffer < mesh.vertexBuffers.size();
      if (!exists ||
          std::uint64_t{submesh.startIndex} + std::uint64_t{submesh.primitiveCount} * 3 > mesh.indexBuffers[submesh.indexBuffer].size())
      {
        return std::unexpected(std::format("byte {}: submesh {} of mesh {} names a material or buffer that does not exist, or indices "
                                           "past its index buffer (ADR-019 check 3)",
                                           offsets[m].submeshes[s], s, mesh.name));
      }
    }
    for (std::size_t b = 0; b < mesh.indexBuffers.size(); ++b)
    {
      if (mesh.indexBuffers[b].size() % 3 != 0)
      {
        return std::unexpected(std::format("byte {}: index buffer {} of mesh {} holds {} indices, not a multiple of 3 (ADR-019 check 4)",
                                           offsets[m].indexData[b], b, mesh.name, mesh.indexBuffers[b].size()));
      }
    }
    for (std::size_t s = 0; s < mesh.submeshes.size(); ++s)
    {
      const CmoSubmesh& submesh = mesh.submeshes[s];
      const std::vector<std::uint16_t>& indices = mesh.indexBuffers[submesh.indexBuffer];
      const std::size_t vertices = mesh.vertexBuffers[submesh.vertexBuffer].size();
      for (std::size_t i = submesh.startIndex; i < submesh.startIndex + std::size_t{submesh.primitiveCount} * 3; ++i)
      {
        if (indices[i] >= vertices)
        {
          return std::unexpected(std::format("byte {}: index {} of submesh {} of mesh {} is {}, and its vertex buffer holds {} vertices "
                                             "(ADR-019 check 4)",
                                             offsets[m].indexData[submesh.indexBuffer] + 2 * i, i, s, mesh.name, indices[i], vertices));
        }
      }
    }
  }
  // Check 5.
  if (in.FloatFault() != NO_FAULT)
  {
    return std::unexpected(std::format("byte {}: a float is not finite (ADR-019 check 5)", in.FloatFault()));
  }
  // Check 6.
  for (std::size_t m = 0; m < file.meshes.size(); ++m)
  {
    const std::vector<CmoBone>& bones = file.meshes[m].bones;
    for (std::size_t b = 0; b < bones.size(); ++b)
    {
      if (bones[b].parent < -1 || bones[b].parent >= static_cast<std::int64_t>(b))
      {
        return std::unexpected(std::format("byte {}: bone {} names the parent {}, which is not -1 or a bone before it (ADR-019 check 6)",
                                           offsets[m].bones[b], bones[b].name, bones[b].parent));
      }
    }
  }
  // Check 7.
  std::set<std::string> boneNames;
  for (std::size_t m = 0; m < file.meshes.size(); ++m)
  {
    for (std::size_t b = 0; b < file.meshes[m].bones.size(); ++b)
    {
      if (!boneNames.insert(file.meshes[m].bones[b].name).second)
      {
        return std::unexpected(
          std::format("byte {}: the bone name {} is used twice (ADR-019 check 7)", offsets[m].bones[b], file.meshes[m].bones[b].name));
      }
    }
  }
  std::set<std::string> meshNames;
  for (std::size_t m = 0; m < file.meshes.size(); ++m)
  {
    const std::string& name = file.meshes[m].name;
    if (!meshNames.insert(name).second || (!boneNames.empty() && !boneNames.contains(name)))
    {
      return std::unexpected(
        std::format("byte {}: the mesh name {} is used twice or names no bone of the skeleton (ADR-019 check 7)", offsets[m].name, name));
    }
  }
  // Check 8.
  std::size_t skeletons = 0;
  for (std::size_t m = 0; m < file.meshes.size(); ++m)
  {
    if (file.meshes[m].skeleton)
    {
      ++skeletons;
    }
    if (offsets[m].skinningBuffers != 0 || offsets[m].clipCount != 0 || skeletons > 1)
    {
      return std::unexpected(std::format("byte {}: mesh {} carries skinning buffers, animation clips or a second skeleton, which Species "
                                         "animates procedurally instead (ADR-019 check 8, ADR-035)",
                                         offsets[m].skinningBuffers != 0 ? offsets[m].skinning : offsets[m].clips, file.meshes[m].name));
    }
  }
  return file;
}

} // namespace ContentBuild
