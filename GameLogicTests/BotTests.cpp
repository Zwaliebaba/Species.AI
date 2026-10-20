#include "pch.h"

#include "AiController.h"
#include "BitReader.h"
#include "BitWriter.h"
#include "BotNavigation.h"
#include "Combat.h"
#include "Formation.h"
#include "RegionSimulation.h"
#include "RegionTerrain.h"
#include "StateHash.h"
#include "TestSupport.h"
#include "WorkerPool.h"
#include "WorldSimulation.h"
#include "WorldSnapshot.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

constexpr Neuron::SipHashKey BOT_TEST_KEY{0xb07b07b000000001, 0xb07b07b000000002};
constexpr Neuron::Vec3 MIDDLE{33000.0, 50.0, 33000.0};

// Level ground with navigation nodes at _positions, in that order.
std::shared_ptr<const Species::RegionTerrain> TerrainWithNodes(const std::vector<Neuron::Vec3>& _positions)
{
  auto terrain = std::make_shared<Species::RegionTerrain>(*FlatTerrain(50.0));
  std::vector<Species::NavigationNode> nodes;
  nodes.reserve(_positions.size());
  for (const Neuron::Vec3& position : _positions)
  {
    nodes.push_back({position, true});
  }
  terrain->routes = Species::LandRouteGraph::Build(nodes, terrain->field, CommittedTuning());
  return terrain;
}

std::vector<std::uint32_t> SpawnCrowd(Species::RegionSimulation& _region, std::uint8_t _team, const Neuron::Vec3& _position,
                                      std::uint32_t _count, double _spread)
{
  Species::RegionSimulation::Spawn spawn;
  spawn.team = _team;
  spawn.position = _position;
  spawn.count = _count;
  spawn.spread = _spread;
  std::vector<std::uint32_t> slots = _region.SpawnFigures(spawn);
  for (const std::uint32_t slot : slots)
  {
    _region.TeamAt(_team).figures.At(slot).onGround = true;
  }
  return slots;
}

// 200 Workers a side around two diagonally opposite site centres of the valley, each side commanded by an AI (O70).
void AiBattle(Species::RegionSimulation& _region)
{
  const Species::RegionTerrain& terrain = *_region.Terrain();
  const std::array<std::size_t, 2> sites{0, 3};
  for (const std::uint8_t team : {std::uint8_t{0}, std::uint8_t{1}})
  {
    Neuron::Vec3 center = terrain.layout.sites[sites[team]];
    center.y = terrain.field.Height(center.x, center.z);
    static_cast<void>(SpawnCrowd(_region, team, center, 200, 60.0));
  }
  static_cast<void>(_region.AddAi(0));
  static_cast<void>(_region.AddAi(1));
}

std::int64_t Live(const Species::RegionSimulation& _region, std::uint8_t _team, Species::FigureKind _kind)
{
  const auto& figures = _region.TeamAt(_team).figures;
  std::int64_t count = 0;
  for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
  {
    count += figures.Used(slot) && !figures.At(slot).dead && figures.At(slot).kind == _kind ? 1 : 0;
  }
  return count;
}

std::vector<std::byte> StateOf(const Species::RegionSimulation& _region)
{
  Neuron::BitWriter writer;
  _region.WriteState(writer);
  return writer.Take();
}

} // namespace

