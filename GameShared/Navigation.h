#pragma once

#include "TerrainField.h"
#include "TuningMvp.h"
#include "Vec3.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace Species
{

struct RegionLayout;

/// A node of the land route graph (R-SIM-331): its position on the terrain and its check-cliffs flag, which every node
/// the MVP generates holds as the reference's default, true.
struct NavigationNode
{
  Neuron::Vec3 position;
  bool checkCliffs = true;
};

// MVP-017 (ADR-104): the region's nodes, numbered in a stable order from the terrain alone, with no draw: the points of
// the lattice of spacing mvp.nav.node_spacing, at the centres of its cells z outer and x inner, whose terrain is dry and
// whose normal passes the path test's slope (R-SIM-335); then the four site centres in local order, the central resource
// site, and the middle of each valley border area (WLD-062), the nearer site's half first, in the order of the valley
// neighbour pairs 0–1, 2–3, 0–2 and 1–3.
[[nodiscard]] std::vector<NavigationNode> PlaceNavigationNodes(const RegionLayout& _layout, const Neuron::TerrainField& _field,
                                                               const TuningMvp& _tuning);

// R-SIM-335 to R-SIM-337: whether a straight walk is possible. The MVP has no wall and no Barrier Emitter, so the test
// reads the terrain alone and reports no emitter.
[[nodiscard]] bool IsPathClear(const Neuron::Vec3& _from, const Neuron::Vec3& _to, bool _evaluateCliffs, bool _permitSteepDownhill,
                               const Neuron::TerrainField& _field, const TuningMvp& _tuning);
// R-SIM-338: the walking cost of a straight segment, or −1 when it is blocked by water.
[[nodiscard]] double WalkCost(const Neuron::Vec3& _from, const Neuron::Vec3& _to, const Neuron::TerrainField& _field,
                              const TuningMvp& _tuning);
// R-SIM-334: the sum of the legs' walking costs, or the first negative one.
[[nodiscard]] double WalkingDistance(const std::vector<Neuron::Vec3>& _points, const Neuron::TerrainField& _field,
                                     const TuningMvp& _tuning);

/// The land route graph of a region (R-SIM-330 to R-SIM-352, ADR-104): its nodes and each node's route table, built
/// once from the terrain, and the path query over them. It holds no route: the region's store keeps the routes the
/// simulation builds.
class LandRouteGraph
{
public:
  // R-SIM-332: to reach target, go to next first; the remaining cost from here is total.
  struct Hop
  {
    std::uint32_t target = 0;
    std::uint32_t next = 0;
    double total = 0.0;
  };

  // R-SIM-339 to R-SIM-341: the edges of every ordered pair within the link range that the symmetric memo finds
  // walkable, then distance-vector relaxation to convergence.
  [[nodiscard]] static LandRouteGraph Build(std::vector<NavigationNode> _nodes, const Neuron::TerrainField& _field,
                                            const TuningMvp& _tuning);

  [[nodiscard]] const std::vector<NavigationNode>& Nodes() const noexcept
  {
    return m_nodes;
  }
  // The node's route table, in the order its entries were made.
  [[nodiscard]] const std::vector<Hop>& TableOf(std::uint32_t _node) const;
  // The entry of _node for _target, or none.
  [[nodiscard]] const Hop* HopOf(std::uint32_t _node, std::uint32_t _target) const;

  // R-SIM-346: the walkable node nearest _position, or the nearest when none is walkable; none with no node.
  [[nodiscard]] std::optional<std::uint32_t> NearestNode(const Neuron::Vec3& _position, const Neuron::TerrainField& _field,
                                                         const TuningMvp& _tuning) const;

  // R-SIM-345 to R-SIM-351: a route's points from _from to _to, which the caller stores (R-SIM-352).
  [[nodiscard]] std::vector<Neuron::Vec3> BuildRoute(const Neuron::Vec3& _from, const Neuron::Vec3& _to, bool _directRoute,
                                                     bool _permitClimbing, bool _noRounding, const Neuron::TerrainField& _field,
                                                     const TuningMvp& _tuning) const;

private:
  std::vector<NavigationNode> m_nodes;
  std::vector<std::vector<Hop>> m_tables;
  std::vector<std::int32_t> m_slots; // node × node: the index of the entry for a target, or −1
};

} // namespace Species
