#include "pch.h"

#include "ContentKey.h"
#include "DdsHeader.h"
#include "PlaceholderTexture.h"
#include "RowSpec.h"
#include "TextureCheck.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <numbers>
#include <string_view>
#include <utility>

namespace ContentBuild
{

namespace
{

// The rows that keep a stand-in: the MVP's (AST-082), whose files the owner makes (O88).
constexpr std::string_view STAND_IN_PRIORITY = "mvp";
constexpr std::string_view CONTENT_PREFIX = "GameData/";

// GameAssets.md §2.6's kinds that the build draws.
constexpr std::string_view FIGURE = "generated figure";
constexpr std::string_view GLYPH = "generated glyph";
constexpr std::string_view PANEL = "generated panel";
constexpr std::string_view GRADIENT = "generated gradient";
constexpr std::string_view DERIVED = "derived by the cooker";

// The samples a texel's coverage is the mean of, on each axis.
constexpr int SAMPLES_PER_AXIS = 4;
// A derived variant's blur, in texels of its first level on each side.
constexpr int DERIVED_BLUR_TEXELS = 3;
// A panel's grey.
constexpr float PANEL_GRAY = 0.5f;

// The DDS file's fields (Microsoft's DDS_HEADER and DDS_HEADER_DXT10).
constexpr std::uint32_t DDS_MAGIC = 0x20534444;                         // "DDS "
constexpr std::uint32_t DDS_HEADER_SIZE = 124;                          // DDS_HEADER::dwSize
constexpr std::uint32_t DDS_PIXEL_FORMAT_SIZE = 32;                     // DDS_PIXELFORMAT::dwSize
constexpr std::uint32_t DDS_FLAGS = 0x1 | 0x2 | 0x4 | 0x1000 | 0x80000; // caps, height, width, pixel format, linear size
constexpr std::uint32_t DDS_FLAG_MIP_MAP_COUNT = 0x20000;
constexpr std::uint32_t DDS_FOURCC = 0x4;
constexpr std::uint32_t DDS_FOURCC_DX10 = 0x30315844; // "DX10"
constexpr std::uint32_t DDS_CAPS_TEXTURE = 0x1000;
constexpr std::uint32_t DDS_CAPS_MIP_MAPS = 0x400008; // complex and mip map

// BC7's weights of a 4-bit index (the format's specification).
constexpr std::array<int, 16> BC7_WEIGHTS{0, 4, 9, 13, 17, 21, 26, 30, 34, 38, 43, 47, 51, 55, 60, 64};
// BC4's 8 values when its first end is 255 and its second 0, by index.
constexpr std::array<int, 8> BC4_VALUES{255, 0, 219, 182, 146, 109, 73, 36};

using Shape = std::function<float(double, double)>; // the shape's cover of a point of the unit square, x right and y down

float Disk(double _u, double _v, double _cx, double _cy, double _radius) noexcept
{
  return std::hypot(_u - _cx, _v - _cy) <= _radius ? 1.0f : 0.0f;
}

// ADR-177: the parts a glyph's own stand-in is drawn from, each true where it covers the point (_u, _v).
bool InBox(double _u, double _v, double _left, double _top, double _right, double _bottom) noexcept
{
  return _u >= _left && _u <= _right && _v >= _top && _v <= _bottom;
}

bool InRing(double _u, double _v, double _cx, double _cy, double _inner, double _outer) noexcept
{
  const double r = std::hypot(_u - _cx, _v - _cy);
  return r >= _inner && r <= _outer;
}

// Within _halfWidth of the segment from (_ax, _ay) to (_bx, _by).
bool NearSegment(double _u, double _v, double _ax, double _ay, double _bx, double _by, double _halfWidth) noexcept
{
  const double dx = _bx - _ax;
  const double dy = _by - _ay;
  const double t = std::clamp(((_u - _ax) * dx + (_v - _ay) * dy) / (dx * dx + dy * dy), 0.0, 1.0);
  return std::hypot(_u - (_ax + t * dx), _v - (_ay + t * dy)) <= _halfWidth;
}

bool InTriangle(double _u, double _v, double _ax, double _ay, double _bx, double _by, double _cx, double _cy) noexcept
{
  const auto side = [_u, _v](double _x0, double _y0, double _x1, double _y1)
  { return (_x1 - _x0) * (_v - _y0) - (_y1 - _y0) * (_u - _x0); };
  const double ab = side(_ax, _ay, _bx, _by);
  const double bc = side(_bx, _by, _cx, _cy);
  const double ca = side(_cx, _cy, _ax, _ay);
  return (ab >= 0.0 && bc >= 0.0 && ca >= 0.0) || (ab <= 0.0 && bc <= 0.0 && ca <= 0.0);
}

// A star of _points points round (_cx, _cy), from _outer to _inner, its first point up.
bool InStar(double _u, double _v, double _cx, double _cy, double _outer, double _inner, int _points) noexcept
{
  const double step = std::numbers::pi / _points;
  for (int k = 0; k < 2 * _points; ++k)
  {
    const double a0 = -std::numbers::pi / 2.0 + k * step;
    const double a1 = a0 + step;
    const double r0 = k % 2 == 0 ? _outer : _inner;
    const double r1 = k % 2 == 0 ? _inner : _outer;
    if (InTriangle(_u, _v, _cx, _cy, _cx + r0 * std::cos(a0), _cy + r0 * std::sin(a0), _cx + r1 * std::cos(a1), _cy + r1 * std::sin(a1)))
    {
      return true;
    }
  }
  return false;
}

float Cover(bool _covered) noexcept
{
  return _covered ? 1.0f : 0.0f;
}

/// ADR-177: a glyph row's own stand-in, by the stem of its file, so that the HUD's icons and buttons tell apart before
/// the owner draws them; any other glyph keeps the ring round a dot (ADR-140).
struct GlyphStandIn
{
  std::string_view stem;
  float (*cover)(double, double);
};

constexpr std::array<GlyphStandIn, 20> GLYPH_STAND_INS{{
  // cursor.pointer: an arrow pointing down, which the HUD flips for up (UI-076).
  {"Pointer", [](double _u, double _v)
   { return Cover(InBox(_u, _v, 0.40, 0.12, 0.60, 0.52) || InTriangle(_u, _v, 0.18, 0.45, 0.82, 0.45, 0.50, 0.88)); }},
  // cursor.place: a placing frame of four corners round a dot.
  {"Place",
   [](double _u, double _v)
   {
     const double du = std::abs(_u - 0.5);
     const double dv = std::abs(_v - 0.5);
     const bool corner = du <= 0.34 && dv <= 0.34 && ((du >= 0.26 && dv >= 0.10) || (dv >= 0.26 && du >= 0.10));
     return Cover(corner || std::hypot(_u - 0.5, _v - 0.5) <= 0.06);
   }},
  // icon.hud.resource: a crystal.
  {"Resource", [](double _u, double _v) { return Cover(std::abs(_u - 0.5) / 0.26 + std::abs(_v - 0.5) / 0.40 <= 1.0); }},
  // icon.hud.growth: a plus in a ring.
  {"Growth",
   [](double _u, double _v) {
     return Cover(InRing(_u, _v, 0.5, 0.5, 0.31, 0.41) || InBox(_u, _v, 0.44, 0.28, 0.56, 0.72) || InBox(_u, _v, 0.28, 0.44, 0.72, 0.56));
   }},
  // icon.hud.priority, gathering: a basket.
  {"PriorityGather", [](double _u, double _v)
   { return Cover((InRing(_u, _v, 0.5, 0.42, 0.22, 0.34) && _v >= 0.42) || InBox(_u, _v, 0.12, 0.36, 0.88, 0.44)); }},
  // icon.hud.priority, construction: a hammer.
  {"PriorityConstruct",
   [](double _u, double _v) { return Cover(InBox(_u, _v, 0.20, 0.16, 0.80, 0.38) || InBox(_u, _v, 0.43, 0.38, 0.57, 0.88)); }},
  // icon.hud.priority, guard duty: a shield.
  {"PriorityGuard",
   [](double _u, double _v)
   {
     const double half = _v <= 0.50 ? 0.32 : 0.32 * (0.88 - _v) / 0.38;
     return Cover(_v >= 0.14 && _v <= 0.88 && std::abs(_u - 0.5) <= half);
   }},
  // icon.hud.dwelling: a house with a door.
  {"Dwelling",
   [](double _u, double _v)
   {
     const bool body = InBox(_u, _v, 0.24, 0.48, 0.76, 0.86) && !InBox(_u, _v, 0.43, 0.64, 0.57, 0.86);
     return Cover(body || InTriangle(_u, _v, 0.12, 0.50, 0.88, 0.50, 0.50, 0.14));
   }},
  // icon.hud.storehouse: a crate.
  {"Storehouse",
   [](double _u, double _v)
   {
     const bool outline = InBox(_u, _v, 0.16, 0.22, 0.84, 0.84) && !InBox(_u, _v, 0.24, 0.30, 0.76, 0.76);
     return Cover(outline || InBox(_u, _v, 0.16, 0.49, 0.84, 0.57));
   }},
  // icon.power.gun_turret: a turret's base and barrel.
  {"GunTurret",
   [](double _u, double _v) { return Cover(std::hypot(_u - 0.5, _v - 0.66) <= 0.22 || NearSegment(_u, _v, 0.5, 0.62, 0.82, 0.20, 0.07)); }},
  // icon.hud.contact: two rings that meet.
  {"Contact", [](double _u, double _v) { return Cover(InRing(_u, _v, 0.36, 0.5, 0.18, 0.26) || InRing(_u, _v, 0.64, 0.5, 0.18, 0.26)); }},
  // icon.hud.claim: a flag on its pole.
  {"Claim", [](double _u, double _v)
   { return Cover(InBox(_u, _v, 0.22, 0.12, 0.30, 0.88) || InTriangle(_u, _v, 0.30, 0.14, 0.30, 0.52, 0.84, 0.33)); }},
  // icon.hud.attack: two crossed blades.
  {"Attack", [](double _u, double _v)
   { return Cover(NearSegment(_u, _v, 0.2, 0.2, 0.8, 0.8, 0.07) || NearSegment(_u, _v, 0.8, 0.2, 0.2, 0.8, 0.07)); }},
  // icon.hud.battle: crossed blades in a ring.
  {"Battle",
   [](double _u, double _v)
   {
     return Cover(InRing(_u, _v, 0.5, 0.5, 0.35, 0.43) || NearSegment(_u, _v, 0.32, 0.32, 0.68, 0.68, 0.06) ||
                  NearSegment(_u, _v, 0.68, 0.32, 0.32, 0.68, 0.06));
   }},
  // icon.hud.tribute: a stack of coins.
  {"Tribute",
   [](double _u, double _v) {
     return Cover(InBox(_u, _v, 0.20, 0.64, 0.80, 0.78) || InBox(_u, _v, 0.27, 0.45, 0.73, 0.59) || InBox(_u, _v, 0.34, 0.26, 0.66, 0.40));
   }},
  // icon.hud.withdraw: an arrow pointing back, to the left.
  {"Withdraw", [](double _u, double _v)
   { return Cover(InBox(_u, _v, 0.42, 0.42, 0.86, 0.58) || InTriangle(_u, _v, 0.12, 0.50, 0.48, 0.20, 0.48, 0.80)); }},
  // icon.hud.respite: a pause.
  {"Respite", [](double _u, double _v) { return Cover(InBox(_u, _v, 0.26, 0.20, 0.42, 0.80) || InBox(_u, _v, 0.58, 0.20, 0.74, 0.80)); }},
  // icon.hud.renown: a star.
  {"Renown", [](double _u, double _v) { return Cover(InStar(_u, _v, 0.5, 0.54, 0.42, 0.18, 5)); }},
  // icon.hud.run_clock: a clock.
  {"RunClock",
   [](double _u, double _v)
   {
     return Cover(InRing(_u, _v, 0.5, 0.5, 0.33, 0.41) || NearSegment(_u, _v, 0.5, 0.5, 0.5, 0.26, 0.045) ||
                  NearSegment(_u, _v, 0.5, 0.5, 0.68, 0.5, 0.045));
   }},
  // icon.hud.deputy: two rank chevrons.
  {"Deputy",
   [](double _u, double _v)
   {
     const auto chevron = [_u, _v](double _top)
     { return NearSegment(_u, _v, 0.20, _top + 0.22, 0.50, _top, 0.06) || NearSegment(_u, _v, 0.50, _top, 0.80, _top + 0.22, 0.06); };
     return Cover(chevron(0.24) || chevron(0.48));
   }},
}};

// The stem of a file under GameData/: its name without its folder or extension.
std::string_view StemOf(std::string_view _filename) noexcept
{
  const std::size_t slash = _filename.find_last_of('/');
  std::string_view name = slash == std::string_view::npos ? _filename : _filename.substr(slash + 1);
  return name.substr(0, name.find_last_of('.'));
}

// ADR-177: the glyph row's own stand-in, and a derived variant's its base's (`<stem>Shadow`), if it has one.
std::optional<Shape> GlyphShapeOf(std::string_view _filename)
{
  constexpr std::string_view SHADOW = "Shadow";
  std::string_view stem = StemOf(_filename);
  if (stem.ends_with(SHADOW))
  {
    stem.remove_suffix(SHADOW.size());
  }
  for (const GlyphStandIn& standIn : GLYPH_STAND_INS)
  {
    if (standIn.stem == stem)
    {
      return Shape(standIn.cover);
    }
  }
  return std::nullopt;
}

std::optional<Shape> ShapeOf(std::string_view _kind)
{
  if (_kind == FIGURE)
  {
    return Shape([](double _u, double _v) { return std::abs(_u - 0.5) + std::abs(_v - 0.3) <= 0.2 ? 1.0f : 0.0f; });
  }
  if (_kind == GLYPH || _kind == DERIVED)
  {
    return Shape(
      [](double _u, double _v)
      {
        const double r = std::hypot(_u - 0.5, _v - 0.5);
        return (r >= 0.30 && r <= 0.42) || r <= 0.10 ? 1.0f : 0.0f;
      });
  }
  if (_kind == PANEL)
  {
    return Shape([](double, double) { return 1.0f; });
  }
  if (_kind == GRADIENT)
  {
    return Shape([](double _u, double _v) { return static_cast<float>(std::max(0.0, 1.0 - std::hypot(_u - 0.5, _v - 0.5) / 0.5)); });
  }
  return std::nullopt;
}

// A level's coverage of the shape, each texel the mean of its samples.
std::vector<float> Coverage(const Shape& _shape, std::uint32_t _width, std::uint32_t _height)
{
  std::vector<float> coverage(static_cast<std::size_t>(_width) * _height, 0.0f);
  for (std::uint32_t y = 0; y < _height; ++y)
  {
    for (std::uint32_t x = 0; x < _width; ++x)
    {
      double sum = 0.0;
      for (int sy = 0; sy < SAMPLES_PER_AXIS; ++sy)
      {
        for (int sx = 0; sx < SAMPLES_PER_AXIS; ++sx)
        {
          const double u = (x + (sx + 0.5) / SAMPLES_PER_AXIS) / _width;
          const double v = (y + (sy + 0.5) / SAMPLES_PER_AXIS) / _height;
          sum += _shape(u, v);
        }
      }
      coverage[static_cast<std::size_t>(y) * _width + x] = static_cast<float>(sum / (SAMPLES_PER_AXIS * SAMPLES_PER_AXIS));
    }
  }
  return coverage;
}

// A box blur of _radius texels, clamped at the edges.
std::vector<float> Blurred(const std::vector<float>& _coverage, std::uint32_t _width, std::uint32_t _height, int _radius)
{
  std::vector<float> blurred(_coverage.size(), 0.0f);
  for (std::uint32_t y = 0; y < _height; ++y)
  {
    for (std::uint32_t x = 0; x < _width; ++x)
    {
      double sum = 0.0;
      for (int dy = -_radius; dy <= _radius; ++dy)
      {
        for (int dx = -_radius; dx <= _radius; ++dx)
        {
          const auto sx = static_cast<std::uint32_t>(std::clamp<std::int64_t>(std::int64_t{x} + dx, 0, std::int64_t{_width} - 1));
          const auto sy = static_cast<std::uint32_t>(std::clamp<std::int64_t>(std::int64_t{y} + dy, 0, std::int64_t{_height} - 1));
          sum += _coverage[static_cast<std::size_t>(sy) * _width + sx];
        }
      }
      blurred[static_cast<std::size_t>(y) * _width + x] = static_cast<float>(sum / ((2 * _radius + 1) * (2 * _radius + 1)));
    }
  }
  return blurred;
}

// The texel of a block, clamped to the level, since a level smaller than a block still takes one.
float TexelOf(std::span<const float> _coverage, std::uint32_t _width, std::uint32_t _height, std::uint32_t _x, std::uint32_t _y) noexcept
{
  return _coverage[static_cast<std::size_t>(std::min(_y, _height - 1)) * _width + std::min(_x, _width - 1)];
}

/// 128 bits written from the least significant bit of the first byte, as a BC7 block is.
class BlockBits
{
public:
  void Write(std::uint32_t _value, int _bits) noexcept
  {
    for (int bit = 0; bit < _bits; ++bit, ++m_at)
    {
      if (((_value >> bit) & 1u) != 0)
      {
        m_bytes[static_cast<std::size_t>(m_at / 8)] |= static_cast<std::uint8_t>(1u << (m_at % 8));
      }
    }
  }
  [[nodiscard]] const std::array<std::uint8_t, 16>& Bytes() const noexcept
  {
    return m_bytes;
  }

private:
  std::array<std::uint8_t, 16> m_bytes{};
  int m_at = 0;
};

// A BC7 mode 6 end: four channels of 7 bits and the bit they share, which make 8 bits each.
struct Bc7End
{
  std::array<int, 4> channels{};
  int pBit = 0;

