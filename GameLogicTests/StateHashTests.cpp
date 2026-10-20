#include "pch.h"

#include "SipHash.h"
#include "StateHash.h"
#include "TestSupport.h"
#include "WorkerPool.h"
#include "WorldSimulation.h"

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

// The seed of the golden runs: a fixed simulation key in place of the one a new world draws (ADR-082 item 1).
constexpr Neuron::SipHashKey GOLDEN_KEY{0x5eed5eed5eed5eed, 0x0123456789abcdef};

struct GoldenRun
{
  std::uint64_t last = 0;  // the world hash after the last tick
  std::uint64_t chain = 0; // SipHash of every tick's world hash, as little-endian words in tick order
};

GoldenRun Run600Ticks(std::uint16_t _regions, std::size_t _workers)
{
  Neuron::WorkerPool pool(_workers);
  Species::WorldSimulation world({GOLDEN_KEY, 0, _regions, &CommittedTuning()});
  Neuron::BitWriter chain;
  GoldenRun run;
  for (std::uint64_t tick = 0; tick < 600; ++tick)
  {
    world.Step(tick, pool);
    const Species::WorldHashes hashes = Species::HashWorld(world);
    Assert::IsTrue(hashes.nextTick == tick + 1, L"the hashes are those of the tick just stepped");
    chain.WriteBits(hashes.world, 64);
    run.last = hashes.world;
  }
  run.chain = Species::HashState(chain.Bytes());
  return run;
}

} // namespace

TEST_CLASS(StateHashTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dbg147TheHashOf600TicksFromOneSeedIsGolden)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-147")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dbg147TheHashOf600TicksFromOneSeedIsGolden)
  {
    // M0 task T8's done-when. The golden values change only with a reviewed change that names the rule it implements
    // (DBG-147); an independent model of the state layout of ADR-096 computes the same values. Changed by M1 task T2:
    // each region writes its team slots after its commands (R-SIM-221), none in this empty world. Changed by M1 task
    // T3: the world owner writes the world seed after the tick (ADR-102), seed 0 here. Changed by M1 task T4: each
    // region writes its actor grid's capacities and its cache after its teams (ADR-103), none in this empty world.
    // Changed by M1 task T5: each region writes its last route handle and its routes after its cache (ADR-104), none here.
    // Changed by M1 task T6: each region writes its laser-bolt, effect and Essence-orb pools after its routes (ADR-105):
    // two empty pools and the orb pool's 100 free slots, advanced to its last slot. Changed by M1 task T7: each region
    // writes its commanders before its commands (ADR-106), none here.
    const GoldenRun one = Run600Ticks(1, 0);
    Assert::IsTrue(one.last == 0xe6773df46414f09f, L"the hash after 600 ticks of one region");
    Assert::IsTrue(one.chain == 0x9403894070f00e95, L"the chain of 600 tick hashes of one region");
    const GoldenRun three = Run600Ticks(3, 0);
    Assert::IsTrue(three.last == 0x14634c20a5eed716, L"the hash after 600 ticks of three regions");
    Assert::IsTrue(three.chain == 0xa4385c2a7db63d5b, L"the chain of 600 tick hashes of three regions");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp093TheHashesAgreeAtAnyWorkerCount)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-093")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp093TheHashesAgreeAtAnyWorkerCount)
  {
    // ADR-081 Level 1: one build gives the same state every tick at any thread count (R-SIM-732).
    const GoldenRun serial = Run600Ticks(3, 0);
    for (const std::size_t workers : {std::size_t{1}, std::size_t{2}, std::size_t{8}})
    {
      const GoldenRun parallel = Run600Ticks(3, workers);
      Assert::IsTrue(parallel.chain == serial.chain, L"the same hashes on every tick");
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp093TheHashCoversTheStateAndNotTheKey)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-093")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp093TheHashCoversTheStateAndNotTheKey)
  {
    Neuron::WorkerPool pool(0);
    Species::WorldSimulation world({GOLDEN_KEY, 0, 2, &CommittedTuning()});
    Species::WorldSimulation sameState({{9, 9}, 0, 2, &CommittedTuning()});
    world.Step(0, pool);
    sameState.Step(0, pool);
    const Species::WorldHashes before = Species::HashWorld(world);
    Assert::IsTrue(before.world == Species::HashWorld(sameState).world, L"the simulation key is no part of the hash");
    ExpectEqual(3, static_cast<std::int64_t>(before.owners.size()), L"the world owner and two regions");
    Assert::IsTrue(before.owners[0].owner == Species::WORLD_OWNER_ID && before.owners[2].owner == Species::REGION_OWNER_ID_FIRST + 1,
                   L"in owner order");

    // An issued id is owner state: the world's hash and that owner's change, and no other owner's.
    static_cast<void>(world.Ids().Issue());
    const Species::WorldHashes after = Species::HashWorld(world);
    Assert::IsTrue(after.world != before.world, L"the world's hash changes");
    Assert::IsTrue(after.owners[0].hash != before.owners[0].hash, L"the world owner's changes");
    Assert::IsTrue(after.owners[1].hash == before.owners[1].hash && after.owners[2].hash == before.owners[2].hash, L"the regions' do not");
  }
};

} // namespace GameLogicTests
