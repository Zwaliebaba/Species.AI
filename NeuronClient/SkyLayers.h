#pragma once

#include "ShaderLayouts.h"
#include "Vec3.h"
#include "WorldScene.h"

#include <cstdint>
#include <vector>

namespace Neuron
{

// RND-151 to RND-154 for one frame, relative to the camera (ADR-115): the grid lines parallel to z, then those parallel
// to x, then the blobby and the flat cloud layers. _beam and _cloud are the heap indices of fx.beam_short and
// fx.sky_cloud.
[[nodiscard]] std::vector<SkyLayerInstance> BuildSkyLayers(const SkyLook& _sky, const Vec3& _camera, double _skyTicks, std::uint32_t _beam,
                                                           std::uint32_t _cloud);

} // namespace Neuron