  [[nodiscard]] int Value(std::size_t _channel) const noexcept
  {
    return (channels[_channel] << 1) | pBit;
  }
};

// The end nearest to an 8-bit colour, over both values of its shared bit.
Bc7End EndOf(const std::array<int, 4>& _color) noexcept
{
  Bc7End best;
  int bestError = -1;
  for (int p = 0; p <= 1; ++p)
  {
    Bc7End end;
    end.pBit = p;
    int error = 0;
    for (std::size_t c = 0; c < 4; ++c)
    {
      end.channels[c] = std::clamp((_color[c] - p + 1) / 2, 0, 127);
      error += std::abs(end.Value(c) - _color[c]);
    }
    if (bestError < 0 || error < bestError)
    {
      best = end;
      bestError = error;
    }
  }
  return best;
}

int Bc7Interpolate(int _a, int _b, int _weight) noexcept
{
  return ((64 - _weight) * _a + _weight * _b + 32) >> 6;
}

void WriteWord(std::vector<std::byte>& _file, std::uint32_t _value)
{
  for (int shift = 0; shift < 32; shift += 8)
  {
    _file.push_back(static_cast<std::byte>((_value >> shift) & 0xFF));
  }
}

} // namespace

std::vector<std::byte> EncodeBc7Coverage(std::span<const float> _coverage, std::uint32_t _width, std::uint32_t _height, float _red,
                                         float _green, float _blue)
{
  // Mode 6: one subset whose ends are the colour clear and opaque, so that a texel's index gives its alpha.
  const auto to8 = [](float _value) { return static_cast<int>(std::lround(std::clamp(_value, 0.0f, 1.0f) * 255.0f)); };
  const std::array<int, 4> clear{to8(_red), to8(_green), to8(_blue), 0};
  const std::array<int, 4> opaque{to8(_red), to8(_green), to8(_blue), 255};
  std::vector<std::byte> blocks;
  for (std::uint32_t by = 0; by < std::max<std::uint32_t>(1, (_height + 3) / 4); ++by)
  {
    for (std::uint32_t bx = 0; bx < std::max<std::uint32_t>(1, (_width + 3) / 4); ++bx)
    {
      std::array<Bc7End, 2> ends{EndOf(clear), EndOf(opaque)};
      std::array<int, 16> indices{};
      for (std::uint32_t t = 0; t < 16; ++t)
      {
        const int alpha = to8(TexelOf(_coverage, _width, _height, bx * 4 + t % 4, by * 4 + t / 4));
        int best = 0;
        for (int i = 1; i < 16; ++i)
        {
          const auto weight = static_cast<std::size_t>(i);
          if (std::abs(Bc7Interpolate(ends[0].Value(3), ends[1].Value(3), BC7_WEIGHTS[weight]) - alpha) <
              std::abs(Bc7Interpolate(ends[0].Value(3), ends[1].Value(3), BC7_WEIGHTS[static_cast<std::size_t>(best)]) - alpha))
          {
            best = i;
          }
        }
        indices[t] = best;
      }
      // The first texel's index keeps its top bit clear: when it would not, the ends change places.
      if (indices[0] >= 8)
      {
        std::swap(ends[0], ends[1]);
        for (int& index : indices)
        {
          index = 15 - index;
        }
      }
      BlockBits bits;
      bits.Write(1u << 6, 7); // mode 6
      for (std::size_t c = 0; c < 4; ++c)
      {
        bits.Write(static_cast<std::uint32_t>(ends[0].channels[c]), 7);
        bits.Write(static_cast<std::uint32_t>(ends[1].channels[c]), 7);
      }
      bits.Write(static_cast<std::uint32_t>(ends[0].pBit), 1);
      bits.Write(static_cast<std::uint32_t>(ends[1].pBit), 1);
      for (std::size_t t = 0; t < 16; ++t)
      {
        bits.Write(static_cast<std::uint32_t>(indices[t]), t == 0 ? 3 : 4);
      }
      for (const std::uint8_t byte : bits.Bytes())
      {
        blocks.push_back(static_cast<std::byte>(byte));
      }
    }
  }
  return blocks;
}

std::vector<std::byte> EncodeBc4Coverage(std::span<const float> _coverage, std::uint32_t _width, std::uint32_t _height)
{
  std::vector<std::byte> blocks;
  for (std::uint32_t by = 0; by < std::max<std::uint32_t>(1, (_height + 3) / 4); ++by)
  {
    for (std::uint32_t bx = 0; bx < std::max<std::uint32_t>(1, (_width + 3) / 4); ++bx)
    {
      // Ends of 255 and 0, so that the 8 values of BC4_VALUES are the palette.
      blocks.push_back(std::byte{255});
      blocks.push_back(std::byte{0});
      std::uint64_t indices = 0;
      for (std::uint32_t t = 0; t < 16; ++t)
      {
        const int value = static_cast<int>(
          std::lround(std::clamp(TexelOf(_coverage, _width, _height, bx * 4 + t % 4, by * 4 + t / 4), 0.0f, 1.0f) * 255.0f));
        const auto nearest = std::ranges::min_element(BC4_VALUES, {}, [value](int _palette) { return std::abs(_palette - value); });
        indices |= static_cast<std::uint64_t>(std::distance(BC4_VALUES.begin(), nearest)) << (3 * t);
      }
      for (int shift = 0; shift < 48; shift += 8)
      {
        blocks.push_back(static_cast<std::byte>((indices >> shift) & 0xFF));
      }
    }
  }
  return blocks;
}

std::optional<Neuron::CookedEntry> MakePlaceholderTexture(const ManifestRow& _row)
{
  if (_row.priority != STAND_IN_PRIORITY || !_row.speciesFilename.starts_with(CONTENT_PREFIX) || !_row.speciesFilename.ends_with(".dds"))
  {
    return std::nullopt;
  }
  std::optional<Shape> shape = ShapeOf(_row.placeholder);
  if (_row.placeholder == GLYPH || _row.placeholder == DERIVED)
  {
    if (std::optional<Shape> own = GlyphShapeOf(_row.speciesFilename); own.has_value())
    {
      shape = std::move(own);
    }
  }
  const bool bc7 = _row.targetFormat.find("BC7_UNORM_SRGB") != std::string::npos;
  const bool bc4 = _row.targetFormat.find("BC4_UNORM") != std::string::npos;
  const std::optional<PixelSize> display = PixelSizeAfter(_row.spec);
  if (!shape || (bc7 == bc4) || !display)
  {
    return std::nullopt;
  }
  const std::uint32_t scale = _row.spec.find("author at 2x") != std::string::npos ? UI_AUTHORING_SCALE : 1;
  const std::uint32_t width = display->width * scale;
  const std::uint32_t height = display->height * scale;
  const bool fullChain = _row.targetFormat.find("full mip chain") != std::string::npos;
  const std::uint32_t levels = fullChain ? Neuron::FullMipLevels(width, height) : 1;
  const Neuron::TextureFormat format = bc7 ? Neuron::TextureFormat::Bc7UnormSrgb : Neuron::TextureFormat::Bc4Unorm;
  const float gray = _row.placeholder == PANEL ? PANEL_GRAY : 1.0f;

  std::vector<std::byte> payload;
  for (std::uint32_t level = 0; level < levels; ++level)
  {
    const std::uint32_t w = std::max<std::uint32_t>(1, width >> level);
    const std::uint32_t h = std::max<std::uint32_t>(1, height >> level);
    std::vector<float> coverage = Coverage(*shape, w, h);
    if (_row.placeholder == DERIVED)
    {
      coverage = Blurred(coverage, w, h, std::max(1, DERIVED_BLUR_TEXELS >> static_cast<int>(level)));
    }
    const std::vector<std::byte> blocks = bc7 ? EncodeBc7Coverage(coverage, w, h, gray, gray, gray) : EncodeBc4Coverage(coverage, w, h);
    payload.insert(payload.end(), blocks.begin(), blocks.end());
  }

  std::vector<std::byte> file;
  WriteWord(file, DDS_MAGIC);
  WriteWord(file, DDS_HEADER_SIZE);
  WriteWord(file, DDS_FLAGS | (levels > 1 ? DDS_FLAG_MIP_MAP_COUNT : 0));
  WriteWord(file, height);
  WriteWord(file, width);
  WriteWord(file, static_cast<std::uint32_t>(Neuron::LevelBytes(format, width, height))); // the first level's bytes
  WriteWord(file, 0);                                                                     // depth
  WriteWord(file, levels);
  for (int reserved = 0; reserved < 11; ++reserved)
  {
    WriteWord(file, 0);
  }
  WriteWord(file, DDS_PIXEL_FORMAT_SIZE);
  WriteWord(file, DDS_FOURCC);
  WriteWord(file, DDS_FOURCC_DX10);
  for (int mask = 0; mask < 5; ++mask)
  {
    WriteWord(file, 0); // the bit count and the four masks, which a DX10 header leaves unused
  }
  WriteWord(file, DDS_CAPS_TEXTURE | (levels > 1 ? DDS_CAPS_MIP_MAPS : 0));
  for (int caps = 0; caps < 4; ++caps)
  {
    WriteWord(file, 0); // the other caps and the last reserved word
  }
  WriteWord(file, static_cast<std::uint32_t>(format)); // DXGI_FORMAT
  WriteWord(file, Neuron::DDS_TEXTURE_2D);
  WriteWord(file, 0); // flags
  WriteWord(file, 1); // array size
  WriteWord(file, 0); // alpha mode, unknown
  file.insert(file.end(), payload.begin(), payload.end());

  Neuron::CookedEntry entry;
  entry.entry.key = Neuron::KeyOfSource(std::string_view(_row.speciesFilename).substr(CONTENT_PREFIX.size()));
  entry.entry.type = Neuron::ContentType::Texture;
  entry.entry.placeholder = true;
  entry.bytes = std::move(file);
  return entry;
}

} // namespace ContentBuild
