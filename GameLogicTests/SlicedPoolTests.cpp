#include "pch.h"

#include "SlicedPool.h"
#include "TestSupport.h"

#include <algorithm>
#include <cstdint>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

constexpr std::uint32_t SLICES = 10; // `ref.sim.update_slices`

struct Advanced
{
  std::uint32_t slice = 0;
  std::uint32_t slot = 0;
};

// The slots each slice of one tick advances, in order.
std::vector<Advanced> RunTick(Species::SlicedPool<int>& _pool)
{
  std::vector<Advanced> advanced;
  for (std::uint32_t slice = 0; slice < SLICES; ++slice)
  {
    _pool.AdvanceSlice(slice, SLICES,
                       [&](std::uint32_t _slot)
                       {
                         advanced.push_back({slice, _slot});
                         return false;
                       });
  }
  return advanced;
}

Species::SlicedPool<int> FullPool(std::uint32_t _capacity)
{
  Species::SlicedPool<int> pool(_capacity);
  for (std::uint32_t slot = 0; slot < _capacity; ++slot)
  {
    static_cast<void>(pool.Take(1));
  }
  return pool;
}

} // namespace

TEST_CLASS(SlicedPoolTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RSim207FreedSlotsAreTakenLastFreedFirst)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-207")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RSim207FreedSlotsAreTakenLastFreedFirst)
  {
    // AC-1 of the containers: slots 3 and 7 freed in that order give 7 and then 3.
    Species::SlicedPool<int> pool = FullPool(10);
    pool.Free(3);
    pool.Free(7);
    ExpectEqual(7, pool.Take(2), L"the last freed first");
    ExpectEqual(3, pool.Take(2), L"then the one before it");
    // AC-2: a full pool of capacity 10 grows by its step, and the new object takes slot 10, then 11.
    ExpectEqual(10, pool.Take(3), L"the first new slot");
    ExpectEqual(20, pool.Capacity(), L"grown by its step");
    ExpectEqual(11, pool.Take(3), L"new slots in ascending order");
    ExpectEqual(12, pool.Count(), L"twelve objects");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RSim213SlicesCoverTheirShareAndTheLastTheRest)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-213")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RSim213SlicesCoverTheirShareAndTheLastTheRest)
  {
    // AC-4: a pool of capacity 100 advances 11 slots in each of slices 0 to 8 and 1 slot in slice 9, every slot once.
    Species::SlicedPool<int> pool = FullPool(100);
    const std::vector<Advanced> advanced = RunTick(pool);
    ExpectEqual(100, static_cast<std::int64_t>(advanced.size()), L"every slot once");
    std::vector<int> perSlice(SLICES, 0);
    for (std::size_t n = 0; n < advanced.size(); ++n)
    {
      ExpectEqual(static_cast<std::int64_t>(n), advanced[n].slot, L"in ascending slot order");
      ++perSlice[advanced[n].slice];
    }
    for (std::uint32_t slice = 0; slice + 1 < SLICES; ++slice)
    {
      ExpectEqual(11, perSlice[slice], L"slices 0 to 8: capacity / 10 + 1 slots");
    }
    ExpectEqual(1, perSlice[SLICES - 1], L"slice 9: the rest");
    ExpectEqual(99, pool.LastAdvanced(), L"the last slot advanced");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RSim213APoolThatGrowsUsesItsNewCapacityInLaterSlices)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-213")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RSim213APoolThatGrowsUsesItsNewCapacityInLaterSlices)
  {
    // AC-5: grown from 100 to 200 slots in slice 4, slices 5 to 9 take their bounds from 200.
    Species::SlicedPool<int> pool = FullPool(100);
    std::vector<Advanced> advanced;
    for (std::uint32_t slice = 0; slice < SLICES; ++slice)
    {
      pool.AdvanceSlice(slice, SLICES,
                        [&](std::uint32_t _slot)
                        {
                          advanced.push_back({slice, _slot});
                          if (slice == 4 && pool.Capacity() == 100)
                          {
                            for (int n = 0; n < 100; ++n)
                            {
                              static_cast<void>(pool.Take(9));
                            }
                          }
                          return false;
                        });
      if (slice == 5)
      {
        const auto range = pool.RangeOf(6, SLICES);
        ExpectEqual(55 + 21, range.first, L"slice 6 starts after slice 5's 21 slots");
        ExpectEqual(55 + 21 + 20, range.last, L"and covers 200 / 10 + 1 slots");
      }
    }
    ExpectEqual(200, static_cast<std::int64_t>(advanced.size()), L"every slot, the new ones included, once");
    ExpectEqual(54, static_cast<std::int64_t>(advanced[54].slot), L"slice 4 ended at slot 54, from capacity 100");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RSim215ANewPoolsFirstLateSliceSkipsSlotZero)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-215")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RSim215ANewPoolsFirstLateSliceSkipsSlotZero)
  {
    // AC-6: a pool made during a tick whose first slice is 3 starts at slot 1, so slot 0 waits for slice 0 of the
    // next tick; an emptied pool behaves the same.
    for (const bool emptied : {false, true})
    {
      Species::SlicedPool<int> pool = FullPool(5);
      if (emptied)
      {
        RunTick(pool);
        pool.Clear();
        for (int n = 0; n < 5; ++n)
        {
          static_cast<void>(pool.Take(1));
        }
      }
      std::vector<std::uint32_t> slots;
      for (std::uint32_t slice = 3; slice < SLICES; ++slice)
      {
        pool.AdvanceSlice(slice, SLICES,
                          [&](std::uint32_t _slot)
                          {
                            slots.push_back(_slot);
                            return false;
                          });
      }
      Assert::IsTrue(slots == std::vector<std::uint32_t>{1, 2, 3, 4}, L"slot 0 is not advanced in this tick");
      const std::vector<Advanced> next = RunTick(pool);
      Assert::IsTrue(!next.empty() && next.front().slot == 0 && next.front().slice == 0, L"it is in slice 0 of the next tick");
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RSim516AnObjectMadeDuringATickAdvancesWhenItsSlotIsStillAhead)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-516")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RSim516AnObjectMadeDuringATickAdvancesWhenItsSlotIsStillAhead)
  {
    // DEC-455's default: made in slice 2, an object in a slot after the last advanced one advances in this tick, and one
    // in an earlier, freed slot first advances in the next.
    Species::SlicedPool<int> pool = FullPool(30);
    pool.Free(25);
    pool.Free(1);
    std::uint32_t early = 0;
    std::uint32_t late = 0;
    std::vector<std::uint32_t> advanced;
    for (std::uint32_t slice = 0; slice < SLICES; ++slice)
    {
      pool.AdvanceSlice(slice, SLICES,
                        [&](std::uint32_t _slot)
                        {
                          advanced.push_back(_slot);
                          if (slice == 2 && early == 0)
                          {
                            early = pool.Take(5); // slot 1, already passed
                            late = pool.Take(6);  // slot 25, still ahead
                          }
                          return false;
                        });
    }
    ExpectEqual(1, early, L"the last freed slot");
    ExpectEqual(25, late, L"the slot freed before it");
    Assert::IsTrue(std::ranges::count(advanced, 25u) == 1, L"the object in slot 25 advanced in this tick");
    Assert::IsTrue(std::ranges::count(advanced, 1u) == 0, L"the object in slot 1 did not");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RSim221AnAdvanceThatAsksForRemovalFreesItsSlot)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-221")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RSim221AnAdvanceThatAsksForRemovalFreesItsSlot)
  {
    Species::SlicedPool<int> pool = FullPool(20);
    pool.AdvanceSlice(0, SLICES, [](std::uint32_t _slot) { return _slot == 2; });
    Assert::IsFalse(pool.Used(2), L"slot 2 was removed");
    ExpectEqual(19, pool.Count(), L"one object fewer");
    ExpectEqual(2, pool.Take(4), L"its slot is the head of the free list");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp095APoolReadsBackWhatItWrote)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-095")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp095APoolReadsBackWhatItWrote)
  {
    Species::SlicedPool<int> pool = FullPool(12);
    pool.Free(4);
    pool.Free(9);
    RunTick(pool);
    Neuron::BitWriter writer;
    const auto write = [](Neuron::BitWriter& _out, int _value) { _out.WriteBits(static_cast<std::uint64_t>(_value), 8); };
    pool.WriteState(writer, write);
    Species::SlicedPool<int> copy(12);
    Neuron::BitReader reader(writer.Bytes());
    Assert::IsTrue(copy.ReadState(reader, 1000,
                                  [](Neuron::BitReader& _in, int& _value)
                                  {
                                    _value = static_cast<int>(_in.ReadBits(8));
                                    return true;
                                  }),
                   L"the pool reads whole");
    Neuron::BitWriter again;
    copy.WriteState(again, write);
    Assert::IsTrue(std::ranges::equal(writer.Bytes(), again.Bytes()), L"the same state");
    ExpectEqual(9, copy.Take(1), L"the same free list");

    // A free list that names a used slot is no pool.
    Neuron::BitWriter broken;
    broken.WriteBits(2, 32);
    broken.WriteBits(0, 32);
    broken.WriteBits(1, 32);
    broken.WriteBits(0, 32);
    broken.WriteBool(true);
    broken.WriteBits(1, 8);
    broken.WriteBool(false);
    Species::SlicedPool<int> refused(12);
    Neuron::BitReader brokenReader(broken.Bytes());
    Assert::IsFalse(refused.ReadState(brokenReader, 1000,
                                      [](Neuron::BitReader& _in, int& _value)
                                      {
                                        _value = static_cast<int>(_in.ReadBits(8));
                                        return true;
                                      }),
                    L"slot 0 is both used and free");
  }
};

} // namespace GameLogicTests
