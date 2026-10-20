#include "pch.h"

#include "Check.h"
#include "Navigation.h"
#include "RegionTerrain.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>
#include <utility>

namespace Species
{

namespace
{

// The perpendicular of R-SIM-347: (_from − _to) × up, horizontal.
Neuron::Vec3 SideOf(const Neuron::Vec3& _from, const Neuron::Vec3& _to, double _offset) noexcept
{
  const Neuron::Vec3 along = _from - _to;
  return Neuron::WithLength({-along.z, 0.0, along.x}, _offset);
}

// R-SIM-347's three lanes: the segment and the same segment offset to each side; clear only when all three are.
bool CorridorClear(const Neuron::Vec3& _from, const Neuron::Vec3& _to, bool _permitSteepDownhill, const Neuron::TerrainField& _field,
                   const TuningMvp& _tuning)
{
  const Neuron::Vec3 side = SideOf(_from, _to, _tuning.ref.sim.route.corridorOffset);
  return IsPathClear(_from, _to, true, _permitSteepDownhill, _field, _tuning) &&
         IsPathClear(_from + side, _to + side, true, _permitSteepDownhill, _field, _tuning) &&
         IsPathClear(_from - side, _to - side, true, _permitSteepDownhill, _field, _tuning);
}

Neuron::Vec3 OnTerrain(double _x, double _z, const Neuron::TerrainField& _field) noexcept
{
  return {_x, _field.Height(_x, _z), _z};
}

} // namespace

std::vector<NavigationNode> PlaceNavigationNodes(const RegionLayout& _layout, const Neuron::TerrainField& _field, const TuningMvp& _tuning)
{
  std::vector<NavigationNode> nodes;
  const double spacing = _tuning.mvp.nav.nodeSpacing;
  const auto perAxis = static_cast<std::int64_t>(std::floor(_layout.side / spacing));
  for (std::int64_t j = 0; j < perAxis; ++j)
  {
    for (std::int64_t i = 0; i < perAxis; ++i)
    {
      const double x = _layout.originX + (static_cast<double>(i) + 0.5) * spacing;
      const double z = _layout.originZ + (static_cast<double>(j) + 0.5) * spacing;
      if (_field.Height(x, z) > _tuning.ref.sim.physics.seaLevel && _field.Normal(x, z).y >= _tuning.ref.sim.walk.minNormalY)
      {
        nodes.push_back({OnTerrain(x, z, _field), true});
      }
    }
  }
  for (const Neuron::Vec3& site : _layout.sites)
  {
    nodes.push_back({OnTerrain(site.x, site.z, _field), true});
  }
  nodes.push_back({OnTerrain(_layout.valleyCenter.x, _layout.valleyCenter.z, _field), true});
  // WLD-062: the lens of the two sites' largest territories on the line between them, from d − R to R from the first
  // site, split halfway; each half's node stands at the middle of its extent on that line.
  const double reach = _tuning.wld.territory.maxRadius;
  for (const auto& [a, b] : std::array<std::pair<std::size_t, std::size_t>, 4>{{{0, 1}, {2, 3}, {0, 2}, {1, 3}}})
  {
    const Neuron::Vec3& first = _layout.sites[a];
    const Neuron::Vec3& second = _layout.sites[b];
    const double distance = Neuron::HorizontalDistance(first, second);
    const Neuron::Vec3 direction = Neuron::WithLength({second.x - first.x, 0.0, second.z - first.z}, 1.0);
    for (const double along : {((distance - reach) + distance / 2.0) / 2.0, (distance / 2.0 + reach) / 2.0})
    {
      nodes.push_back({OnTerrain(first.x + direction.x * along, first.z + direction.z * along, _field), true});
    }
  }
  return nodes;
}

bool IsPathClear(const Neuron::Vec3& _from, const Neuron::Vec3& _to, bool _evaluateCliffs, bool _permitSteepDownhill,
                 const Neuron::TerrainField& _field, const TuningMvp& _tuning)
{
  const auto& walk = _tuning.ref.sim.walk;
  const double seaLevel = _tuning.ref.sim.physics.seaLevel;
  if (_from.y <= seaLevel || _to.y <= seaLevel)
  {
    return false; // the heights passed in, not the terrain's
  }
  const auto steps = static_cast<std::int64_t>(Neuron::Length(_to - _from) / walk.step);
  if (steps == 0)
  {
    return true; // R-SIM-337: no step vector is computed
  }
  const Neuron::Vec3 diff = (_to - _from) * (1.0 / static_cast<double>(steps));
  Neuron::Vec3 position = _from;
  double wet = 0.0;
  for (std::int64_t n = 0; n < steps; ++n)
  {
    position.y = _field.Height(position.x, position.z);
    if (position.y <= seaLevel)
    {
      wet += walk.step;
    }
    if (wet >= walk.wetLimit)
    {
      return false;
    }
    if (_evaluateCliffs)
    {
      // R-SIM-336: the gradient is measured from the start point, never from the previous sample.
      const double gradient = (position.y - _from.y) / walk.step;
      if (gradient > walk.maxClimbGradient)
      {
        return false;
      }
      if ((!_permitSteepDownhill || gradient > 0.0) && _field.Normal(position.x, position.z).y < walk.minNormalY)
      {
        return false;
      }
    }
    position = position + diff;
  }
  return true;
}

double WalkCost(const Neuron::Vec3& _from, const Neuron::Vec3& _to, const Neuron::TerrainField& _field, const TuningMvp& _tuning)
{
  const auto& walk = _tuning.ref.sim.walk;
  const double seaLevel = _tuning.ref.sim.physics.seaLevel;
  const auto steps = static_cast<std::int64_t>(Neuron::Length(_to - _from) / walk.costStep);
  double cost = 0.0;
  double wet = 0.0;
  Neuron::Vec3 previous = _from;
  for (std::int64_t n = 1; n <= steps; ++n)
  {
    Neuron::Vec3 point = _from + (_to - _from) * static_cast<double>(n) * (1.0 / static_cast<double>(steps));
    point.y = _field.Height(point.x, point.z);
    if (point.y <= seaLevel)
    {
      wet += walk.costStep;
    }
    if (wet >= walk.costWetLimit)
    {
      return -1.0;
    }
    // Rise over run between consecutive samples; the uphill penalty only.
    const double run = Neuron::HorizontalDistance(point, previous);
    const double gradient = run > 0.0 ? (point.y - previous.y) / run : 0.0;
    cost += Neuron::Length(point - previous) * (1.0 + std::max(0.0, walk.uphillFactor * gradient));
    previous = point;
  }
  return cost;
}

double WalkingDistance(const std::vector<Neuron::Vec3>& _points, const Neuron::TerrainField& _field, const TuningMvp& _tuning)
{
  double total = 0.0;
  for (std::size_t n = 1; n < _points.size(); ++n)
  {
    const double leg = WalkCost(_points[n - 1], _points[n], _field, _tuning);
    if (leg < 0.0)
    {
      return leg;
    }
    total += leg;
  }
  return total;
}

LandRouteGraph LandRouteGraph::Build(std::vector<NavigationNode> _nodes, const Neuron::TerrainField& _field, const TuningMvp& _tuning)
{
  LandRouteGraph graph;
  graph.m_nodes = std::move(_nodes);
  const auto count = static_cast<std::uint32_t>(graph.m_nodes.size());
  const std::size_t pairs = static_cast<std::size_t>(count) * count;
  graph.m_tables.resize(count);
  graph.m_slots.assign(pairs, -1);
  const auto at = [count](std::uint32_t _i, std::uint32_t _j) { return static_cast<std::size_t>(_i) * count + _j; };

  // R-SIM-340: one answer per unordered pair, the first computed: from the lower-indexed node to the higher.
  std::vector<std::int8_t> walkable(pairs, -1);
  std::vector<double> costs(pairs, 0.0);
  const auto edge = [&](std::uint32_t _i, std::uint32_t _j)
  {
    const std::size_t key = at(std::min(_i, _j), std::max(_i, _j));
    if (walkable[key] < 0)
    {
      const NavigationNode& from = graph.m_nodes[_i];
      const NavigationNode& to = graph.m_nodes[_j];
      const bool clear = IsPathClear(from.position, to.position, from.checkCliffs && to.checkCliffs, false, _field, _tuning);
      walkable[key] = clear ? 1 : 0;
      costs[key] = clear ? WalkCost(from.position, to.position, _field, _tuning) : 0.0;
    }
    return walkable[key] != 0;
  };
  const auto costOf = [&](std::uint32_t _i, std::uint32_t _j) { return costs[at(std::min(_i, _j), std::max(_i, _j))]; };

  // The entries a node has accepted, in order, and how far into each neighbour's record a node has offered it.
  std::vector<std::vector<std::uint32_t>> changes(count);
  std::vector<std::uint32_t> offered(pairs, 0);
  const auto offer = [&](std::uint32_t _node, std::uint32_t _target, std::uint32_t _next, double _total)
  {
    std::int32_t& slot = graph.m_slots[at(_node, _target)];
    std::vector<Hop>& table = graph.m_tables[_node];
    if (slot < 0)
    {
      slot = static_cast<std::int32_t>(table.size());
      table.push_back({_target, _next, _total});
    }
    else if (table[static_cast<std::size_t>(slot)].total > _total)
    {
      table[static_cast<std::size_t>(slot)] = {_target, _next, _total};
    }
    else
    {
      return false;
    }
    changes[_node].push_back(static_cast<std::uint32_t>(slot));
    return true;
  };

  // R-SIM-339 step 2: every ordered pair within the link range, i = j included, i ascending, then j ascending.
  const double range = _tuning.ref.sim.route.linkRange;
  for (std::uint32_t i = 0; i < count; ++i)
  {
    for (std::uint32_t j = 0; j < count; ++j)
    {
      if (Neuron::Length(graph.m_nodes[j].position - graph.m_nodes[i].position) < range && edge(i, j))
      {
        static_cast<void>(offer(i, j, j, costOf(i, j)));
      }
    }
  }

  // R-SIM-341, by the reference's loops and order, offering only what can still be accepted (ADR-104): an entry of the
  // next hop that has not changed since this node last offered it can only tie or lose, so it is skipped; the self
  // entry's offers, 0 + its own totals, always tie. The passes stop when one accepts nothing, as every later pass would.
  std::vector<std::uint32_t> scanned(count, 0);
  std::uint32_t stamp = 0;
  for (std::uint32_t pass = 0; pass < count; ++pass)
  {
    bool accepted = false;
    for (std::uint32_t i = 0; i < count; ++i)
    {
      ++stamp;
      for (std::size_t e = 0; e < graph.m_tables[i].size(); ++e)
      {
        const std::uint32_t hop = graph.m_tables[i][e].next;
        if (hop == i || scanned[hop] == stamp)
        {
          continue;
        }
        scanned[hop] = stamp;
        const double toHop = costOf(i, hop);
        std::uint32_t& from = offered[at(i, hop)];
        const std::vector<std::uint32_t>& record = changes[hop];
        for (; from < record.size(); ++from)
        {
          const Hop& entry = graph.m_tables[hop][record[from]];
          accepted = offer(i, entry.target, hop, toHop + entry.total) || accepted;
        }
      }
    }
    if (!accepted)
    {
      break;
    }
  }
  return graph;
}

const std::vector<LandRouteGraph::Hop>& LandRouteGraph::TableOf(std::uint32_t _node) const
{
  ENGINE_CHECK_INDEX(_node, m_tables.size());
  return m_tables[_node];
}

const LandRouteGraph::Hop* LandRouteGraph::HopOf(std::uint32_t _node, std::uint32_t _target) const
{
  ENGINE_CHECK_INDEX(_node, m_tables.size());
  ENGINE_CHECK_INDEX(_target, m_tables.size());
  const std::int32_t slot = m_slots[static_cast<std::size_t>(_node) * m_nodes.size() + _target];
  return slot < 0 ? nullptr : &m_tables[_node][static_cast<std::size_t>(slot)];
}

std::optional<std::uint32_t> LandRouteGraph::NearestNode(const Neuron::Vec3& _position, const Neuron::TerrainField& _field,
                                                         const TuningMvp& _tuning) const
{
  if (m_nodes.empty())
  {
    return std::nullopt;
  }
  // R-SIM-346's scan in node order keeps the least distance among the walkable nodes, the first on a tie: the first
  // walkable node in the order of distance, then index, which tests only as many paths as it must.
  std::vector<std::uint32_t> order(m_nodes.size());
  std::iota(order.begin(), order.end(), 0u);
  const auto distance = [&](std::uint32_t _node) { return Neuron::LengthSquared(m_nodes[_node].position - _position); };
  std::ranges::stable_sort(order, [&](std::uint32_t _a, std::uint32_t _b) { return distance(_a) < distance(_b); });
  for (const std::uint32_t node : order)
  {
    if (IsPathClear(_position, m_nodes[node].position, true, false, _field, _tuning))
    {
      return node;
    }
  }
  return order.front();
}

std::vector<Neuron::Vec3> LandRouteGraph::BuildRoute(const Neuron::Vec3& _from, const Neuron::Vec3& _to, bool _directRoute,
                                                     bool _permitClimbing, bool _noRounding, const Neuron::TerrainField& _field,
                                                     const TuningMvp& _tuning) const
{
  const auto& route = _tuning.ref.sim.route;
  std::vector<Neuron::Vec3> points{_from};
  const std::optional<std::uint32_t> target = NearestNode(_to, _field, _tuning);
  const std::optional<std::uint32_t> start = NearestNode(_from, _field, _tuning);
  const bool corridor = target && start && !_directRoute;
  bool straightWalkable = false;
  double straightCost = 0.0;
  if (corridor)
  {
    straightWalkable = CorridorClear(_from, _to, _permitClimbing, _field, _tuning);
    straightCost = WalkCost(_from, _to, _field, _tuning);
  }
  if (corridor && !straightWalkable)
  {
    // R-SIM-348: the start node, then each next hop towards the target node, until it is reached or has no entry.
    std::uint32_t current = *start;
    points.push_back(m_nodes[current].position);
    while (current != *target)
    {
      const Hop* hop = HopOf(current, *target);
      if (hop == nullptr)
      {
        break;
      }
      current = hop->next;
      points.push_back(m_nodes[current].position);
      if (static_cast<std::int64_t>(points.size()) >= route.maxPoints)
      {
        points.assign(1, _from); // a diagnostic of the reference; the route degenerates to its start
        break;
      }
    }
  }
  points.push_back(_to);

  if (points.size() > 2 && !_noRounding)
  {
    // R-SIM-349: each interior corner pulled back towards its smoothed predecessor while the corridor to its successor
    // stays clear.
    for (std::size_t i = 1; i + 1 < points.size(); ++i)
    {
      const Neuron::Vec3 from = points[i - 1];
      const Neuron::Vec3 middle = points[i];
      const auto steps = static_cast<std::int64_t>(Neuron::Length(middle - from) / route.smoothingStep);
      Neuron::Vec3 accepted = middle;
      for (std::int64_t j = steps - 1; j >= 0; --j)
      {
        const Neuron::Vec3 candidate = from + (middle - from) * static_cast<double>(j) * (1.0 / static_cast<double>(steps));
        if (!CorridorClear(candidate, points[i + 1], true, _field, _tuning))
        {
          break;
        }
        accepted = candidate;
      }
      points[i] = accepted;
    }
  }

  // R-SIM-350: a point closer than the merge distance to its successor, with a clear path, merges with it, and the
  // merged point is tested again against its new successor.
  for (std::size_t i = 0; i + 1 < points.size();)
  {
    if (Neuron::Length(points[i + 1] - points[i]) < route.mergeDistance &&
        IsPathClear(points[i], points[i + 1], true, false, _field, _tuning))
    {
      points[i] = (points[i] + points[i + 1]) * 0.5;
      points.erase(points.begin() + static_cast<std::ptrdiff_t>(i) + 1);
    }
    else
    {
      ++i;
    }
  }

  // R-SIM-351: every point on the terrain, then the direct-route override, only after a corridor test (DEC-263's stated
  // behaviour).
  for (Neuron::Vec3& point : points)
  {
    point.y = _field.Height(point.x, point.z);
  }
  if (corridor)
  {
    const bool climbing = !straightWalkable && straightCost > 0.0;
    if (straightCost >= 0.0 && (!climbing || _permitClimbing) && straightCost <= WalkingDistance(points, _field, _tuning))
    {
      points = {_from, _to};
    }
  }
  return points;
}

} // namespace Species
