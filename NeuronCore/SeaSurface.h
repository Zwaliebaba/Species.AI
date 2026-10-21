#pragma once

#include "TerrainField.h"
#include "TerrainSurface.h"
#include "Vec3.h"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace Neuron
{

// TER-204: one travelling sinusoid of the wave tables, amplitude × sin(perWu × distance + perSecond × time).
struct WaveTerm
{
  double perWu = 0.0;
  double perSecond = 0.0;
  double amplitude = 0.0;
};

inline constexpr std::size_t BLUR_TAP_COUNT = 11; // TER-197: 11 taps centred on tap 5

/// The presentation values the sea is built and moved with (RND-140 to RND-146, TER-195 to TER-205), from the client's
/// presentation record (ADR-116).
struct SeaLook
{
  double seaLevel = 0.0;                                                                              // ref.sim.physics.sea_level, wu
  double flatHeight = -9.0;                                                                           // rnd.sea.flat_height, wu
  std::uint32_t maskSize = 128;                                                                       // ter.sea.mask_size, pixels per side
  std::uint32_t maskLandOffset = 32;                                                                  // ter.sea.mask_land_offset, pixels
  std::array<double, BLUR_TAP_COUNT> blurTaps{0.2, 0.3, 0.4, 0.5, 0.8, 1.0, 0.8, 0.5, 0.4, 0.3, 0.2}; // ter.sea.blur_taps
  double blurNormalizer = 5.4;                                                                        // ter.sea.blur_normalizer
  double blurSkipTolerance = 1e-6;                                                                    // ter.sea.blur_skip_tolerance
  double maskGain = 855.0;                                                                            // ter.sea.mask_gain
  double maskMax = 255.0;                                                                             // ter.sea.mask_max
  double depthOutside = 1.0;                                                                          // ter.sea.depth_outside
  std::uint32_t flatTileRatio = 4;                                                                    // ter.sea.flat_tile_ratio
  double cellDivisor = 100.0;                                                                         // ter.sea.cell_divisor
  double vertexLandLimit = 4.0;                                                                       // ter.sea.vertex_land_limit, wu
  double vertexDepthLimit = 0.999;                                                                    // ter.sea.vertex_depth_limit
  double shoreNoise = 0.25;                                                                           // ter.sea.wave.shore_noise
  double shoreBrightening = 250.0;                                                                    // ter.sea.wave.shore_brightening
  std::uint32_t tableMargin = 2;                                                                      // ter.sea.wave.table_margin
  double amplitudeScale = 7.0;                                                                        // ter.sea.wave.amplitude_scale, wu
  std::array<WaveTerm, 2> xTerms{WaveTerm{0.01, 0.65, 1.2}, WaveTerm{0.03, 1.5, 0.9}};                // ter.sea.wave.x_terms
  std::array<WaveTerm, 2> zTerms{WaveTerm{0.02, 0.75, 0.9}, WaveTerm{0.03, 1.85, 0.65}};              // ter.sea.wave.z_terms
  double indexBias = 0.1;                                                                             // ter.sea.wave.index_bias, cells
  double brightnessScale = 4.0;                                                                       // ter.sea.wave.brightness_scale
  double mixOther = 0.7; // ter.sea.wave.color_mix: the other vertex's brightness
  double mixOwn = 0.3;
  std::uint32_t detail = 1; // the sea-detail level of the wave mesh (RND-448); 0 builds none
};

/// One vertex pair of a wave mesh (TER-201, TER-202): v1 at the pair's position and v2 one water cell along +z, both in
/// plane coordinates, with each vertex's depth and shore noise.
struct WavePair
{
  std::uint32_t xb = 0;
  std::uint32_t zb = 0;
  float depth1 = 0.0f;
  float depth2 = 0.0f;
  float noise1 = 0.0f;
  float noise2 = 0.0f;
  bool joinsPrevious = false; // the previous emitted pair is its neighbour at xb - 1 of the same row (ADR-115)
};

/// A vertex of the sea's two layers: a position in plane coordinates and its colour as encoded, red lowest.
struct SeaVertex
{
  std::array<float, 3> position;
  std::uint32_t color = 0;
};
static_assert(sizeof(SeaVertex) == 16);

/// The sea of a region (RND-140, TER-195), its rules restated about the region's origin (ADR-115): the plane runs
/// from planeOrigin, half a region before the region's origin on both axes, over twice the region's side.
struct SeaSurface
{
  Vec3 planeOrigin; // wu; y is 0
  double planeSide = 0.0;
  std::uint32_t maskSize = 0;
  std::vector<std::uint8_t> shoreMask; // TER-197: grey levels, image row 0 holding mask row maskSize - 1
  std::vector<SeaVertex> flatVertices; // RND-141: two triangles per drawn tile, in plane coordinates, white
  double waterCell = 0.0;              // TER-200, wu
  std::uint32_t tableX = 0;            // TER-204
  std::uint32_t tableZ = 0;
  std::vector<WavePair> pairs; // in build order
  std::uint32_t waveTriangles = 0;
  std::vector<std::uint32_t> waveColors; // TER-041, TER-203
};

// TER-195 to TER-202 on a region's field: the masks, the flat tiles, and the wave mesh of the look's sea-detail level,
// whose shore noise draws from _shoreNoise, the copy of the colour chain at its end (TER-186). _gradient is the
// shore-wave gradient and _gradientRow the row of RND-144, counted from the bottom as viewed (TER-041).
[[nodiscard]] SeaSurface BuildSeaSurface(const TerrainField& _field, const SeaLook& _look, TileRandom _shoreNoise,
                                         const CpuImage& _gradient, std::uint32_t _gradientRow);

// TER-204 and TER-205 at the smooth presentation time _seconds (RND-145): the wave mesh's triangles, three vertices
// each, every corner with its triangle's facet colour. _out is resized to the mesh.
void EvaluateWaves(const SeaSurface& _sea, const SeaLook& _look, double _seconds, std::vector<SeaVertex>& _out);

// TER-041: one row of an image, counted from the bottom as viewed, as opaque colours.
[[nodiscard]] std::vector<std::uint32_t> ImageRowColors(const CpuImage& _image, std::uint32_t _rowFromBottom);

} // namespace Neuron
