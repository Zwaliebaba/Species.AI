#include "pch.h"

#include "SkyLayers.h"

#include <algorithm>
#include <cmath>

namespace Neuron
{

namespace
{

Float4 Color(const std::array<float, 4>& _color) noexcept
{
  return {_color[0], _color[1], _color[2], _color[3]};
}

} // namespace

std::vector<SkyLayerInstance> BuildSkyLayers(const SkyLook& _sky, const Vec3& _camera, double _skyTicks, std::uint32_t _beam,
                                             std::uint32_t _cloud)
{
  std::vector<SkyLayerInstance> layers;
  const double spacing = std::max(_sky.gridSpacing, 1.0);
  const auto lines = static_cast<std::int64_t>((_sky.gridMax - _sky.gridMin) / spacing);
  const double reach = (_sky.gridMax - _sky.gridMin) / 2.0;
  const auto gridHeight = static_cast<float>(_sky.gridHeight - _camera.y);
  const Float4 gridShape{gridHeight, static_cast<float>(_sky.gridFogStart), static_cast<float>(_sky.gridFogEnd), 0.0f};
  const auto halfWidth = static_cast<float>(_sky.gridHalfWidth);
  const auto lineReach = static_cast<float>(reach);
  layers.reserve(static_cast<std::size_t>(2 * lines + 5));

  // RND-151 and ADR-115: the lines stand on the lattice of the region's origin, and the `lines` positions nearest the
  // camera are drawn, each reaching as far as the grid's extent on either side of the camera, beyond its fog.
  const std::int64_t firstX = std::llround((_camera.x - _sky.regionOriginX) / spacing) - lines / 2;
  for (std::int64_t k = 0; k < lines; ++k)
  {
    const auto x = static_cast<float>(_sky.regionOriginX + spacing * static_cast<double>(firstX + k) - _camera.x);
    layers.push_back({{x - halfWidth, -lineReach, x + halfWidth, lineReach},
                      Color(_sky.gridColor),
                      {0.0f, 0.0f, 1.0f, 1.0f},
                      gridShape,
                      {_beam, SAMPLER_LINEAR_CLAMP, SKY_ALONG_Z, 0}});
  }
  const std::int64_t firstZ = std::llround((_camera.z - _sky.regionOriginZ) / spacing) - lines / 2;
  for (std::int64_t k = 0; k < lines; ++k)
  {
    const auto z = static_cast<float>(_sky.regionOriginZ + spacing * static_cast<double>(firstZ + k) - _camera.z);
    layers.push_back({{-lineReach, z - halfWidth, lineReach, z + halfWidth},
                      Color(_sky.gridColor),
                      {0.0f, 0.0f, 1.0f, 1.0f},
                      gridShape,
                      {_beam, SAMPLER_LINEAR_CLAMP, 0, 0}});
  }

  // RND-152: the offset in texture repeats, one tick's worth of the velocity per displayed tick (ADR-115).
  const Vec3 offset = _sky.velocity * (_sky.tickSeconds * _skyTicks);
  const auto ox = static_cast<float>(offset.x - std::floor(offset.x));
  const auto oz = static_cast<float>(offset.z - std::floor(offset.z));
  const auto halfX = static_cast<float>(offset.x / 2.0 - std::floor(offset.x / 2.0));
  const double cloudHalf = (_sky.cloudMax - _sky.cloudMin) / 2.0;
  const Float4 rectangle{
    static_cast<float>(_sky.regionCenter.x - cloudHalf - _camera.x), static_cast<float>(_sky.regionCenter.z - cloudHalf - _camera.z),
    static_cast<float>(_sky.regionCenter.x + cloudHalf - _camera.x), static_cast<float>(_sky.regionCenter.z + cloudHalf - _camera.z)};
  const auto cloud = [&](double _height, const std::array<float, 4>& _color, float _u, float _v, double _repeats, bool _flat)
  {
    const auto repeats = static_cast<float>(_repeats);
    layers.push_back(
      {rectangle,
       Color(_color),
       {_u, _v, repeats, repeats},
       {static_cast<float>(_height - _camera.y), static_cast<float>(_sky.cloudFogStart), static_cast<float>(_sky.cloudFogEnd), 0.0f},
       {_cloud, _flat ? SAMPLER_POINT_CLAMP : SAMPLER_LINEAR_WRAP, _flat ? SKY_LEVEL_ZERO | SKY_WRAP_IN_SHADER : SKY_LEVEL_ZERO, 0}});
  };
  // RND-153 at sky detail 1, every layer: its height, colour and coordinates as the table gives them (layer 2 reads
  // o.x on both axes and layer 3 swaps them, as written).
  cloud(_sky.gridHeight, _sky.blobbyColor, ox, oz, _sky.layer1Repeats, false);
  cloud(_sky.layer2Height, _sky.blobbyColor, halfX, halfX, _sky.layer2Repeats, false);
  cloud(_sky.layer3Height, _sky.blobbyColor, oz, ox, _sky.layer3Repeats, false);
  cloud(_sky.gridHeight, _sky.flatColor, ox, oz, _sky.layer1Repeats, true);
  cloud(_sky.layer2Height, _sky.flatColor, halfX, halfX, _sky.layer2Repeats, true);
  return layers;
}

} // namespace Neuron
