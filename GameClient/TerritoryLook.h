#pragma once

#include "RegionTerrain.h"
#include "ReplicatedState.h"
#include "StateReplica.h"
#include "TuningMvp.h"
#include "TuningPresentation.h"
#include "Vec3.h"
#include "WorldScene.h"

#include <array>
#include <cstdint>
#include <vector>

namespace Species
{

/// The territories and border areas of the settlement view (MVP-084; M2 task T13, ADR-143). Each settlement's territory
/// is a glowing line of fx.beam_long in its Cladan's colour, along the edge of the cells the replica holds of it. Each
/// valley border area (WLD-062) has its halfway line and its edge marked with dotted spots, in a neutral colour while
/// no holder is replicated. Its looks are provisional presentation keys (O90, DEC-1002). Frame thread only.
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
  // The border areas' spots, on their halfway lines and their edges, at their height over the terrain.
  [[nodiscard]] const std::vector<Neuron::Vec3>& BorderSpots() const noexcept
  {
    return m_spots;
  }

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
  std::vector<Neuron::Vec3> m_spots;
};

} // namespace Species