TEST_CLASS(BotTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RBot030NodesLinkWithinRangeAndCullDetours)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BOT-030")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BOT-034")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RBot030NodesLinkWithinRangeAndCullDetours)
  {
    // AC-BOT-08: 590 wu apart on level ground, each links to the other; 610 wu, neither. AC-BOT-10: A, B and C in a line,
    // 200 wu apart, keep A–B and B–C and lose A–C both ways.
    const Species::TuningMvp& tuning = CommittedTuning();
    const auto field = FlatTerrain(50.0);
    const auto graph = [&](const std::vector<Neuron::Vec3>& _positions)
    {
      std::vector<Species::NavigationNode> nodes;
      nodes.reserve(_positions.size());
      for (const Neuron::Vec3& position : _positions)
      {
        nodes.push_back({position, true});
      }
      return Species::BotGraph::Build(nodes, field->field, tuning);
    };
    const Species::BotGraph linked = graph({MIDDLE, MIDDLE + Neuron::Vec3{590.0, 0.0, 0.0}});
    Assert::IsTrue(linked.Links(0) == std::vector<std::uint32_t>{1} && linked.Links(1) == std::vector<std::uint32_t>{0}, L"590 wu: linked");
    const Species::BotGraph apart = graph({MIDDLE, MIDDLE + Neuron::Vec3{610.0, 0.0, 0.0}});
    Assert::IsTrue(apart.Links(0).empty() && apart.Links(1).empty(), L"610 wu: not linked");
    const Species::BotGraph line = graph({MIDDLE, MIDDLE + Neuron::Vec3{200.0, 0.0, 0.0}, MIDDLE + Neuron::Vec3{400.0, 0.0, 0.0}});
    Assert::IsTrue(line.Links(0) == std::vector<std::uint32_t>{1}, L"A keeps B and loses C");
    Assert::IsTrue(line.Links(1) == (std::vector<std::uint32_t>{0, 2}), L"B keeps both");
    Assert::IsTrue(line.Links(2) == std::vector<std::uint32_t>{1}, L"C keeps B and loses A");
    const std::optional<std::uint32_t> nearest =
      line.NearestNode(MIDDLE + Neuron::Vec3{290.0, 0.0, 30.0}, true, false, field->field, tuning);
    Assert::IsTrue(nearest == std::optional<std::uint32_t>(1), L"R-BOT-052: the nearest walkable node");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RBot042ARecountCountsTeamsAndFindsTheOwner)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BOT-041")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BOT-042")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BOT-043")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BOT-047")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RBot042ARecountCountsTeamsAndFindsTheOwner)
  {
    // AC-BOT-11 with no alliance: 12 Workers of team 1, 12 of team 2 and 3 of team 0 within 200 wu; the owner is team 1,
    // the lower slot of the tie, and team 0 counts 24 enemies. AC-BOT-12: then one recount every 20 ticks.
    Species::RegionSimulation region(0, BOT_TEST_KEY, 0, CommittedTuning(),
                                     TerrainWithNodes({MIDDLE, MIDDLE + Neuron::Vec3{500.0, 0.0, 0.0}}));
    region.EnsureTeam(2);
    static_cast<void>(SpawnCrowd(region, 1, MIDDLE, 12, 50.0));
    static_cast<void>(SpawnCrowd(region, 2, MIDDLE, 12, 50.0));
    const std::vector<std::uint32_t> mine = SpawnCrowd(region, 0, MIDDLE + Neuron::Vec3{0.0, 0.0, 20.0}, 3, 0.0);
    // Nobody fights before the first recount, whose node counts no enemy (R-UNIT-098 step 6).
    int recounts = 0;
    std::uint64_t first = 0;
    for (std::uint64_t tick = 0; tick < 240; ++tick)
    {
      region.Step(region.NextTick());
      if (region.BotNodes().At(0).recountTicks != static_cast<std::int32_t>(CommittedTuning().ref.bot.node.recountTicks))
      {
        continue;
      }
      if (recounts == 0)
      {
        first = tick;
        const Species::BotNode& node = region.BotNodes().At(0);
        ExpectEqual(1, node.owner, L"the owner");
        ExpectEqual(24, node.EnemiesOf(0), L"team 0's enemies");
        ExpectEqual(3, node.FriendsOf(0), L"team 0's friends");
        ExpectEqual(3, node.IdleOf(0), L"team 0's idle Workers");
        ExpectEqual(-1, region.BotNodes().At(1).owner, L"a node with nobody near has no owner");
        Assert::IsTrue(region.TeamAt(0).figures.At(mine[0]).nearestNode == 0, L"R-BOT-043: the Worker's nearest node");
      }
      recounts += tick - first <= 200 ? 1 : 0;
    }
    ExpectEqual(11, recounts, L"the first recount and then 10 in 200 ticks");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit098NoEnemyAtANodeMeansNoThreat)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-098")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BOT-045")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit098NoEnemyAtANodeMeansNoThreat)
  {
    // Step 6, the census shortcut: an enemy 40 wu away is no threat while the node near the Worker counted no enemy at
    // its last recount, and is one after the recount that finds it.
    Species::RegionSimulation region(0, BOT_TEST_KEY, 0, CommittedTuning(), TerrainWithNodes({MIDDLE}));
    const std::uint32_t worker = SpawnCrowd(region, 0, MIDDLE, 1, 0.0).front();
    region.Step(region.NextTick());
    while (region.BotNodes().At(0).recountTicks != static_cast<std::int32_t>(CommittedTuning().ref.bot.node.recountTicks))
    {
      region.Step(region.NextTick());
    }
    static_cast<void>(SpawnCrowd(region, 1, MIDDLE + Neuron::Vec3{40.0, 0.0, 0.0}, 1, 0.0));
    const auto search = [&]
    {
      const Species::MovementContext movement = region.Movement();
      Species::Figure& figure = region.TeamAt(0).figures.At(worker);
      const Species::FigureContext context{&region, &movement, {figure.id, worker, 0}};
      return Species::ThreatSearch(figure, context);
    };
    Assert::IsFalse(search(), L"no threat while the node counts no enemy");
    for (int tick = 0; tick < 20; ++tick)
    {
      region.Step(region.NextTick());
    }
    ExpectEqual(1, region.BotNodes().At(0).EnemiesOf(0), L"the next recount finds the enemy");
    Assert::IsTrue(search(), L"and the Worker finds its threat");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RBot075TheAiThinksEvery200Ticks)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BOT-075")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-058")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RBot075TheAiThinksEvery200Ticks)
  {
    // AC-BOT-20 for the easy bot of M1: 10 main cycles 200 ticks apart in 2,000 ticks after its first.
    Species::RegionSimulation region(0, BOT_TEST_KEY, 0, CommittedTuning(), TerrainWithNodes({MIDDLE}));
    region.EnsureTeam(0);
    static_cast<void>(region.AddAi(0));
    std::vector<std::uint64_t> cycles;
    for (std::uint64_t tick = 0; tick < 2400; ++tick)
    {
      region.Step(region.NextTick());
      if (region.Ais().front().cycleTicks == static_cast<std::int32_t>(CommittedTuning().ref.bot.cycle.periodEasy))
      {
        cycles.push_back(tick);
      }
    }
    Assert::IsTrue(!cycles.empty() && cycles.front() < 200, L"the first within its phase");
    for (std::size_t index = 1; index < cycles.size(); ++index)
    {
      ExpectEqual(200, static_cast<std::int64_t>(cycles[index] - cycles[index - 1]), L"200 ticks apart");
    }
    ExpectEqual(11,
                static_cast<std::int64_t>(
                  std::count_if(cycles.begin(), cycles.end(), [&](std::uint64_t _tick) { return _tick - cycles.front() <= 2000; })),
                L"the first and 10 more in 2,000 ticks");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RBot091ACrowdedNodeGetsAFormationLeader)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BOT-091")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BOT-092")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-188")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-094")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RBot091ACrowdedNodeGetsAFormationLeader)
  {
    // 80 Workers at a node the AI's team owns: its main cycle promotes one through the command path, at the next tick;
    // the Leader's policy forms it up, and the Workers gather into its formation.
    Species::RegionSimulation region(0, BOT_TEST_KEY, 0, CommittedTuning(),
                                     TerrainWithNodes({MIDDLE, MIDDLE + Neuron::Vec3{400.0, 0.0, 0.0}}));
    static_cast<void>(SpawnCrowd(region, 0, MIDDLE, 80, 60.0));
    const Species::StableId ai = region.AddAi(0);
    std::uint64_t promoted = 0;
    for (std::uint64_t tick = 0; tick < 400 && promoted == 0; ++tick)
    {
      region.Step(region.NextTick());
      if (Live(region, 0, Species::FigureKind::Leader) == 1)
      {
        promoted = tick;
      }
    }
    Assert::IsTrue(promoted != 0, L"a Leader is promoted");
    bool viaQueue = false;
    for (const Neuron::CommandOutcome& outcome : region.Commands().TakeOutcomes())
    {
      viaQueue = viaQueue || (outcome.sender == ai && outcome.tick == promoted && !outcome.rejection);
    }
    Assert::IsTrue(viaQueue, L"by a command of the AI, executed at the tick it appeared");
    region.Step(region.NextTick());
    region.Step(region.NextTick());
    const auto& figures = region.TeamAt(0).figures;
    for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
    {
      if (figures.Used(slot) && figures.At(slot).kind == Species::FigureKind::Leader)
      {
        Assert::IsTrue(Species::InFormationMode(figures.At(slot)), L"formed up by its policy at its first call");
      }
    }
    std::int64_t members = 0;
    for (int tick = 0; tick < 100 && members == 0; ++tick)
    {
      region.Step(region.NextTick());
      for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
      {
        members += figures.Used(slot) && region.InFormation(figures.At(slot), 0) ? 1 : 0;
      }
    }
    Assert::IsTrue(members > 0, L"and Workers join its formation");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RBot085IdleWorkersAreSentAlongALinkAndStraysRecalled)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BOT-076")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BOT-078")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BOT-085")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BOT-088")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RBot085IdleWorkersAreSentAlongALinkAndStraysRecalled)
  {
    // A node of the AI's team that is not important is a corridor: its main cycle sends every idle Worker whose nearest
    // node it is to its one linked node. A lone idle Worker far from any node is recalled to its nearest node.
    const Neuron::Vec3 other = MIDDLE + Neuron::Vec3{500.0, 0.0, 0.0};
    Species::RegionSimulation region(0, BOT_TEST_KEY, 0, CommittedTuning(), TerrainWithNodes({MIDDLE, other}));
    const std::vector<std::uint32_t> crowd = SpawnCrowd(region, 0, MIDDLE, 15, 40.0);
    static_cast<void>(region.AddAi(0));
    const auto sent = [&]
    {
      std::int64_t count = 0;
      for (const std::uint32_t slot : crowd)
      {
        const Species::Figure& worker = region.TeamAt(0).figures.At(slot);
        count += worker.workerState == Species::WorkerState::OnWaypointOrder && worker.route == 0 &&
                     Neuron::HorizontalDistance(worker.orderPosition, other) <= 40.0
                   ? 1
                   : 0;
      }
      return count;
    };
    for (int tick = 0; tick < 250 && sent() == 0; ++tick)
    {
      region.Step(region.NextTick());
    }
    ExpectEqual(15, sent(), L"every idle Worker sent, with no route, to the linked node");

    Species::RegionSimulation lone(0, BOT_TEST_KEY, 0, CommittedTuning(), TerrainWithNodes({MIDDLE}));
    const std::uint32_t stray = SpawnCrowd(lone, 0, MIDDLE + Neuron::Vec3{0.0, 0.0, 300.0}, 1, 0.0).front();
    static_cast<void>(lone.AddAi(0));
    lone.Step(lone.NextTick());
    lone.Step(lone.NextTick());
    const Species::Figure& recalled = lone.TeamAt(0).figures.At(stray);
    Assert::IsTrue(recalled.workerState == Species::WorkerState::OnWaypointOrder &&
                     Neuron::HorizontalDistance(recalled.orderPosition, MIDDLE) <= 40.0,
                   L"recalled within 40 wu of the node");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp049TwoAisCommandTheirSidesOnTheValley)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-049")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-058")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-094")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp049TwoAisCommandTheirSidesOnTheValley)
  {
    // 200 a side at diagonally opposite sites of seed 1's valley, each side the AI's: each sends Workers along the graph,
    // promotes Leaders and forms them up, and every one of its commands takes the queue and is permitted. With no
    // reinforcement the reference rules do not bring the battle to an end: a formation advances only while it is full
    // (R-UNIT-188 item 4), and one that has lost members, or never filled, holds (ADR-107).
    Species::RegionSimulation region(0, BOT_TEST_KEY, 0, CommittedTuning(), ValleyTerrain());
    AiBattle(region);
    const std::array<Species::StableId, 2> ais{region.Ais()[0].id, region.Ais()[1].id};
    std::array<std::int64_t, 2> sent{0, 0};
    std::int64_t commands = 0;
    for (int tick = 0; tick < 4000; ++tick)
    {
      region.Step(region.NextTick());
      for (const Neuron::CommandOutcome& outcome : region.Commands().TakeOutcomes())
      {
        Assert::IsTrue((outcome.sender == ais[0] || outcome.sender == ais[1]) && !outcome.rejection, L"the AIs' commands, permitted");
        ++commands;
      }
      for (const std::uint8_t team : {std::uint8_t{0}, std::uint8_t{1}})
      {
        const auto& figures = region.TeamAt(team).figures;
        for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
        {
          sent[team] += figures.Used(slot) && figures.At(slot).workerState == Species::WorkerState::OnWaypointOrder ? 1 : 0;
        }
      }
    }
    Assert::IsTrue(commands > 0 && sent[0] > 0 && sent[1] > 0, L"both send Workers along the graph");
    for (const std::uint8_t team : {std::uint8_t{0}, std::uint8_t{1}})
    {
      const auto& figures = region.TeamAt(team).figures;
      std::int64_t formed = 0;
      std::int64_t members = 0;
      for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
      {
        if (!figures.Used(slot))
        {
          continue;
        }
        formed += figures.At(slot).kind == Species::FigureKind::Leader && Species::InFormationMode(figures.At(slot)) ? 1 : 0;
        members += region.InFormation(figures.At(slot), team) ? 1 : 0;
      }
      Assert::IsTrue(formed >= 1 && members >= 60, L"each side has formed Leaders with members");
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp093TheAiBattleHashesAgreeAtAnyWorkerCount)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-093")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-094")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp093TheAiBattleHashesAgreeAtAnyWorkerCount)
  {
    // Three regions, each an AI battle on the valley, give the same hash every tick at 1, 2 and 8 workers as serially.
    const auto run = [](std::size_t _workers)
    {
      Neuron::WorkerPool pool(_workers);
      Species::WorldSimulation world({BOT_TEST_KEY, 0, 3, &CommittedTuning()});
      for (std::size_t index = 0; index < world.RegionCount(); ++index)
      {
        world.Region(index).AttachTerrain(ValleyTerrain());
        AiBattle(world.Region(index));
      }
      Neuron::BitWriter chain;
      for (std::uint64_t tick = 0; tick < 600; ++tick)
      {
        world.Step(tick, pool);
        chain.WriteBits(Species::HashWorld(world).world, 64);
      }
      return Species::HashState(chain.Bytes());
    };
    const std::uint64_t serial = run(0);
    for (const std::size_t workers : {std::size_t{1}, std::size_t{2}, std::size_t{8}})
    {
      Assert::IsTrue(run(workers) == serial, L"the same hashes on every tick");
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp095AnAiBattleSurvivesARestore)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-095")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp095AnAiBattleSurvivesARestore)
  {
    // The AIs, their pending commands and the nodes' counts read back, and the battle steps on alike.
    Species::RegionSimulation region(0, BOT_TEST_KEY, 0, CommittedTuning(), ValleyTerrain());
    AiBattle(region);
    for (int tick = 0; tick < 700; ++tick)
    {
      region.Step(region.NextTick());
    }
    const std::vector<std::byte> bytes = StateOf(region);
    Species::RegionSimulation restored(0, BOT_TEST_KEY, region.NextTick(), CommittedTuning(), ValleyTerrain());
    Neuron::BitReader reader(bytes);
    Assert::IsTrue(restored.ReadState(reader, Species::WORLD_STATE_VERSION) && reader.Finish(), L"reads whole");
    Assert::IsTrue(StateOf(restored) == bytes, L"the same state");
    for (int tick = 0; tick < 300; ++tick)
    {
      region.Step(region.NextTick());
      restored.Step(restored.NextTick());
    }
    Assert::IsTrue(StateOf(restored) == StateOf(region), L"and steps on alike");
  }
};

} // namespace GameLogicTests
