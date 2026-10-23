#pragma once

#include "RegionTerrain.h"
#include "RelationMessages.h"
#include "ReplicatedState.h"
#include "StateReplica.h"
#include "TuningMvp.h"
#include "TuningPresentation.h"
#include "Vec3.h"
#include "WorldScene.h"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace Species
{

/// The territories and border areas of the settlement view (MVP-084; M2 task T13, ADR-143). Each settlement's territory
/// is a glowing line of fx.beam_long in its Cladan's colour, along the edge of the cells the replica holds of it. Each
/// valley border area (WLD-062) has its halfway line and its edge marked with dotted spots, in a neutral colour while
/// no holder is known. From M3 task T11 (ADR-166) the edge of each half takes its holder's colour from the valley's
/// status, or its claimant's while a claim on it stands (MVP-053). Its looks are provisional presentation keys (O90,
/// DEC-1002). Frame thread only.
class TerritoryLook
{
public:
  // _presentation and _tuning outlive the look.
  TerritoryLook(const TuningPresentation& _presentation, const TuningMvp& _tuning) noexcept;

  // Places the border areas' spots for a new terrain, and builds again the line of each settlement of _latest whose
  // territory chunks changed. A settlement that left the replica loses its line.
  void Update(const StateReplica::View& _latest, const RegionTerrain& _terrain);
  // A new world: every line and spot dropped.
  void Clear() noexcept;
  // ADR-166: the areas' holders and the claims of the latest valley status, or none before the first.
  void SetValley(const std::optional<ValleyStatus>& _valley);

  /// A settlement's territory line: its local site, its Cladan's colour, and the line's pieces, each from a start to an
  /// end at the line's height over the terrain.
  struct Line
  {
    std::uint8_t site = 0;
    std::uint8_t color = 0;
    std::vector<std::array<Neuron::Vec3, 2>> pieces;
  };
  [[nodiscard]] const std::vector<Line>& Lines() const noexcept
  {
    return m_lines;
  }
  /// A border area's spot at its height over the terrain: on its pair's halfway line, or on the edge of one half, which
  /// its area names (VALLEY_PAIRS index × 2 + side).
  struct BorderSpot
  {
    Neuron::Vec3 position;
    std::optional<std::uint8_t> area; // none on the halfway line
  };
  [[nodiscard]] const std::vector<BorderSpot>& BorderSpots() const noexcept
  {
    return m_spots;
  }
  // ADR-166: the colour of a spot: the claimant's while a claim stands on its half, else its holder's, else the neutral
  // mark's; at the mark's alpha.
  [[nodiscard]] std::array<float, 4> SpotColor(const BorderSpot& _spot) const noexcept;

  // Pass 10a: a segment of fx.beam_long for each piece of each line, in its Cladan's colour at the line's alpha, and,
  // while the replica holds a settlement, a camera-facing spot of fx.particle_dot for each border spot.
  void AppendBatches(std::vector<Neuron::QuadBatch>& _batches) const;

private:
  void PlaceBorderSpots(const RegionTerrain& _terrain);
  [[nodiscard]] Line BuildLine(std::uint8_t _site, const std::vector<ReplicatedCells>& _chunks, const RegionTerrain& _terrain) const;

  const TuningPresentation* m_presentation;
  const TuningMvp* m_tuning;
  const RegionTerrain* m_terrain = nullptr;
  std::vector<Line> m_lines;                          // in site order
  std::vector<std::vector<ReplicatedCells>> m_chunks; // the chunks each line was built from, as m_lines
  std::vector<BorderSpot> m_spots;
  std::array<std::optional<std::uint8_t>, VALLEY_AREAS> m_holders{};   // ADR-166: each area's holder
  std::array<std::optional<std::uint8_t>, VALLEY_AREAS> m_claimants{}; // and the first claimant of a claim on it
};

} // namespace Species
