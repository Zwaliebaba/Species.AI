#pragma once

#include "BitReader.h"
#include "BitWriter.h"
#include "RegionTerrain.h"
#include "Settlement.h"
#include "Vec3.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace Species
{

class RegionSimulation;

/// A set of cells of a region's square (ADR-134): a settlement's territory, in cells of mvp.territory.cell_size (MVP-026,
/// WLD-060), or its seen layer, in cells of mvp.vision.cell_size (MVP-016). A cell is held when its centre is. Both are
/// derived from the state, so neither the hash nor the snapshot holds one.
class CellRaster
{
public:
  CellRaster() = default;
  CellRaster(double _originX, double _originZ, double _side, double _cellSize);

  [[nodiscard]] std::int32_t CellsPerAxis() const noexcept
  {
    return m_cellsPerAxis;
  }
  [[nodiscard]] double CellSize() const noexcept
  {
    return m_cellSize;
  }
  // The cells held.
  [[nodiscard]] std::size_t Count() const noexcept
  {
    return m_count;
  }
  // The centre of a cell, at height 0.
  [[nodiscard]] Neuron::Vec3 CellCenter(std::int32_t _x, std::int32_t _z) const noexcept;
  [[nodiscard]] bool HoldsCell(std::int32_t _x, std::int32_t _z) const noexcept;
  void Hold(std::int32_t _x, std::int32_t _z);
  // The cell that holds the point, which is held; false off the raster.
  [[nodiscard]] bool Holds(double _x, double _z) const noexcept;
  // MVP-032: the disc of _radius about (_x, _z) lies in the territory: the cell of its centre and every cell whose
  // centre lies within it are held.
  [[nodiscard]] bool HoldsDisc(double _x, double _z, double _radius) const noexcept;
  // MVP-016: every cell whose centre lies within _radius of (_x, _z) becomes held.
  void HoldDisc(double _x, double _z, double _radius);
  // ADR-139: the cells of the chunk (_chunkX, _chunkZ) of CELL_CHUNK_SIDE cells a side, bit (z × CELL_CHUNK_SIDE + x)
  // for the cell x across and z up from its corner; and the chunks that hold a cell, as the first and last chunk on
  // each axis, or none while no cell is held.
  [[nodiscard]] std::uint64_t ChunkCells(std::int32_t _chunkX, std::int32_t _chunkZ) const noexcept;
  struct ChunkBounds
  {
    std::int32_t firstX = 0;
    std::int32_t firstZ = 0;
    std::int32_t lastX = 0;
    std::int32_t lastZ = 0;
  };
  [[nodiscard]] std::optional<ChunkBounds> HeldChunks() const noexcept;

private:
  [[nodiscard]] std::int32_t CellOf(double _coordinate, double _origin) const noexcept;

  double m_originX = 0.0;
  double m_originZ = 0.0;
  double m_cellSize = 0.0;
  std::int32_t m_cellsPerAxis = 0;
  std::vector<std::uint8_t> m_cells;
  std::size_t m_count = 0;
  // The held cells' bounds, which only grow: no cell is ever released.
  std::int32_t m_minX = 0;
  std::int32_t m_minZ = 0;
  std::int32_t m_maxX = -1;
  std::int32_t m_maxZ = -1;
};

// WLD-060 and WLD-061 for a point of the settlement at _site, before its radius: inside the coast bands, within
// wld.territory.max_radius of its site's centre and nearer to it than to either valley neighbour's, so in its home cell
// or its own valley border areas; at least wld.territory.site_clearance outside the core of every other site; at least
// wld.resource.wild_clearance outside every central resource site's footprint; and on ground above the shore height.
// From M3 (ADR-160) a half of a valley border area that a claim moved is its new holder's and no longer its own
// side's (WLD-086).
[[nodiscard]] bool WithinTerritoryBounds(const RegionSimulation& _region, std::uint8_t _site, double _x, double _z);

// WLD-062 (ADR-160): the point lies in the half of a valley border area that _area names (CLAIM_AREA_CENTRAL names
// none): inside the coast bands, within wld.territory.max_radius of both sites of its pair, and no nearer the other
// site than its own.
[[nodiscard]] bool InBorderHalf(const RegionSimulation& _region, std::uint8_t _area, double _x, double _z);

// MVP-027 and WLD-064 (ADR-160): the territories of the settlements at _first and _second meet: a cell of one and a cell
// of the other share a side or a corner. Only the lens of their sites' discs can hold such cells.
[[nodiscard]] bool TerritoriesMeet(const RegionSimulation& _region, std::uint8_t _first, std::uint8_t _second);

// MVP-027 and WLD-064 (ADR-134): the territory of the settlement at _site reaches the halfway line of the valley border
// pair VALLEY_PAIRS[_pair]: it holds a cell whose centre lies within a cell's side of the line, inside the lens of the
// two sites' discs of wld.territory.max_radius.
[[nodiscard]] bool ReachesHalfwayLine(const RegionSimulation& _region, std::uint8_t _site, std::size_t _pair);

// WLD-066: what a territory event records; a value of the saved state that is never renumbered. A border area that
// returns to its side when its settlement leaves the map is B5's: no MVP settlement leaves.
enum class ContactEventKind : std::uint8_t
{
  Contact = 0,  // MVP-027: two settlements came into contact at their valley border pair's halfway line
  AreaMoved = 1 // MVP-053 (ADR-160): a claim battle moved a half of their valley border area from first to second
};

/// A territory event of the region (WLD-066): its world time as the tick, its kind, its two settlements by local site
/// and the valley border pair concerned, by its index in VALLEY_PAIRS.
struct ContactEvent
{
  std::uint64_t tick = 0;
  ContactEventKind kind = ContactEventKind::Contact;
  std::uint8_t first = 0;
  std::uint8_t second = 0;
  std::uint8_t pair = 0;
  std::uint8_t area = 0; // the half that moved, for AreaMoved (ADR-160)

  friend constexpr bool operator==(const ContactEvent& _a, const ContactEvent& _b) noexcept = default;
};

// The event canonically (ADR-081 item 7), and the values the writer wrote into a snapshot of _stateVersion; a region
// before version 12 records contact alone, in a kind of no bits (ADR-160).
void WriteContactEvent(Neuron::BitWriter& _writer, const ContactEvent& _event);
[[nodiscard]] bool ReadContactEvent(Neuron::BitReader& _reader, ContactEvent& _event, std::uint32_t _stateVersion);

// MVP-026 (ADR-134): the settlement's territory, the cells within its radius of its Wellspring and within the bounds,
// connected to the Wellspring's cell by their sides, so that it is one connected area (WLD-060).
[[nodiscard]] CellRaster ComputeTerritory(const RegionSimulation& _region, const Settlement& _settlement);

} // namespace Species
