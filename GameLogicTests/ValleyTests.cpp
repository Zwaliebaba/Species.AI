#include "pch.h"

#include "CommandJournal.h"
#include "RegionSimulation.h"
#include "StateHash.h"
#include "TestSupport.h"
#include "WorkerPool.h"
#include "WorldSimulation.h"
#include "WorldSnapshot.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

constexpr std::uint8_t STEWARD_SITE = 2;
constexpr std::uint8_t AI_SETTLEMENTS = 3;
// A team that is no settlement's, as the sandbox's are.
constexpr std::uint8_t WILD_TEAM = 7;

// M3's valley (ADR-157): the steward's settlement at STEWARD_SITE and the three AI settlements at the other sites.
std::unique_ptr<Species::WorldSimulation> ValleyWorld(const Neuron::SipHashKey& _key = SETTLED_TEST_KEY)
{
  Species::WorldStart start;
  start.key = _key;
  start.seed = ValleyTerrain()->seed;
  start.stewardSite = STEWARD_SITE;
  start.aiSettlements = AI_SETTLEMENTS;
  return Species::StartWorld(start, ValleyTerrain(), CommittedTuning(), &TestRecords());
}

// The hurt and death events of _ticks steps of _world from _first.
std::size_t HarmOver(Species::WorldSimulation& _world, std::uint64_t _first, std::uint64_t _ticks)
{
  Neuron::WorkerPool pool(0);
  std::size_t harm = 0;
  for (std::uint64_t tick = _first; tick < _first + _ticks; ++tick)
  {
    _world.Step(tick, pool);
    harm += static_cast<std::size_t>(std::ranges::count_if(
      _world.Region(0).Events(), [](const Species::EffectEvent& _event)
      { return _event.kind == Species::EffectEventKind::FigureHurt || _event.kind == Species::EffectEventKind::FigureDied; }));
  }
  return harm;
}

// _count Workers of _team at the steward's Wellspring.
void SpawnAtSteward(Species::RegionSimulation& _region, std::uint8_t _team, std::uint32_t _count)
{
  Species::RegionSimulation::Spawn spawn;
  spawn.team = _team;
  spawn.position = _region.Terrain()->layout.sites[STEWARD_SITE];
  spawn.position.y = _region.Terrain()->field.Height(spawn.position.x, spawn.position.z);
  spawn.count = _count;
  spawn.spread = 20.0;
  static_cast<void>(_region.SpawnFigures(spawn));
}

} // namespace

