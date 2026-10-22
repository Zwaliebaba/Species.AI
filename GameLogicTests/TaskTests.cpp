#include "pch.h"

#include "Construction.h"
#include "Growth.h"
#include "SettlementTasks.h"
#include "StateHash.h"
#include "TestSupport.h"
#include "WorkerPool.h"
#include "WorldSimulation.h"

#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

constexpr std::uint8_t SETTLED_TEAM = 2; // SettledWorld's settlement, whose team is its site

// The settlement's Workers, in slot order.
std::vector<Species::Figure*> WorkersOf(Species::RegionSimulation& _region)
{
  std::vector<Species::Figure*> workers;
  auto& figures = _region.TeamAt(SETTLED_TEAM).figures;
  for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
  {
    if (figures.Used(slot) && figures.At(slot).kind == Species::FigureKind::Worker && !figures.At(slot).dead)
    {
      workers.push_back(&figures.At(slot));
    }
  }
  return workers;
}

// A settled world whose first Workers have landed and whose settlement has the given priorities.
std::unique_ptr<Species::WorldSimulation> TaskWorld(const std::array<std::uint8_t, 3>& _priorities, Neuron::WorkerPool& _pool,
                                                    std::uint64_t& _tick)
{
  std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
  world->Region(0).Settlements().front().priorities = _priorities;
  for (; _tick < 60; ++_tick)
  {
    world->Step(_tick, _pool);
  }
  return world;
}

// The free gathering ports of the resource sites in the settlement's territory.
std::size_t FreeGatheringPorts(const Species::RegionSimulation& _region)
{
  const Species::CellRaster& territory = *_region.TerritoryOf(SETTLED_TEAM);
  std::size_t ports = 0;
  const auto& structures = _region.Structures();
  for (std::uint32_t slot = 0; slot < structures.Capacity(); ++slot)
  {
    if (structures.Used(slot) && structures.At(slot).kind == Species::StructureKind::ResourceSite &&
        territory.Holds(structures.At(slot).position.x, structures.At(slot).position.z))
    {
      ports += structures.At(slot).ports.size();
    }
  }
  return ports;
}

} // namespace

