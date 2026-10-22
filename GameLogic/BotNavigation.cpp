#include "pch.h"

#include "BotNavigation.h"
#include "DrawPurpose.h"
#include "RegionSimulation.h"
#include "SeenLayer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>

namespace Species
{

namespace
{

// The most team slots a saved node counts: the region's bound.
constexpr std::uint64_t BOT_NODE_TEAMS_MAX = RegionSimulation::TEAM_SLOTS;
// What an AI reads of a node its sight does not see (ADR-158): no team owns it and none is counted there.
const BotNode UNSEEN_NODE{};

void WriteCounts(Neuron::BitWriter& _writer, const std::vector<std::uint32_t>& _counts)
{
  _writer.WriteBounded(_counts.size(), BOT_NODE_TEAMS_MAX);
  for (const std::uint32_t count : _counts)
  {
    _writer.WriteBits(count, 32);
  }
}

void ReadCounts(Neuron::BitReader& _reader, std::vector<std::uint32_t>& _counts)
{
  const std::uint64_t teams = _reader.ReadBounded(BOT_NODE_TEAMS_MAX);
  _counts.assign(_reader.Failed() ? 0 : static_cast<std::size_t>(teams), 0);
  for (std::uint32_t& count : _counts)
  {
    count = static_cast<std::uint32_t>(_reader.ReadBits(32));
  }
}

} // namespace

BotGraph BotGraph::Build(const std::vector<NavigationNode>& _nodes, const Neuron::TerrainField& _field, const TuningMvp& _tuning)
{
  const auto& link = _tuning.ref.bot.link;
  BotGraph graph;
  const std::size_t count = _nodes.size();
  graph.m_positions.reserve(count);
  graph.m_checkCliffs.reserve(count);
  graph.m_sea.reserve(count);
  for (const NavigationNode& node : _nodes)
  {
    graph.m_positions.push_back(node.position);
    graph.m_checkCliffs.push_back(node.checkCliffs ? 1 : 0);
    graph.m_sea.push_back(_field.Height(node.position.x, node.position.z) <= _tuning.ref.sim.physics.seaLevel ? 1 : 0);
  }
  // R-BOT-030: a link to every other node within range that is walkable, or when either end is a sea node.
  graph.m_links.resize(count);
  for (std::uint32_t a = 0; a < count; ++a)
  {
    for (std::uint32_t b = 0; b < count; ++b)
    {
      if (a == b || Neuron::Length(graph.m_positions[b] - graph.m_positions[a]) > link.range)
      {
        continue;
      }
      const bool checkCliffs = graph.m_checkCliffs[a] != 0 && graph.m_checkCliffs[b] != 0;
      if (graph.m_sea[a] != 0 || graph.m_sea[b] != 0 ||
          IsPathClear(graph.m_positions[a], graph.m_positions[b], checkCliffs, false, _field, _tuning))
      {
        graph.m_links[a].push_back(b);
      }
    }
  }
  // R-BOT-034: a link A→C goes when another neighbor B of A links on to C and the detour is short enough; B's list may
  // already be culled. The MVP has no manual link and no turret node.
  const auto linkDistance = [&graph](std::uint32_t _from, std::uint32_t _to)
  {
    const std::vector<std::uint32_t>& links = graph.m_links[_from];
    return std::find(links.begin(), links.end(), _to) == links.end() ? -1.0
                                                                     : Neuron::Length(graph.m_positions[_to] - graph.m_positions[_from]);
  };
  for (std::uint32_t a = 0; a < count; ++a)
  {
    std::vector<std::uint32_t>& links = graph.m_links[a];
    std::size_t n = 0;
    while (n < links.size())
    {
      const std::uint32_t c = links[n];
      const double ac = linkDistance(a, c);
      bool culled = false;
      for (std::size_t x = 0; x < links.size() && !culled; ++x)
      {
        if (x == n)
        {
          continue;
        }
        const std::uint32_t b = links[x];
        const double bc = linkDistance(b, c);
        culled = bc > 0.0 && ac > link.cullRatio * (linkDistance(a, b) + bc);
      }
      if (culled)
      {
        links.erase(links.begin() + static_cast<std::ptrdiff_t>(n));
      }
      else
      {
        ++n;
      }
    }
  }
  return graph;
}

std::optional<std::uint32_t> BotGraph::NearestNode(const Neuron::Vec3& _point, bool _evaluateCliffs, bool _permitSteepDownhill,
                                                   const Neuron::TerrainField& _field, const TuningMvp& _tuning) const
{
  std::optional<std::uint32_t> nearest;
  double best = std::numeric_limits<double>::infinity();
  for (std::uint32_t node = 0; node < m_positions.size(); ++node)
  {
    const double distance = Neuron::Length(m_positions[node] - _point);
    if (distance < best && IsPathClear(_point, m_positions[node], _evaluateCliffs, _permitSteepDownhill, _field, _tuning))
    {
      best = distance;
      nearest = node;
    }
  }
  return nearest;
}

BotNode MakeBotNode(RegionSimulation& _region, std::uint32_t _index)
{
  BotNode node;
  // A node has no stable id; its index + 1 is its subject, since a subject is never 0 (ADR-082 item 3).
  const double phase = _region.Draws()
                         .For(DrawPurpose::NodePhase, StableId{_index} + 1)
                         .NextUnit(static_cast<double>(_region.Tuning().ref.bot.node.recountPhase));
  node.recountTicks = std::max(1, static_cast<std::int32_t>(std::ceil(phase)));
  return node;
}

void AdvanceBotNode(RegionSimulation& _region, std::uint32_t _index, BotNode& _node)
{
  const auto& tuning = _region.Tuning().ref.bot.node;
  --_node.recountTicks;
  if (_node.recountTicks > 0)
  {
    return;
  }
  _node.recountTicks = static_cast<std::int32_t>(tuning.recountTicks);
  // R-BOT-042: every actor of every team in range, in result order, each team's enemies the teams the region makes its
  // enemies (ADR-157, ADR-158). R-BOT-043: each Worker takes the node as its nearest node when it is nearer than the
  // one it has.
  const BotGraph& graph = _region.Bots();
  const Neuron::Vec3& position = graph.Position(_index);
  const std::size_t teams = _region.TeamCount();
  _node.friends.assign(teams, 0);
  _node.enemies.assign(teams, 0);
  _node.idle.assign(teams, 0);
  TeamMask every;
  every.set();
  for (const ActorRef& ref : _region.InRange(position.x, position.z, static_cast<double>(tuning.range), every))
  {
    Figure* figure = _region.Resolve(ref);
    if (figure == nullptr || ref.team >= teams)
    {
      continue;
    }
    ++_node.friends[ref.team];
    if (figure->kind != FigureKind::Worker)
    {
      continue;
    }
    if (!figure->dead && figure->workerState == WorkerState::Idle)
    {
      ++_node.idle[ref.team];
    }
    if (figure->nearestNode < 0 || Neuron::Length(position - figure->position) <
                                     Neuron::Length(graph.Position(static_cast<std::uint32_t>(figure->nearestNode)) - figure->position))
    {
      figure->nearestNode = static_cast<std::int32_t>(_index);
    }
  }
  for (std::size_t team = 0; team < teams; ++team)
  {
    const TeamMask enemies = _region.EnemiesOf(static_cast<std::uint8_t>(team));
    for (std::size_t other = 0; other < teams; ++other)
    {
      if (enemies.test(other))
      {
        _node.enemies[team] += _node.friends[other];
      }
    }
  }
  // R-BOT-047: the strictly highest friend count, from 0, so a tie goes to the lowest slot and an empty node has none.
  _node.owner = -1;
  std::uint32_t best = 0;
  for (std::size_t team = 0; team < teams; ++team)
  {
    if (_node.friends[team] > best)
    {
      best = _node.friends[team];
      _node.owner = static_cast<std::int32_t>(team);
    }
  }
}

std::uint32_t LeaderCount(const RegionSimulation& _region, std::uint8_t _team, const Neuron::Vec3& _point, double _range,
                          LeaderCountKind _kind)
{
  if (_team >= _region.TeamCount())
  {
    return 0;
  }
  const SlicedPool<Figure>& figures = _region.TeamAt(_team).figures;
  std::uint32_t count = 0;
  for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
  {
    if (!figures.Used(slot))
    {
      continue;
    }
    const Figure& leader = figures.At(slot);
    const bool waypoint = leader.order == LeaderOrder::Waypoint;
    if (leader.kind != FigureKind::Leader || leader.dead || leader.demoted || Neuron::Length(leader.position - _point) > _range ||
        (_kind == LeaderCountKind::Waypoint && !waypoint) || (_kind == LeaderCountKind::NotWaypoint && waypoint))
    {
      continue;
    }
    ++count;
  }
  return count;
}

bool NeedsDefending(const RegionSimulation& _region, const Sight& _sight, std::uint32_t _index, std::uint8_t _team)
{
  const BotNode& node = SeenBotNode(_region, _sight, _index);
  if (node.owner != _team)
  {
    return false;
  }
  const auto& tuning = _region.Tuning().ref.bot.node;
  const BotGraph& graph = _region.Bots();
  for (const std::uint32_t linked : graph.Links(_index))
  {
    const BotNode& neighbor = SeenBotNode(_region, _sight, linked);
    if (HeldByEnemy(_region, neighbor, _team) && neighbor.EnemiesOf(_team) > static_cast<std::uint32_t>(tuning.defendEnemyThreshold) &&
        LeaderCount(_region, _team, graph.Position(_index), tuning.defendLeaderRadius, LeaderCountKind::Every) == 0)
    {
      return true;
    }
  }
  return false;
}

const BotNode& SeenBotNode(const RegionSimulation& _region, const Sight& _sight, std::uint32_t _index)
{
  // The node's census counts every actor within its range, so the whole disc must be seen.
  const Neuron::Vec3& position = _region.Bots().Position(_index);
  return _sight.SeesDisc(position.x, position.z, static_cast<double>(_region.Tuning().ref.bot.node.range)) ? _region.BotNodes().At(_index)
                                                                                                           : UNSEEN_NODE;
}

bool HeldByEnemy(const RegionSimulation& _region, const BotNode& _node, std::uint8_t _team)
{
  return _node.owner >= 0 && _node.owner != _team && _region.EnemiesOf(_team).test(static_cast<std::size_t>(_node.owner));
}

void WriteBotNode(Neuron::BitWriter& _writer, const BotNode& _node)
{
  _writer.WriteSigned(_node.recountTicks, 32);
  _writer.WriteSigned(_node.owner, 32);
  WriteCounts(_writer, _node.friends);
  WriteCounts(_writer, _node.enemies);
  WriteCounts(_writer, _node.idle);
}

bool ReadBotNode(Neuron::BitReader& _reader, BotNode& _node)
{
  _node.recountTicks = static_cast<std::int32_t>(_reader.ReadSigned(32));
  _node.owner = static_cast<std::int32_t>(_reader.ReadSigned(32));
  ReadCounts(_reader, _node.friends);
  ReadCounts(_reader, _node.enemies);
  ReadCounts(_reader, _node.idle);
  return !_reader.Failed() && _node.owner >= -1 && _node.owner < static_cast<std::int32_t>(BOT_NODE_TEAMS_MAX);
}

bool Important(const RegionSimulation& _region, std::uint32_t _index)
{
  const Neuron::Vec3& node = _region.Bots().Position(_index);
  const double radius = _region.Tuning().ref.bot.node.importantRadius;
  const SlicedPool<Structure>& structures = _region.Structures();
  for (std::uint32_t slot = 0; slot < structures.Capacity(); ++slot)
  {
    if (structures.Used(slot) && !structures.At(slot).ports.empty() &&
        Neuron::LengthSquared(structures.At(slot).position - node) < radius * radius)
    {
      return true;
    }
  }
  return false;
}

} // namespace Species
