#pragma once

#include "Navigation.h"
#include "Sha256.h"
#include "TerrainField.h"
#include "TuningMvp.h"
#include "Vec3.h"

#include <array>
#include <cstdint>
#include <expected>
#include <vector>

namespace Species
{

// The purposes of world generation's draws (WLD-161): one enumerator per draw-site family, never renumbered.
enum class WorldDrawPurpose : std::uint64_t
{
  ValleyOffset = 1,   // WLD-040: the site pattern's offset, index 0 for x and 1 for z
  ValleyRotation = 2, // WLD-040: the site pattern's rotation
  TileSeed = 3,       // WLD-162: a tile's seed, index 0 for the base tile and 1 + k for hill k
  HillCorner = 4,     // WLD-162: the corner of hill k
  HillHeight = 5      // WLD-162: the desired height of hill k
};

// ADR-082 item 4: SplitMix64's output function chained over the public seed and the keys domain 2, purpose, region,
// attempt and index, as A1's model computes it; a world's shape is public, so its draws need no secret.
[[nodiscard]] std::uint64_t WorldDraw(std::uint64_t _seed, WorldDrawPurpose _purpose, std::uint64_t _region, std::uint64_t _attempt,
                                      std::uint64_t _index) noexcept;
// The same draw as a value in [0, 1): its top 53 bits × 2⁻⁵³.
[[nodiscard]] double WorldUnit(std::uint64_t _seed, WorldDrawPurpose _purpose, std::uint64_t _region, std::uint64_t _attempt,
                               std::uint64_t _index) noexcept;

// The checks of WLD-163 that the MVP's one region runs (MVP-011): it has no crossing, so check 2 has nothing to check;
// check 7 waits for M2's yields.
enum class TerrainCheck : std::uint8_t
{
  Channels = 1,
  Plateaus = 3,
  Rims = 4,
  DrySites = 5,
  Reachability = 6,
  Routes = 8
};

/// Where a region's valley stands (WLD-022, WLD-040): its square, the valley's centre, the rotation of its site pattern,
/// its four site centres in local order (0 at low x and low z, 1 at high x, 2 at high z, 3 at both), and the levels its
/// first pass read (WLD-162).
struct RegionLayout
{
  std::uint16_t index = 0;
  double originX = 0.0;
  double originZ = 0.0;
  double side = 0.0;
  Neuron::Vec3 valleyCenter;
  double rotationRadians = 0.0;
  std::array<Neuron::Vec3, 4> sites{};
  std::array<double, 4> siteFloors{};
  double centerFloor = 0.0;
};

/// One region's terrain (WLD-160 to WLD-166): its layout, the records its generation placed, the generated field and
/// the digests of its baked chunks. The server and the client build it from the same seed with the same library and
/// compare the digest at connection (MVP-011, TER-009 amendment).
struct RegionTerrain
{
  std::uint64_t seed = 0;    // MVP-005: the seed shown, which gives this terrain again
  std::uint32_t attempt = 0; // WLD-163: the attempt of that seed whose draws passed every check
  RegionLayout layout;
  std::vector<Neuron::TerrainTile> tiles;
  std::vector<Neuron::TerrainFlattenArea> areas;
  Neuron::TerrainField field;
  std::vector<Neuron::Sha256Digest> chunkDigests; // WLD-166: TER-252 per chunk, z outer and x inner
  Neuron::Sha256Digest digest{};                  // SHA-256 of the chunk digests in that order
  LandRouteGraph routes;                          // MVP-017: the region's nodes and their route tables (ADR-104)
};

// One attempt of WLD-160 to WLD-162 for the MVP's one region: its layout, its tiles, the first pass and the levels it
// reads, the flatten areas, the second pass, the chunk digests, and the navigation nodes and land route graph of MVP-017.
// It runs no check.
[[nodiscard]] RegionTerrain BuildRegionTerrain(std::uint64_t _seed, std::uint32_t _attempt, const TuningMvp& _tuning);

// WLD-163 checks 1 and 3 to 6 on the region's samples and check 8 on its routes: the checks that fail, in order; none
// when it passes.
[[nodiscard]] std::vector<TerrainCheck> CheckRegionTerrain(const RegionTerrain& _terrain, const TuningMvp& _tuning);

struct TerrainFailure
{
  std::uint64_t seed = 0;
  std::vector<TerrainCheck> checks; // those of the last attempt
};

// MVP-011 and WLD-163: the region of the first attempt that passes every check, trying wld.generation.max_attempts
// attempts of a seed and then the next seed, for as many seeds; the region's seed is the one that passed.
[[nodiscard]] std::expected<RegionTerrain, TerrainFailure> GenerateRegionTerrain(std::uint64_t _seed, const TuningMvp& _tuning);

} // namespace Species