TEST_CLASS(TaskTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp031AGathererAtTheWellspringTakesTheNearestFreePort350WuAway)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-031")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp031AGathererAtTheWellspringTakesTheNearestFreePort350WuAway)
  {
    // O84: with gathering at 100 %, the first Worker to search takes the free port nearest to it, beyond the idle
    // search's 100 wu, at a home resource site about 350 wu from the Wellspring, and walks there along a route.
    Neuron::WorkerPool pool(0);
    std::uint64_t tick = 0;
    const std::unique_ptr<Species::WorldSimulation> world = TaskWorld({100, 0, 0}, pool, tick);
    Species::RegionSimulation& region = world->Region(0);
    const Species::Settlement& settlement = region.Settlements().front();
    const Neuron::Vec3 wellspring = region.Structures().At(settlement.wellspringSlot).position;
    Species::Figure* gatherer = nullptr;
    for (; tick < 120 && gatherer == nullptr; ++tick)
    {
      world->Step(tick, pool);
      for (Species::Figure* worker : WorkersOf(region))
      {
        gatherer = worker->task == Species::WorkerTask::Gather && worker->workerState == Species::WorkerState::ToPort ? worker : gatherer;
      }
    }
    Assert::IsNotNull(gatherer, L"a gatherer walks to a port");
    const Species::Structure& site = region.Structures().At(gatherer->portStructureSlot);
    Assert::IsTrue(site.kind == Species::StructureKind::ResourceSite, L"at a resource site");
    const double distance = Neuron::HorizontalDistance(site.position, wellspring);
    // Beyond ref.unit.worker.port_search_range, 100 wu, which the idle search of R-UNIT-090 would keep to.
    Assert::IsTrue(distance > 100.0 && distance < 450.0, L"about 350 wu out");
    Assert::IsTrue(gatherer->route != 0 || gatherer->waypoint == site.ports[gatherer->portIndex].position, L"along a route");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp031AGathererMansItsPortCarriesItsLoadAndDepositsIt)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-030")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-031")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp031AGathererMansItsPortCarriesItsLoadAndDepositsIt)
  {
    // Gatherers man their ports for mvp.gather.port_ticks, carry a load of mvp.gather.load_ru each to the Wellspring and
    // deposit it: the stores hold whole loads.
    Neuron::WorkerPool pool(0);
    std::uint64_t tick = 0;
    const std::unique_ptr<Species::WorldSimulation> world = TaskWorld({100, 0, 0}, pool, tick);
    Species::RegionSimulation& region = world->Region(0);
    const Species::Settlement& settlement = region.Settlements().front();
    bool carried = false;
    for (; tick < 1200; ++tick)
    {
      world->Step(tick, pool);
      for (const Species::Figure* worker : WorkersOf(region))
      {
        carried = carried || (worker->workerState == Species::WorkerState::Carrying &&
                              worker->carriedMilliRu == Species::MilliRuOf(CommittedTuning().mvp.gather.loadRu));
      }
    }
    Assert::IsTrue(carried, L"a load carried");
    const std::int64_t load = Species::MilliRuOf(CommittedTuning().mvp.gather.loadRu);
    Assert::IsTrue(settlement.storesMilliRu > 0 && settlement.storesMilliRu % load == 0, L"whole loads deposited");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp031AFullStoreRefusesTheLoadAndTheCarrierWaits)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-031")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-037")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp031AFullStoreRefusesTheLoadAndTheCarrierWaits)
  {
    // With the stores held full and the Wellspring's growth held off, a carrier that reaches the Wellspring keeps its load
    // and waits with the reason StoreFull; once the stores have room it deposits the load and goes back to its task.
    Neuron::WorkerPool pool(0);
    std::uint64_t tick = 0;
    const std::unique_ptr<Species::WorldSimulation> world = TaskWorld({100, 0, 0}, pool, tick);
    Species::RegionSimulation& region = world->Region(0);
    Species::Settlement& settlement = region.Settlements().front();
    const std::int64_t capacity = Species::StoreCapacityMilliRu(CommittedTuning(), 0);
    settlement.storesMilliRu = capacity;
    Species::Figure* waiting = nullptr;
    const std::int32_t interval = Species::GrowthIntervalTicks(CommittedTuning());
    for (; tick < 1500 && waiting == nullptr; ++tick)
    {
      settlement.storesMilliRu = capacity;
      settlement.growthTicks = interval;
      world->Step(tick, pool);
      for (Species::Figure* worker : WorkersOf(region))
      {
        waiting = worker->idleReason == Species::IdleReason::StoreFull ? worker : waiting;
      }
    }
    Assert::IsNotNull(waiting, L"a carrier waits");
    Assert::IsTrue(waiting->workerState == Species::WorkerState::Carrying && waiting->carriedMilliRu > 0, L"with its load");
    ExpectEqual(capacity, settlement.storesMilliRu, L"the store refused it");
    for (int step = 0; step < 20 && waiting->carriedMilliRu != 0; ++step)
    {
      settlement.storesMilliRu = 0;
      settlement.growthTicks = interval;
      world->Step(tick++, pool);
    }
    Assert::IsTrue(waiting->idleReason == Species::IdleReason::None && waiting->carriedMilliRu == 0, L"deposited once there is room");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp036ATaskWithNoWorkReleasesItsShare)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-036")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp036ATaskWithNoWorkReleasesItsShare)
  {
    // At the default 50/30/20 with an empty queue, construction has no work, so its 30 % go to gathering and guard duty
    // in proportion: of 20 Workers about 14 gather and 6 guard. A dwelling placed gives construction work, and Workers
    // take it up.
    Neuron::WorkerPool pool(0);
    std::uint64_t tick = 0;
    const std::unique_ptr<Species::WorldSimulation> world = TaskWorld({50, 30, 20}, pool, tick);
    Species::RegionSimulation& region = world->Region(0);
    Species::Settlement& settlement = region.Settlements().front();
    for (; tick < 200; ++tick)
    {
      world->Step(tick, pool);
    }
    const Species::TaskCounts counts = Species::CountTasks(region, settlement, 0);
    ExpectEqual(0, counts.tasks[1], L"no constructor without a site");
    Assert::IsTrue(counts.tasks[0] >= 13 && counts.tasks[0] <= 15 && counts.tasks[2] >= 5 && counts.tasks[2] <= 7,
                   L"its share released to the others in proportion");
    ExpectEqual(20, counts.tasks[0] + counts.tasks[2], L"every Worker has a task");
    settlement.storesMilliRu = Species::MilliRuOf(1000.0);
    const Neuron::Vec3 wellspring = region.Structures().At(settlement.wellspringSlot).position;
    bool placed = false;
    for (int turn = 0; turn < 36 && !placed; ++turn)
    {
      const double angle = turn * 3.141592653589793 / 18.0;
      placed = Species::PlaceStructure(region, settlement.site, Species::StructureKind::Dwelling, wellspring.x + 150.0 * std::cos(angle),
                                       wellspring.z + 150.0 * std::sin(angle))
                 .has_value();
    }
    Assert::IsTrue(placed, L"a dwelling placed");
    for (int step = 0; step < 300; ++step)
    {
      world->Step(tick++, pool);
    }
    Assert::IsTrue(Species::CountTasks(region, settlement, 0).tasks[1] > 0, L"Workers take up construction");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp037AnIdleWorkerSaysWhy)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-037")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp037AnIdleWorkerSaysWhy)
  {
    // Construction alone with an empty queue: every Worker has no task, because the queue is empty, and wanders.
    // Gathering alone with more Workers than gathering ports: each Worker gathers or has no free port, and some have
    // none. A reservation turns a second Worker away only within its tick (R-BLDG-025), so a few more than the ports
    // may walk to them and be turned away there.
    Neuron::WorkerPool pool(0);
    std::uint64_t tick = 0;
    {
      const std::unique_ptr<Species::WorldSimulation> world = TaskWorld({0, 100, 0}, pool, tick);
      for (int step = 0; step < 40; ++step)
      {
        world->Step(tick++, pool);
      }
      for (const Species::Figure* worker : WorkersOf(world->Region(0)))
      {
        Assert::IsTrue(worker->task == Species::WorkerTask::None && worker->idleReason == Species::IdleReason::QueueEmpty,
                       L"the queue is empty");
      }
    }
    tick = 0;
    const std::unique_ptr<Species::WorldSimulation> world = TaskWorld({100, 0, 0}, pool, tick);
    Species::RegionSimulation& region = world->Region(0);
    Assert::IsTrue(WorkersOf(region).size() > FreeGatheringPorts(region), L"more Workers than ports");
    for (int step = 0; step < 40; ++step)
    {
      world->Step(tick++, pool);
    }
    std::size_t noPort = 0;
    for (const Species::Figure* worker : WorkersOf(region))
    {
      const bool none = worker->task == Species::WorkerTask::None && worker->idleReason == Species::IdleReason::NoFreePort;
      Assert::IsTrue(none || worker->task == Species::WorkerTask::Gather, L"gathering or with no free port");
      noPort += none ? 1 : 0;
    }
    Assert::IsTrue(noPort > 0, L"those left over have no free port");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp036AGuardMansATurretAndOtherwiseGathersNearTheWellspring)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-036")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp036AGuardMansATurretAndOtherwiseGathersNearTheWellspring)
  {
    // Guard duty alone with a built turret of the settlement: four guards man its ports and the rest gather within
    // mvp.guard.gather_radius of the Wellspring, give or take a wander.
    Neuron::WorkerPool pool(0);
    std::uint64_t tick = 0;
    const std::unique_ptr<Species::WorldSimulation> world = TaskWorld({0, 0, 100}, pool, tick);
    Species::RegionSimulation& region = world->Region(0);
    const Species::Settlement& settlement = region.Settlements().front();
    const Neuron::Vec3 wellspring = region.Structures().At(settlement.wellspringSlot).position;
    const std::uint32_t turret =
      region.AddStructure(Species::StructureKind::Turret, SETTLED_TEAM, std::nullopt, wellspring.x + 150.0, wellspring.z, {0.0, 0.0, 1.0});
    for (int step = 0; step < 600; ++step)
    {
      world->Step(tick++, pool);
    }
    std::size_t manning = 0;
    for (const Species::Figure* worker : WorkersOf(region))
    {
      Assert::IsTrue(worker->task == Species::WorkerTask::Guard, L"on guard duty");
      if (worker->workerState == Species::WorkerState::Manning && worker->portStructureSlot == turret)
      {
        ++manning;
      }
      else
      {
        Assert::IsTrue(Neuron::HorizontalDistance(worker->position, wellspring) < CommittedTuning().mvp.guard.gatherRadius + 60.0,
                       L"near the Wellspring");
      }
    }
    ExpectEqual(4, static_cast<std::int64_t>(manning), L"the turret's four ports manned");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp033ConstructorsBuildTheFirstQueuedSite)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-033")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-036")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp033ConstructorsBuildTheFirstQueuedSite)
  {
    // Construction alone with a dwelling queued: Workers walk to its ports and build it, and it completes.
    Neuron::WorkerPool pool(0);
    std::uint64_t tick = 0;
    const std::unique_ptr<Species::WorldSimulation> world = TaskWorld({0, 100, 0}, pool, tick);
    Species::RegionSimulation& region = world->Region(0);
    Species::Settlement& settlement = region.Settlements().front();
    settlement.storesMilliRu = Species::MilliRuOf(1000.0);
    const Neuron::Vec3 wellspring = region.Structures().At(settlement.wellspringSlot).position;
    std::optional<std::uint32_t> site;
    for (int turn = 0; turn < 36 && !site; ++turn)
    {
      const double angle = turn * 3.141592653589793 / 18.0;
      const auto placed = Species::PlaceStructure(region, settlement.site, Species::StructureKind::Dwelling,
                                                  wellspring.x + 150.0 * std::cos(angle), wellspring.z + 150.0 * std::sin(angle));
      site = placed.has_value() ? std::optional<std::uint32_t>(placed.value()) : std::nullopt;
    }
    Assert::IsTrue(site.has_value(), L"a dwelling placed");
    const std::int64_t required = Species::RequiredWorkTicks(region.Structures().At(site.value_or(0)), CommittedTuning());
    for (int step = 0; step < 300; ++step)
    {
      world->Step(tick++, pool);
    }
    Assert::IsTrue(region.Structures().At(site.value_or(0)).workTicks > 0, L"its constructors work");
    region.Structures().At(site.value_or(0)).workTicks = required - 1;
    for (int step = 0; step < 3; ++step)
    {
      world->Step(tick++, pool);
    }
    Assert::IsTrue(region.Structures().At(site.value_or(0)).built && settlement.queue.empty(), L"built");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp093ASettlementAtWorkHashesAlikeAtAnyWorkerCount)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-093")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-036")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp093ASettlementAtWorkHashesAlikeAtAnyWorkerCount)
  {
    // The M2 plan's T7: a settlement at its default priorities, gathering and growing, gives the same hash on every
    // tick with 1, 2 and 8 worker threads as serially.
    const auto run = [](std::size_t _workers)
    {
      Neuron::WorkerPool pool(_workers);
      const std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
      world->Region(0).Settlements().front().storesMilliRu = Species::MilliRuOf(200.0);
      Neuron::BitWriter chain;
      for (std::uint64_t tick = 0; tick < 600; ++tick)
      {
        world->Step(tick, pool);
        chain.WriteBits(Species::HashWorld(*world).world, 64);
      }
      return Species::HashState(chain.Bytes());
    };
    static_cast<void>(ValleyTerrain());
    const std::vector<std::size_t> workerCounts{0, 1, 2, 8};
    std::vector<std::uint64_t> chains(workerCounts.size());
    std::vector<std::function<void()>> jobs;
    jobs.reserve(workerCounts.size());
    for (std::size_t index = 0; index < workerCounts.size(); ++index)
    {
      jobs.emplace_back([&chains, &run, &workerCounts, index] { chains[index] = run(workerCounts[index]); });
    }
    RunConcurrently(jobs);
    for (const std::uint64_t chain : chains)
    {
      Assert::IsTrue(chain == chains.front(), L"the same hashes on every tick");
    }
  }
};

} // namespace GameLogicTests
