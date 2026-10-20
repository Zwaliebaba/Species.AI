#pragma once

#include "TerrainField.h"
#include "Vec3.h"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace Neuron
{

/// The presentation values the terrain mesh is built with (RND-133, TER-181, TER-185), from the client's presentation
/// record (ADR-116).
struct TerrainLook
{
  double sinkThreshold = 0.3;         // rnd.terrain.sink_threshold, wu
  double sinkHeight = -10.0;          // rnd.terrain.sink_height, wu
  std::uint32_t rampSize = 64;        // ter.color.ramp_size, texels
  double centerFactor = 0.33333;      // ter.color.center_factor
  double slopeExponent = 0.4;         // ter.color.slope_exponent
  double jitterNumerator = 0.45;      // ter.color.jitter_numerator
  double jitterOffset = 2.0;          // ter.color.jitter_offset, wu
  std::uint32_t presentationSeed = 1; // ref.sim.match.presentation_seed
  double chunkSide = 1024.0;          // wld.terrain.chunk_side, wu: the side of a culled chunk (ADR-115)
};

/// An image read on the CPU as its texture stores it (AST-072's CPU table): R8G8B8A8 texels, rows top-down. The land
/// colour ramp (TER-040) and the shore-wave gradient (TER-041) are read this way.
struct CpuImage
{
  std::uint32_t width = 0;
  std::uint32_t height = 0;
  std::span<const std::uint8_t> texels; // width × height × 4 bytes
};

/// One corner of a terrain triangle as the terrain pass reads it (ADR-115): every corner of a triangle carries its
/// facet colour and facet normal (ADR-020, RND-276), so no shader depends on the provoking vertex.
struct TerrainVertex
{
  std::array<float, 3> position; // wu, relative to the chunk's origin
  std::uint32_t color = 0;       // the ramp texel as encoded, red in the lowest byte (ADR-013)
  std::array<float, 3> normal;
  std::array<float, 2> grid; // cell coordinates from the region's origin: the wire grid's texture coordinates (RND-135)
};
static_assert(sizeof(TerrainVertex) == 36);

/// A square of the mesh that the renderer culls as one (RND-138): its triangles, its origin and its bounds.
struct TerrainChunk
{
  Vec3 origin; // wu; y is 0
  Vec3 boundsMin;
  Vec3 boundsMax;
  std::vector<TerrainVertex> vertices; // three per triangle
};

struct TerrainSurface
{
  std::vector<TerrainChunk> chunks; // the chunks that hold a triangle, x inner and z outer
  std::uint32_t triangles = 0;
  // TER-186: the colour pass's generator at its end, which each sea-detail level's shore noise copies.
  TileRandom colorChainEnd;
};

// RND-132 to RND-134 and TER-180 to TER-185 on a region's field: the needed vertex pairs, the triangles between
// consecutive needed pairs of a row, the sink, each facet's normal and its colour from the ramp, in build order. A
// pair at the end of a row never joins the first of the next (ADR-115): a region has sea along its border.
[[nodiscard]] TerrainSurface BuildTerrainSurface(const TerrainField& _field, const CpuImage& _ramp, const TerrainLook& _look);

// TER-181 for one triangle, given in strip order with its facet normal's y: the colour, and the generator moved on.
[[nodiscard]] std::uint32_t FacetColor(const std::array<Vec3, 3>& _corners, float _normalY, float _highest, const CpuImage& _ramp,
                                       const TerrainLook& _look, TileRandom& _colorVariation) noexcept;

// TER-076: the highest sample, scanning from At(0, 0), z outer and x inner over the row count.
[[nodiscard]] double HighestSample(const SurfaceGrid<double>& _grid) noexcept;

} // namespace Neuron
