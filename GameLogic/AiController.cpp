#include "pch.h"

#include "AiController.h"
#include "BotNavigation.h"
#include "CommandMessages.h"
#include "DrawPurpose.h"
#include "Formation.h"
#include "LeaderOrders.h"
#include "RegionSimulation.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <optional>
#include <set>
#include <vector>

namespace Species
{

namespace
{

constexpr double TWO_PI = 2.0 * std::numbers::pi;
// The most Leader timers a saved AI holds: a bound for the reader.
constexpr std::uint64_t AI_LEADER_TIMERS_MAX = 65'535;

GroundPoint Ground(const Neuron::Vec3& _point) noexcept
{
  return {_point.x, _point.z};
}

// What one tick of an AI decides. The commands it sends execute at the next tick, so it treats the figures it has
// ordered in this tick as no longer idle, and the Workers it has promoted in this cycle as Leaders where they stand, as
// the reference's immediate actions would leave them.
struct Turn
{
  RegionSimulation& region;
  AiController& ai;
  std::set<StableId> ordered;
  std::vector<Neuron::Vec3> promotions;

  template <class T> void Send(const T& _command)
  {
    static_cast<void>(region.Commands().Submit(ai.id, ai.nextSequence++, Neuron::EncodeMessage(_command)));
  }
  [[nodiscard]] const TuningMvp& Tuning() const noexcept
  {
    return region.Tuning();
  }
  [[nodiscard]] const Neuron::TerrainField& Field() const noexcept
  {
    return region.Terrain()->field;
  }
};

// R-BOT-076, R-BOT-085, R-UNIT-188: _center + polar(r, θ), with r = _base + NextUnit(_spread) drawn before θ = NextUnit(2π).
Neuron::Vec3 Polar(const Neuron::Vec3& _center, Neuron::SimRng& _draw, double _base, double _spread)
{
  const double radius = _base + _draw.NextUnit(_spread);
  const double angle = _draw.NextUnit(TWO_PI);
  return {_center.x + std::cos(angle) * radius, _center.y, _center.z + std::sin(angle) * radius};
}

// R-BOT-088: the random target selection from node _from, which redraws a sea node; the MVP's links cross no emitter
// and join no Beam Dish nodes.
std::optional<std::uint32_t> SelectRandom(Turn& _turn, std::uint32_t _from)
{
  const BotGraph& graph = _turn.region.Bots();
  const std::vector<std::uint32_t>& links = graph.Links(_from);
  if (links.empty())
  {
    return std::nullopt;
  }
  Neuron::SimRng& draw = _turn.region.Draws().For(DrawPurpose::AiSelect, _turn.ai.id);
  std::uint64_t index = draw.NextUint() % links.size();
  for (std::int64_t tries = 1; tries <= _turn.Tuning().ref.bot.select.randomTries; ++tries)
  {
    const std::uint32_t node = links[static_cast<std::size_t>(index)];
    if (!graph.Sea(node))
    {
      return node;
    }
    index = draw.NextUint() % links.size();
  }
  return std::nullopt;
}

bool IdleWorker(const Turn& _turn, const Figure& _figure) noexcept
{
  return _figure.kind == FigureKind::Worker && !_figure.dead && !_figure.promoted && _figure.workerState == WorkerState::Idle &&
         !_turn.ordered.contains(_figure.id);
}

// R-BOT-076.
void StrayRecall(Turn& _turn)
{
  RegionSimulation& region = _turn.region;
  const auto& bot = _turn.Tuning().ref.bot;
  SlicedPool<Figure>& figures = region.TeamAt(_turn.ai.team).figures;
  std::vector<std::uint32_t> live;
  for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
  {
    if (figures.Used(slot) && !figures.At(slot).dead)
    {
      live.push_back(slot);
    }
  }
  if (live.empty())
  {
    return;
  }
  const auto picks =
    std::max(bot.recall.minPicks, static_cast<std::int64_t>(std::floor(bot.recall.share * static_cast<double>(live.size()))));
  Neuron::SimRng& draw = region.Draws().For(DrawPurpose::AiRecall, _turn.ai.id);
  for (std::int64_t pick = 0; pick < picks; ++pick)
  {
    const Figure& worker = figures.At(live[static_cast<std::size_t>(draw.NextUint() % live.size())]);
    if (!worker.onGround || !IdleWorker(_turn, worker))
    {
      continue; // the MVP has no wall, so no wall-hit count to reset
    }
    const std::optional<std::uint32_t> nearest = region.Bots().NearestNode(worker.position, false, true, _turn.Field(), _turn.Tuning());
    if (!nearest || Neuron::Length(region.Bots().Position(*nearest) - worker.position) <= bot.recall.distance)
    {
      continue;
    }
    const Neuron::Vec3 point = Polar(region.Bots().Position(*nearest), region.Draws().For(DrawPurpose::AiOffset, worker.id),
                                     bot.order.offsetBase, bot.order.offsetSpread);
    _turn.Send(AiOrderWorkerCommand{_turn.ai.team, worker.id, Ground(point)});
    _turn.ordered.insert(worker.id);
  }
}

void SetFront(Turn& _turn, const Figure& _leader, const Neuron::Vec3& _front)
{
  _turn.Send(AiLeaderFieldsCommand{_turn.ai.team, _leader.id, _leader.noFormations, true, Ground(_front)});
}

// R-UNIT-188: one call of a Leader's policy; returns the ticks to its next call.
std::int32_t LeaderPolicy(Turn& _turn, const Figure& _leader)
{
  RegionSimulation& region = _turn.region;
  const BotGraph& graph = region.Bots();
  const auto& policy = _turn.Tuning().ref.unit.leader;
  const std::uint8_t team = _turn.ai.team;
  auto next = static_cast<std::int32_t>(policy.botThinkTicks);
  const std::optional<std::uint32_t> nearest = graph.NearestNode(_leader.position, true, false, _turn.Field(), _turn.Tuning());
  // Item 1: a waypoint Leader re-aims at the target selection of its nearest node.
  if (_leader.order == LeaderOrder::Waypoint && nearest)
  {
    next = static_cast<std::int32_t>(policy.botWaypointThinkTicks);
    const std::optional<std::uint32_t> target = SelectRandom(_turn, *nearest);
    if (target && Neuron::Length(graph.Position(*target) - _leader.orderPosition) > policy.botReorderDistance)
    {
      const Neuron::Vec3 point = Polar(graph.Position(*target), region.Draws().For(DrawPurpose::AiOffset, _leader.id),
                                       policy.botOrderOffsetBase, policy.botOrderOffsetSpread);
      _turn.Send(AiOrderLeaderCommand{team, _leader.id, LeaderOperation::AssignOrders, false, Ground(point)});
    }
    return next;
  }
  // Item 2: in formation; stopped and facing the enemies' mean position when many are near; done while it walks.
  if (!InFormationMode(_leader))
  {
    _turn.Send(AiOrderLeaderCommand{team, _leader.id, LeaderOperation::FormUp, false,
                                    Ground(_leader.position + _leader.front * policy.botFormationAhead)});
  }
  std::uint32_t enemies = 0;
  Neuron::Vec3 sum;
  for (const ActorRef& ref :
       region.InRange(_leader.position.x, _leader.position.z, policy.botEnemyRange, RegionSimulation::EnemiesOf(team)))
  {
    const Figure* enemy = region.Resolve(ref);
    if (enemy != nullptr && !enemy->dead)
    {
      ++enemies;
      sum = sum + enemy->position;
    }
  }
  if (enemies > static_cast<std::uint32_t>(policy.botEnemyCount))
  {
    _turn.Send(AiOrderLeaderCommand{team, _leader.id, LeaderOperation::AssignWaypoint, false, Ground(_leader.position)});
    SetFront(_turn, _leader, sum * (1.0 / static_cast<double>(enemies)) - _leader.position);
  }
  if (LeaderWalking(_leader, _turn.Tuning()) || !nearest)
  {
    return next;
  }
  // Item 3, with no important node in the MVP: a node that needs defending is held, facing from it towards its
  // hostile neighbour; otherwise the Leader faces towards the node's first neighbour.
  bool holding = false;
  const std::vector<std::uint32_t>& links = graph.Links(*nearest);
  if (NeedsDefending(region, *nearest, team))
  {
    holding = true;
    const auto hostile = std::find_if(links.begin(), links.end(),
                                      [&](std::uint32_t _node)
                                      {
                                        const BotNode& node = region.BotNodes().At(_node);
                                        return node.owner >= 0 && node.owner != team;
                                      });
    if (hostile != links.end())
    {
      SetFront(_turn, _leader, graph.Position(*hostile) - graph.Position(*nearest));
    }
  }
  else if (!links.empty())
  {
    SetFront(_turn, _leader, graph.Position(links.front()) - _leader.position);
  }
  // Item 4: a full formation that holds nothing walks to the nearest walkable node an enemy owns.
  if (holding || !FormationFull(_leader, _turn.Tuning()))
  {
    return next;
  }
  std::optional<std::uint32_t> goal;
  double best = std::numeric_limits<double>::infinity();
  for (std::uint32_t node = 0; node < graph.Size(); ++node)
  {
    const std::int32_t owner = region.BotNodes().At(node).owner;
    const double distance = Neuron::Length(graph.Position(node) - _leader.position);
    if (owner >= 0 && owner != team && distance < best &&
        IsPathClear(_leader.position, graph.Position(node), true, false, _turn.Field(), _turn.Tuning()))
    {
      best = distance;
      goal = node;
    }
  }
  if (goal)
  {
    _turn.Send(AiOrderLeaderCommand{team, _leader.id, LeaderOperation::AssignWaypoint, false, Ground(graph.Position(*goal))});
  }
  return next;
}

// R-BOT-074 step 4: each live Leader's policy countdown; a Leader's first call is at once.
void LeaderPolicies(Turn& _turn)
{
  const SlicedPool<Figure>& figures = _turn.region.TeamAt(_turn.ai.team).figures;
  std::vector<LeaderTimer> timers;
  for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
  {
    if (!figures.Used(slot))
    {
      continue;
    }
    const Figure& leader = figures.At(slot);
    if (leader.kind != FigureKind::Leader || leader.dead || leader.demoted)
    {
      continue;
    }
    const auto found = std::ranges::lower_bound(_turn.ai.leaderTimers, leader.id, {}, &LeaderTimer::leader);
    std::int32_t timer = found != _turn.ai.leaderTimers.end() && found->leader == leader.id ? found->ticks : 0;
    --timer;
    if (timer <= 0)
    {
      timer = LeaderPolicy(_turn, leader);
    }
    timers.push_back({leader.id, timer});
  }
  std::ranges::sort(timers, {}, &LeaderTimer::leader);
  _turn.ai.leaderTimers = std::move(timers);
}

// R-BOT-091: a node that needs defending or holds enough of the team, with no Leader of the team in formation near it,
// promotes the first Worker of the team near it.
void FormationLeaders(Turn& _turn)
{
  RegionSimulation& region = _turn.region;
  const BotGraph& graph = region.Bots();
  const auto& leader = _turn.Tuning().ref.bot.leader;
  const std::uint8_t team = _turn.ai.team;
  for (std::uint32_t node = 0; node < graph.Size(); ++node)
  {
    const BotNode& counts = region.BotNodes().At(node);
    const Neuron::Vec3& position = graph.Position(node);
    if (counts.owner != team ||
        (!NeedsDefending(region, node, team) && counts.FriendsOf(team) < static_cast<std::uint32_t>(leader.formationFriends)) ||
        LeaderCount(region, team, position, leader.formationLeaderRadius, LeaderCountKind::NotWaypoint) != 0 ||
        std::any_of(_turn.promotions.begin(), _turn.promotions.end(),
                    [&](const Neuron::Vec3& _promoted) { return Neuron::Length(_promoted - position) <= leader.formationLeaderRadius; }))
    {
      continue;
    }
    for (const ActorRef& ref : region.InRange(position.x, position.z, leader.formationCandidateRadius, RegionSimulation::FriendsOf(team)))
    {
      const Figure* worker = region.Resolve(ref);
      if (worker != nullptr && worker->kind == FigureKind::Worker && !worker->dead && !worker->promoted)
      {
        _turn.Send(PromoteWorkerCommand{team, worker->id});
        _turn.promotions.push_back(worker->position);
        _turn.ordered.insert(worker->id);
        break;
      }
    }
  }
}

// R-BOT-078 to R-BOT-086 for the easy bot: every calm node of the team with idle Workers to spare, and every node that is
// not important, sends Workers to a random linked node; the easy bot keeps no garrison.
void Dispatch(Turn& _turn)
{
  RegionSimulation& region = _turn.region;
  const BotGraph& graph = region.Bots();
  const auto& bot = _turn.Tuning().ref.bot;
  const std::uint8_t team = _turn.ai.team;
  std::vector<std::uint32_t> wellDefended;
  for (std::uint32_t node = 0; node < graph.Size(); ++node)
  {
    const BotNode& counts = region.BotNodes().At(node);
    const bool calm = static_cast<double>(counts.EnemiesOf(team)) < bot.dispatch.calmRatio * static_cast<double>(counts.FriendsOf(team));
    if (counts.owner == team && ((calm && counts.IdleOf(team) > static_cast<std::uint32_t>(bot.dispatch.idleLimit)) || !Important(node)))
    {
      wellDefended.push_back(node);
    }
  }
  for (const std::uint32_t from : wellDefended)
  {
    const std::optional<std::uint32_t> target = SelectRandom(_turn, from);
    if (!target)
    {
      continue;
    }
    // R-BOT-083: the send chance, every candidate's for a destination that is not important.
    const double enemies = static_cast<double>(region.BotNodes().At(*target).EnemiesOf(team));
    const double idle = static_cast<double>(region.BotNodes().At(from).IdleOf(team));
    double chance = 0.0;
    if (idle > 0.0)
    {
      chance = std::clamp(bot.dispatch.sendFactor * enemies / idle, bot.dispatch.sendMin, bot.dispatch.sendMax);
    }
    else if (enemies > 0.0)
    {
      chance = bot.dispatch.sendMax;
    }
    if (!Important(*target))
    {
      chance = bot.dispatch.sendAll;
    }
    const Neuron::Vec3& position = graph.Position(from);
    for (const ActorRef& ref : region.InRange(position.x, position.z, bot.dispatch.gatherRadius, RegionSimulation::FriendsOf(team)))
    {
      const Figure* worker = region.Resolve(ref);
      if (worker == nullptr || worker->kind != FigureKind::Worker)
      {
        continue;
      }
      const double draw = region.Draws().For(DrawPurpose::AiSend, worker->id).NextUnit(1.0);
      if (draw >= chance || !IdleWorker(_turn, *worker) || region.InFormation(*worker, team) || !worker->onGround ||
          worker->nearestNode != static_cast<std::int32_t>(from))
      {
        continue;
      }
      const Neuron::Vec3 point =
        Polar(graph.Position(*target), region.Draws().For(DrawPurpose::AiOffset, worker->id), bot.order.offsetBase, bot.order.offsetSpread);
      _turn.Send(AiOrderWorkerCommand{team, worker->id, Ground(point)});
      _turn.ordered.insert(worker->id);
    }
  }
}

} // namespace

AiController MakeAi(RegionSimulation& _region, StableId _id, std::uint8_t _team)
{
  AiController ai;
  ai.id = _id;
  ai.team = _team;
  const double phase =
    _region.Draws().For(DrawPurpose::AiCyclePhase, _id).NextUnit(static_cast<double>(_region.Tuning().ref.bot.cycle.phaseEasy));
  ai.cycleTicks = std::max(1, static_cast<std::int32_t>(std::ceil(phase)));
  return ai;
}

void StepAi(RegionSimulation& _region, AiController& _ai)
{
  if (_ai.team >= _region.TeamCount() || _region.Terrain() == nullptr)
  {
    return;
  }
  Turn turn{_region, _ai, {}, {}};
  StrayRecall(turn);
  --_ai.cycleTicks;
  LeaderPolicies(turn);
  if (_ai.cycleTicks <= 0)
  {
    _ai.cycleTicks = static_cast<std::int32_t>(_region.Tuning().ref.bot.cycle.periodEasy);
    FormationLeaders(turn);
    Dispatch(turn);
  }
}

void WriteAi(Neuron::BitWriter& _writer, const AiController& _ai)
{
  _writer.WriteBits(_ai.id, 64);
  _writer.WriteBits(_ai.team, 8);
  _writer.WriteSigned(_ai.cycleTicks, 32);
  _writer.WriteBits(_ai.nextSequence, 64);
  _writer.WriteBounded(_ai.leaderTimers.size(), AI_LEADER_TIMERS_MAX);
  for (const LeaderTimer& timer : _ai.leaderTimers)
  {
    _writer.WriteBits(timer.leader, 64);
    _writer.WriteSigned(timer.ticks, 32);
  }
}

bool ReadAi(Neuron::BitReader& _reader, AiController& _ai)
{
  _ai.id = _reader.ReadBits(64);
  _ai.team = static_cast<std::uint8_t>(_reader.ReadBits(8));
  _ai.cycleTicks = static_cast<std::int32_t>(_reader.ReadSigned(32));
  _ai.nextSequence = _reader.ReadBits(64);
  const std::uint64_t timers = _reader.ReadBounded(AI_LEADER_TIMERS_MAX);
  _ai.leaderTimers.clear();
  StableId previous = 0;
  for (std::uint64_t index = 0; index < timers && !_reader.Failed(); ++index)
  {
    const StableId leader = _reader.ReadBits(64);
    const auto timer = static_cast<std::int32_t>(_reader.ReadSigned(32));
    if (leader <= previous)
    {
      return false; // each Leader once, in id order
    }
    _ai.leaderTimers.push_back({leader, timer});
    previous = leader;
  }
  return !_reader.Failed() && _ai.id != 0;
}

} // namespace Species
