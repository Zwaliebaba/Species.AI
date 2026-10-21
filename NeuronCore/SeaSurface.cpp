#include "pch.h"

#include "SeaSurface.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>

namespace Neuron
{

namespace
{

constexpr std::uint32_t WHITE = 0xFFFF'FFFFu;

std::vector<float> WaveTable(const std::array<WaveTerm, 2>& _terms, double _amplitude, double _cell, std::uint32_t _size, double _seconds)
{
  std::vector<float> table(_size);
  const auto time = static_cast<float>(_seconds);
  for (std::uint32_t i = 0; i < _size; ++i)
  {
    const auto distance = static_cast<float>(static_cast<double>(i) * _cell);
    float sum = 0.0f;
    for (const WaveTerm& term : _terms)
    {
      sum += static_cast<float>(term.amplitude) *
             std::sin(static_cast<float>(term.perWu) * distance + static_cast<float>(term.perSecond) * time);
    }
    table[i] = static_cast<float>(_amplitude) * sum;
  }
  return table;
}

std::uint32_t ColorAt(const std::vector<std::uint32_t>& _table, float _index) noexcept
{
  if (_table.empty())
  {
    return WHITE;
  }
  // TER-205: truncated towards zero, then clamped to the table.
  const float limit = static_cast<float>(_table.size() - 1);
  const float clamped = std::clamp(std::trunc(_index), 0.0f, limit);
  return _table[static_cast<std::size_t>(clamped)];
}

} // namespace

std::vector<std::uint32_t> ImageRowColors(const CpuImage& _image, std::uint32_t _rowFromBottom)
{
  std::vector<std::uint32_t> colors;
  if (_image.width == 0 || _image.height == 0 || _image.texels.size() < std::size_t{_image.width} * _image.height * 4)
  {
    return colors;
  }
  const std::uint32_t row = _image.height - 1 - std::min(_rowFromBottom, _image.height - 1);
  colors.reserve(_image.width);
  for (std::uint32_t x = 0; x < _image.width; ++x)
  {
    const std::size_t at = (std::size_t{row} * _image.width + x) * 4;
    colors.push_back(std::uint32_t{_image.texels[at]} | (std::uint32_t{_image.texels[at + 1]} << 8) |
                     (std::uint32_t{_image.texels[at + 2]} << 16) | 0xFF00'0000u);
  }
  return colors;
}

SeaSurface BuildSeaSurface(const TerrainField& _field, const SeaLook& _look, TileRandom _shoreNoise, const CpuImage& _gradient,
                           std::uint32_t _gradientRow)
{
  const double sideX = _field.ExtentX();
  const double sideZ = _field.ExtentZ();
  const double regionX = _field.Heights().OriginX();
  const double regionZ = _field.Heights().OriginZ();
  SeaSurface sea;
  sea.planeOrigin = {regionX - sideX / 2.0, 0.0, regionZ - sideZ / 2.0};
  sea.planeSide = 2.0 * std::max(sideX, sideZ);
  const std::uint32_t size = std::max<std::uint32_t>(1, _look.maskSize);
  sea.maskSize = size;

  // TER-196: the land values, in binary32, over the plane; pixel (x, z) at the plane origin plus (x, z) pixels.
  const auto pixelX = static_cast<float>(2.0 * sideX / size);
  const auto pixelZ = static_cast<float>(2.0 * sideZ / size);
  const std::uint32_t offset = _look.maskLandOffset;
  const auto index = [size](std::uint32_t _x, std::uint32_t _z) { return std::size_t{_z} * size + _x; };
  std::vector<float> land(std::size_t{size} * size, 0.0f);
  for (std::uint32_t z = offset; z + offset < size; ++z)
  {
    for (std::uint32_t x = offset; x + offset < size; ++x)
    {
      const double worldX = regionX + static_cast<double>(static_cast<float>(x) * pixelX - static_cast<float>(offset) * pixelX);
      const double worldZ = regionZ + static_cast<double>(static_cast<float>(z) * pixelZ - static_cast<float>(offset) * pixelZ);
      if (_field.Height(worldX, worldZ) > _look.seaLevel)
      {
        land[index(x, z)] = 1.0f;
      }
    }
  }

  // TER-197: the scatter blur, horizontally and then vertically, contributions outside the image dropped.
  std::array<float, BLUR_TAP_COUNT> taps{};
  for (std::size_t i = 0; i < BLUR_TAP_COUNT; ++i)
  {
    taps[i] = static_cast<float>(_look.blurTaps[i] / _look.blurNormalizer);
  }
  const auto tolerance = static_cast<float>(_look.blurSkipTolerance);
  const auto centerTap = static_cast<std::int64_t>(BLUR_TAP_COUNT / 2);
  std::vector<float> across(land.size(), 0.0f);
  for (std::uint32_t z = 0; z < size; ++z)
  {
    for (std::uint32_t x = 0; x < size; ++x)
    {
      const float value = land[index(x, z)];
      if (std::abs(value) <= tolerance)
      {
        continue;
      }
      for (std::size_t i = 0; i < BLUR_TAP_COUNT; ++i)
      {
        const std::int64_t target = static_cast<std::int64_t>(x) + static_cast<std::int64_t>(i) - centerTap;
        if (target >= 0 && target < size)
        {
          across[index(static_cast<std::uint32_t>(target), z)] += value * taps[i];
        }
      }
    }
  }
  std::vector<float> blurred(land.size(), 0.0f);
  for (std::uint32_t z = 0; z < size; ++z)
  {
    for (std::uint32_t x = 0; x < size; ++x)
    {
      const float value = across[index(x, z)];
      if (std::abs(value) <= tolerance)
      {
        continue;
      }
      for (std::size_t i = 0; i < BLUR_TAP_COUNT; ++i)
      {
        const std::int64_t target = static_cast<std::int64_t>(z) + static_cast<std::int64_t>(i) - centerTap;
        if (target >= 0 && target < size)
        {
          blurred[index(x, static_cast<std::uint32_t>(target))] += value * taps[i];
        }
      }
    }
  }
  // The gray levels in mask coordinates, and the image, flipped: its row 0 holds mask row size - 1.
  std::vector<std::uint8_t> gray(land.size(), 0);
  sea.shoreMask.assign(land.size(), 0);
  for (std::uint32_t z = 0; z < size; ++z)
  {
    for (std::uint32_t x = 0; x < size; ++x)
    {
      const float level = std::min(static_cast<float>(_look.maskMax), blurred[index(x, z)] * static_cast<float>(_look.maskGain));
      const auto value = static_cast<std::uint8_t>(std::clamp(std::trunc(level), 0.0f, 255.0f));
      gray[index(x, z)] = value;
      sea.shoreMask[index(x, size - 1 - z)] = value;
    }
  }

  // TER-198: the depth map over the plane, cell 2X ÷ S on both axes; 1 is open sea and 0 is on or next to land.
  SurfaceGrid<float> depth(static_cast<std::uint16_t>(size), static_cast<std::uint16_t>(size), sea.planeOrigin.x, sea.planeOrigin.z,
                           2.0 * sideX / size, static_cast<float>(_look.depthOutside));
  for (std::uint32_t z = 0; z < size; ++z)
  {
    for (std::uint32_t x = 0; x < size; ++x)
    {
      depth.Set(x, z, 1.0f - static_cast<float>(gray[index(x, z)]) / 255.0f);
    }
  }

  // TER-199 and RND-141: a tile is drawn unless every depth sample of its block is open sea.
  const std::uint32_t ratio = std::max<std::uint32_t>(1, _look.flatTileRatio);
  const std::uint32_t tiles = size / ratio;
  const auto tileSide = static_cast<float>(depth.CellSize() * ratio);
  const auto flatHeight = static_cast<float>(_look.flatHeight);
  for (std::uint32_t tz = 0; tz < tiles; ++tz)
  {
    for (std::uint32_t tx = 0; tx < tiles; ++tx)
    {
      bool open = true;
      for (std::uint32_t j = 0; j < ratio && open; ++j)
      {
        for (std::uint32_t i = 0; i < ratio && open; ++i)
        {
          open = depth.At(tx * ratio + i, tz * ratio + j) >= 1.0f;
        }
      }
      if (open)
      {
        continue;
      }
      const float x0 = static_cast<float>(tx) * tileSide;
      const float z0 = static_cast<float>(tz) * tileSide;
      const SeaVertex a{{x0, flatHeight, z0}, WHITE};
      const SeaVertex b{{x0, flatHeight, z0 + tileSide}, WHITE};
      const SeaVertex c{{x0 + tileSide, flatHeight, z0}, WHITE};
      const SeaVertex d{{x0 + tileSide, flatHeight, z0 + tileSide}, WHITE};
      sea.flatVertices.insert(sea.flatVertices.end(), {a, b, c, c, b, d});
    }
  }

  // TER-041 and RND-144: the wave colour table.
  sea.waveColors = ImageRowColors(_gradient, _gradientRow);
  if (_look.detail == 0)
  {
    return sea;
  }

  // TER-200 to TER-202: the wave mesh of the look's level.
  const double cell = static_cast<double>(_look.detail) * std::max(sideX, sideZ) / _look.cellDivisor;
  sea.waterCell = cell;
  const auto cellsX = static_cast<std::uint32_t>(2.0 * sideX / cell);
  const auto cellsZ = static_cast<std::uint32_t>(2.0 * sideZ / cell);
  sea.tableX = static_cast<std::uint32_t>(2.0 * sideX / cell + _look.tableMargin);
  sea.tableZ = static_cast<std::uint32_t>(2.0 * sideZ / cell + _look.tableMargin);
  const auto needed = [&](double _x, double _z)
  { return !(_field.Height(_x, _z) > _look.vertexLandLimit || static_cast<double>(depth.Sample(_x, _z)) > _look.vertexDepthLimit); };
  const auto noise = [&](float _depth)
  {
    const float shore = 1.0f - _depth;
    return static_cast<float>((static_cast<double>(shore) + _shoreNoise.Signed(shore) * _look.shoreNoise) * _look.shoreBrightening);
  };
  std::optional<WavePair> previous;
  for (std::uint32_t zb = 0; zb < cellsZ; ++zb)
  {
    for (std::uint32_t xb = 0; xb < cellsX; ++xb)
    {
      const double fx = sea.planeOrigin.x + static_cast<double>(xb) * cell;
      const double fz = sea.planeOrigin.z + static_cast<double>(zb) * cell;
      bool emitted = false;
      for (int dz = 0; dz <= 1 && !emitted; ++dz)
      {
        for (int dx = -1; dx <= 1 && !emitted; ++dx)
        {
          emitted = needed(fx + dx * cell, fz + dz * cell);
        }
      }
      if (!emitted)
      {
        continue;
      }
      WavePair pair;
      pair.xb = xb;
      pair.zb = zb;
      pair.depth1 = depth.Sample(fx, fz);
      pair.depth2 = depth.Sample(fx, fz + cell);
      pair.noise1 = noise(pair.depth1);
      pair.noise2 = noise(pair.depth2);
      pair.joinsPrevious = previous.has_value() && previous->zb == zb && previous->xb + 1 == xb;
      sea.waveTriangles += pair.joinsPrevious ? 2 : 0;
      sea.pairs.push_back(pair);
      previous = pair;
    }
  }
  return sea;
}

void EvaluateWaves(const SeaSurface& _sea, const SeaLook& _look, double _seconds, std::vector<SeaVertex>& _out)
{
  _out.resize(std::size_t{_sea.waveTriangles} * 3);
  if (_sea.pairs.empty() || _sea.waterCell <= 0.0)
  {
    return;
  }
  const std::vector<float> waveX = WaveTable(_look.xTerms, _look.amplitudeScale, _sea.waterCell, _sea.tableX, _seconds);
  const std::vector<float> waveZ = WaveTable(_look.zTerms, _look.amplitudeScale, _sea.waterCell, _sea.tableZ, _seconds);
  const auto brightness = static_cast<float>(_look.brightnessScale);
  const auto other = static_cast<float>(_look.mixOther);
  const auto own = static_cast<float>(_look.mixOwn);
  const auto cell = static_cast<float>(_sea.waterCell);
  float previousH1 = 0.0f;
  float previousH2 = 0.0f;
  SeaVertex previous1{};
  SeaVertex previous2{};
  std::size_t at = 0;
  for (const WavePair& pair : _sea.pairs)
  {
    // TER-205: the index from the plane origin is the pair's own column and row; the bias guards the truncation.
    const auto ix = static_cast<std::size_t>(static_cast<double>(pair.xb) + _look.indexBias);
    const auto iz = static_cast<std::size_t>(static_cast<double>(pair.zb) + _look.indexBias);
    const float tableX = ix < waveX.size() ? waveX[ix] : 0.0f;
    const float tableZ1 = iz < waveZ.size() ? waveZ[iz] : 0.0f;
    const float tableZ2 = iz + 1 < waveZ.size() ? waveZ[iz + 1] : 0.0f;
    const float h1 = (tableX + tableZ1) * pair.depth1;
    const float b1 = (previousH1 + previousH2 + h1) * brightness * (1.0f - pair.depth1) + pair.noise1;
    previousH1 = h1;
    const float h2 = (tableX + tableZ2) * pair.depth2;
    const float b2 = (previousH2 + previousH1 + h2) * brightness * (1.0f - pair.depth2) + pair.noise2;
    previousH2 = h2;
    const float x = static_cast<float>(pair.xb) * cell;
    const float z = static_cast<float>(pair.zb) * cell;
    const SeaVertex v1{{x, h1, z}, ColorAt(_sea.waveColors, other * b2 + own * b1)};
    const SeaVertex v2{{x, h2, z + cell}, ColorAt(_sea.waveColors, other * b1 + own * b2)};
    if (pair.joinsPrevious && at + 6 <= _out.size())
    {
      // RND-145: (p1, p2, q1) takes q1's colour and (q1, p2, q2) takes q2's.
      _out[at++] = {previous1.position, v1.color};
      _out[at++] = {previous2.position, v1.color};
      _out[at++] = {v1.position, v1.color};
      _out[at++] = {v1.position, v2.color};
      _out[at++] = {previous2.position, v2.color};
      _out[at++] = {v2.position, v2.color};
    }
    previous1 = v1;
    previous2 = v2;
  }
}

} // namespace Neuron