TEST_CLASS(ValleyTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp013AWorldFoundsThreeAiSettlementsEachWithItsOwnTemperament)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-013")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-056")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-057")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp013AWorldFoundsThreeAiSettlementsEachWithItsOwnTemperament)
  {
    // ADR-157: the steward's settlement, then one AI settlement at each other site in site order, the three temperaments
    // dealt among them by the world's key, each run by a scripted steward as its AI commander.
    const std::unique_ptr<Species::WorldSimulation> world = ValleyWorld();
    const Species::RegionSimulation& region = world->Region(0);
    ExpectEqual(4, static_cast<std::int64_t>(region.Settlements().size()), L"four settlements");
    std::set<Species::Temperament> temperaments;
    for (const Species::Settlement& settlement : region.Settlements())
    {
      ExpectEqual(settlement.site, settlement.cladan.color, L"each Cladan its site's colour");
      if (settlement.site == STEWARD_SITE)
      {
        Assert::IsTrue(settlement.stewarded && settlement.cladan.temperament == Species::Temperament::None, L"the steward's has none");
        continue;
      }
      Assert::IsFalse(settlement.stewarded, L"an AI settlement");
      temperaments.insert(settlement.cladan.temperament);
      Assert::IsTrue(region.CommanderOf(settlement.team).ai, L"commanded by an AI");
      Assert::IsTrue(std::ranges::count(region.ScriptedStewards(), settlement.team, &Species::ScriptedSteward::team) == 1,
                     L"its scripted steward");
    }
    Assert::IsTrue(temperaments == std::set<Species::Temperament>{Species::Temperament::Bold, Species::Temperament::Territorial,
                                                                  Species::Temperament::Wary},
                   L"no two alike");

    // The deal follows the world's key: another key deals the temperaments otherwise for some world.
    std::set<std::vector<Species::Temperament>> deals;
    for (std::uint64_t low = 1; low <= 12; ++low)
    {
      const std::unique_ptr<Species::WorldSimulation> other = ValleyWorld({low, 0x0123456789abcdef});
      std::vector<Species::Temperament> deal;
      for (const Species::Settlement& settlement : other->Region(0).Settlements())
      {
        deal.push_back(settlement.cladan.temperament);
      }
      deals.insert(deal);
    }
    Assert::IsTrue(deals.size() > 1, L"the deal varies with the key");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Pil041SettlementsAreAtPeaceOutsideBattles)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-041")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-042")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Pil041SettlementsAreAtPeaceOutsideBattles)
  {
    // ADR-157: no settlement's team is another settlement's enemy, and a team that is no settlement's is every team's.
    const std::unique_ptr<Species::WorldSimulation> world = ValleyWorld();
    Species::RegionSimulation& region = world->Region(0);
    for (const Species::Settlement& settlement : region.Settlements())
    {
      const Species::TeamMask enemies = region.EnemiesOf(settlement.team);
      for (const Species::Settlement& other : region.Settlements())
      {
        Assert::IsFalse(enemies.test(other.team), L"no settlement is an enemy");
      }
      Assert::IsTrue(enemies.test(WILD_TEAM) && region.EnemiesOf(WILD_TEAM).test(settlement.team), L"a wild team is an enemy");
    }
    // Workers of two settlements standing together do each other no harm; a wild team's Workers among them are fought.
    SpawnAtSteward(region, 0, 10);
    ExpectEqual(0, static_cast<std::int64_t>(HarmOver(*world, 0, 100)), L"settlements together are at peace");
    SpawnAtSteward(region, WILD_TEAM, 10);
    Assert::IsTrue(HarmOver(*world, 100, 200) > 0, L"a wild team among them is fought");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp056FourSettlementsWorkSideBySideAtAnyWorkerCount)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-056")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-093")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp056FourSettlementsWorkSideBySideAtAnyWorkerCount)
  {
    // The four settlements step side by side with the same hashes at 0, 1, 2 and 8 worker threads, and each gathers.
    constexpr std::uint64_t TICKS = 1'200;
    constexpr std::uint64_t SAMPLE = 100;
    std::vector<std::vector<std::uint64_t>> hashes(4);
    std::vector<std::vector<std::int64_t>> gathered(4);
    const std::vector<std::size_t> workers{0, 1, 2, 8};
    std::vector<std::function<void()>> jobs;
    jobs.reserve(workers.size());
    for (std::size_t run = 0; run < workers.size(); ++run)
    {
      jobs.emplace_back(
        [&, run]
        {
          const std::unique_ptr<Species::WorldSimulation> world = ValleyWorld();
          Neuron::WorkerPool pool(workers[run]);
          for (std::uint64_t tick = 0; tick < TICKS; ++tick)
          {
            world->Step(tick, pool);
            if (tick % SAMPLE == 0)
            {
              hashes[run].push_back(Species::HashWorld(*world).world);
            }
          }
          const Species::RegionSimulation& region = world->Region(0);
          for (const Species::Settlement& settlement : region.Settlements())
          {
            gathered[run].push_back(settlement.storesMilliRu);
          }
        });
    }
    RunConcurrently(jobs);
    for (std::size_t run = 1; run < workers.size(); ++run)
    {
      Assert::IsTrue(hashes[run] == hashes[0], L"the same hashes at every worker count");
    }
    for (const std::int64_t stores : gathered[0])
    {
      Assert::IsTrue(stores > 0, L"each settlement gathers");
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp095AJournalKeepsItsAiSettlementsAndAnOlderOneHasNone)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-095")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-076")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp095AJournalKeepsItsAiSettlementsAndAnOlderOneHasNone)
  {
    // Journal format 6 holds the start's AI settlements, and its replay founds them again; a world of the M2 form, saved
    // with its settlement alone, restores with its settlement alone.
    Species::CommandJournal journal;
    journal.start.key = SETTLED_TEST_KEY;
    journal.start.seed = ValleyTerrain()->seed;
    journal.start.stewardSite = STEWARD_SITE;
    journal.start.aiSettlements = AI_SETTLEMENTS;
    const std::optional<Species::CommandJournal> read = Species::ReadJournal(Species::WriteJournal(journal));
    Assert::IsTrue(read.has_value() && read->start.aiSettlements == AI_SETTLEMENTS, L"read back");
    Neuron::WorkerPool pool(0);
    const std::unique_ptr<Species::WorldSimulation> replayed =
      Species::ReplayJournal(read.value_or(Species::CommandJournal{}), ValleyTerrain(), CommittedTuning(), &TestRecords(), 10, pool);
    const std::unique_ptr<Species::WorldSimulation> made = ValleyWorld();
    for (std::uint64_t tick = 0; tick < 10; ++tick)
    {
      made->Step(tick, pool);
    }
    Assert::IsTrue(Species::HashWorld(*replayed).world == Species::HashWorld(*made).world, L"the replay founds them again");

    const std::unique_ptr<Species::WorldSimulation> alone = SettledWorld(STEWARD_SITE);
    auto restored = Species::RestoreWorld(Species::SnapshotOf(*alone), CommittedTuning(), &TestRecords());
    Assert::IsTrue(restored.has_value(), L"restores");
    ExpectEqual(1, static_cast<std::int64_t>(restored.value()->Region(0).Settlements().size()), L"with its settlement alone");
  }
};

} // namespace GameLogicTests
