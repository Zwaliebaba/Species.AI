#pragma once

#include "BitReader.h"
#include "BitWriter.h"
#include "StableId.h"

#include <algorithm>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <unordered_set>
#include <vector>

namespace Species
{

// A region's team slots are numbered by a byte (R-SIM-221); a mask names a set of them.
inline constexpr std::size_t TEAM_SLOTS_MAX = 256;
using TeamMask = std::bitset<TEAM_SLOTS_MAX>;

/// An actor's entry in the actor grid (R-SIM-307): its team, its slot among the team's figures and its stable id, which
/// stands for the reference's serial and orders the entries of a cell (R-SIM-310).
struct ActorRef
{
  StableId id = 0;
  std::uint32_t slot = 0;
  std::uint8_t team = 0;

  friend constexpr bool operator==(const ActorRef& _a, const ActorRef& _b) noexcept = default;
};

/// A rectangle of cells with both bounds inclusive; empty for an actor that is registered nowhere.
struct CellRect
{
  std::int32_t x0 = 0;
  std::int32_t z0 = 0;
  std::int32_t x1 = -1;
  std::int32_t z1 = -1;

  [[nodiscard]] constexpr bool Empty() const noexcept
  {
    return x1 < x0 || z1 < z0;
  }
  friend constexpr bool operator==(const CellRect& _a, const CellRect& _b) noexcept = default;
};

/// The actor grid of a region (R-SIM-302 to R-SIM-312, ADR-103): one layer per team slot that has figures, each a
/// row-major array of cells over the region's square. A cell holds its entries in ascending stable id and the capacity
/// of the reference's slot array (R-SIM-307), which only grows: the presence queries test it rather than the entries
/// (R-SIM-312, DEC-094). Every query reports in the order cell x, cell z, team, stable id (R-SIM-310).
class ActorGrid
{
public:
  struct Desc
  {
    double originX = 0.0;
    double originZ = 0.0;
    double worldSize = 0.0;            // the side of the region's square
    double cellSize = 32.0;            // ref.sim.actor_grid.cell_size
    std::int32_t extraCells = 1;       // ref.sim.actor_grid.extra_cells
    double footprintFactor = 0.5;      // ref.sim.actor_grid.footprint_factor
    std::uint32_t initialCapacity = 0; // ref.sim.actor_grid.initial_cell_capacity
    std::uint32_t firstCapacity = 2;   // ref.sim.actor_grid.first_cell_capacity
    std::uint32_t growthFactor = 2;    // ref.sim.actor_grid.cell_growth_factor
  };

  explicit ActorGrid(const Desc& _desc);

  // R-SIM-305: trunc(side ÷ cell size) + the extra cells.
  [[nodiscard]] std::int32_t CellsPerAxis() const noexcept
  {
    return m_cellsPerAxis;
  }
  // R-SIM-308: the single cell of an actor of radius 0, or the cells that overlap its footprint square.
  [[nodiscard]] CellRect FootprintOf(double _x, double _z, double _radius) const noexcept;
  // The cells that overlap the square [_x − _half, _x + _half] × [_z − _half, _z + _half], clamped to the grid.
  [[nodiscard]] CellRect SquareOf(double _x, double _z, double _half) const noexcept;

  // R-SIM-307: an entry in every cell of _cells; a cell with no free slot grows its capacity first.
  void Register(const ActorRef& _ref, const CellRect& _cells);
  // R-SIM-307: every entry of _ref in the cells of _cells, which are the cells it was registered over (DEC-650).
  void Unregister(const ActorRef& _ref, const CellRect& _cells);

  // R-SIM-309: the actors of the teams of _teams strictly within _range of (_x, _z) in the horizontal plane, each once.
  // _resolve gives the figure an entry names, or null for one that no longer resolves.
  template <class Resolve>
  [[nodiscard]] std::vector<ActorRef> InRange(double _x, double _z, double _range, const TeamMask& _teams, Resolve&& _resolve) const
  {
    std::vector<ActorRef> result;
    std::unordered_set<StableId> found;
    const double rangeSquared = _range * _range;
    ForEachCell(_x, _z, _range, _teams,
                [&](const Cell& _cell)
                {
                  for (const ActorRef& entry : _cell.entries)
                  {
                    if (found.contains(entry.id))
                    {
                      continue;
                    }
                    const auto* figure = _resolve(entry);
                    if (figure == nullptr)
                    {
                      continue;
                    }
                    const double dx = figure->position.x - _x;
                    const double dz = figure->position.z - _z;
                    if (dx * dx + dz * dz < rangeSquared)
                    {
                      result.push_back(entry);
                      found.insert(entry.id);
                    }
                  }
                });
    return result;
  }

  // R-SIM-311 and R-SIM-312 item 1: the distinct entries of the teams of _teams in every cell that overlaps the square
  // that bounds the range, with no range test; a dead figure that is still registered counts.
  [[nodiscard]] std::uint32_t CountInCells(double _x, double _z, double _range, const TeamMask& _teams) const;
  // R-SIM-312 item 3: true when a covered cell of a team of _teams has a capacity above 0.
  [[nodiscard]] bool AnyInCells(double _x, double _z, double _range, const TeamMask& _teams) const;

  [[nodiscard]] std::uint32_t CapacityAt(std::uint8_t _team, std::int32_t _cellX, std::int32_t _cellZ) const noexcept;
  [[nodiscard]] std::vector<ActorRef> EntriesAt(std::uint8_t _team, std::int32_t _cellX, std::int32_t _cellZ) const;

  // The capacities, which hold the grid's history (R-SIM-312 item 3): per layer the cells whose capacity is not the
  // initial one, in cell order. The entries are not written: each figure's recorded footprint registers it again.
  void WriteState(Neuron::BitWriter& _writer) const;
  // Into a grid with no layers; false when the values do not read whole or name a cell beyond the grid.
  [[nodiscard]] bool ReadState(Neuron::BitReader& _reader);

private:
  struct Cell
  {
    std::vector<ActorRef> entries; // ascending stable id
    std::uint32_t capacity = 0;
  };
  using Layer = std::vector<Cell>;

  [[nodiscard]] std::int32_t ClampedCellOf(double _coordinate, double _origin) const noexcept;
  [[nodiscard]] std::size_t IndexOf(std::int32_t _cellX, std::int32_t _cellZ) const noexcept
  {
    return static_cast<std::size_t>(_cellZ) * static_cast<std::size_t>(m_cellsPerAxis) + static_cast<std::size_t>(_cellX);
  }
  Layer& LayerOf(std::uint8_t _team);

  // Each covered cell in the order cell x ascending, then cell z ascending, and in each the layers of _teams in index
  // order (R-SIM-309, R-SIM-310).
  template <class Fn> void ForEachCell(double _x, double _z, double _range, const TeamMask& _teams, Fn&& _visit) const
  {
    const CellRect square = SquareOf(_x, _z, _range);
    for (std::int32_t cellX = square.x0; cellX <= square.x1; ++cellX)
    {
      for (std::int32_t cellZ = square.z0; cellZ <= square.z1; ++cellZ)
      {
        const std::size_t layers = std::min(m_layers.size(), _teams.size());
        for (std::size_t team = 0; team < layers; ++team)
        {
          if (_teams.test(team) && !m_layers[team].empty())
          {
            _visit(m_layers[team][IndexOf(cellX, cellZ)]);
          }
        }
      }
    }
  }

  Desc m_desc;
  std::int32_t m_cellsPerAxis;
  std::vector<Layer> m_layers; // by team; a team with no figure registered yet has an empty layer
};

} // namespace Species
