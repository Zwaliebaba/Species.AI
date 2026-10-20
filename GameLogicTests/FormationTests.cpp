#include "pch.h"

#include "Figure.h"
#include "Formation.h"
#include "TestSupport.h"

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <optional>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

constexpr Neuron::Vec3 MIDDLE{33000.0, 50.0, 33000.0};

// A Leader in formation mode at _position facing _front, with _slots slots, each held by member id index + 1 or empty.
Species::Figure FormationLeader(std::size_t _slots, const Neuron::Vec3& _position = MIDDLE, const Neuron::Vec3& _front = {0.0, 0.0, 1.0})
{
  Species::Figure leader;
  leader.id = 1000;
  leader.kind = Species::FigureKind::Leader;
  leader.position = _position;
  leader.front = _front;
  leader.order = Species::LeaderOrder::Follow;
  leader.formation = true;
  for (std::size_t index = 0; index < _slots; ++index)
  {
    leader.slots.push_back({index + 1, 100});
  }
  return leader;
}

} // namespace

TEST_CLASS(FormationTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit182SlotPositionsStandInFrontOfTheLeader)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-182")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit182SlotPositionsStandInFrontOfTheLeader)
  {
    // AC-UNIT-L03 on level ground: a Leader facing +z with 20 slots has rows of 6, and right = f × up = (−1, 0, 0).
    const std::shared_ptr<const Species::RegionTerrain> terrain = FlatTerrain(50.0);
    const Species::TuningMvp& tuning = CommittedTuning();
    const auto at = [&](std::uint32_t _index)
    { return Species::FormationSlotPosition(MIDDLE, _index, {0.0, 0.0, 1.0}, 20, terrain->field, tuning) - MIDDLE; };
    Assert::IsTrue(at(0) == Neuron::Vec3{18.0, 0.0, 12.0}, L"index 0: f × 12 + right × (0 − 18)");
    Assert::IsTrue(at(5) == Neuron::Vec3{-12.0, 0.0, 12.0}, L"index 5: f × 12 + right × (30 − 18)");
    Assert::IsTrue(at(6) == Neuron::Vec3{18.0, 0.0, 18.0}, L"index 6: the second rank, 6 wu further ahead");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit182TheRowSizeFollowsTheSlotArray)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-182")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit182TheRowSizeFollowsTheSlotArray)
  {
    // AC-UNIT-L04: table 2.6-A, gaps included.
    const Species::TuningMvp& tuning = CommittedTuning();
    const auto rows = [&](std::size_t _size) { return static_cast<std::int64_t>(Species::RowSizeOf(_size, tuning)); };
    ExpectEqual(6, rows(1), L"1");
    ExpectEqual(6, rows(24), L"24");
    ExpectEqual(9, rows(25), L"25");
    ExpectEqual(9, rows(47), L"47");
    ExpectEqual(12, rows(48), L"48");
    ExpectEqual(12, rows(89), L"89");
    ExpectEqual(15, rows(90), L"90");
    ExpectEqual(15, rows(199), L"199");
    ExpectEqual(25, rows(200), L"200");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit178ASlotTimesOutUnlessItsMemberRegisters)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-178")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-179")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-180")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit178ASlotTimesOutUnlessItsMemberRegisters)
  {
    // AC-UNIT-L05: a new slot's timer is 51 ticks; a member that stops registering loses it 51 upkeeps later.
    const std::shared_ptr<const Species::RegionTerrain> terrain = FlatTerrain(50.0);
    const Species::TuningMvp& tuning = CommittedTuning();
    Species::Figure leader = FormationLeader(0);
    static_cast<void>(Species::SlotPositionFor(leader, 7, terrain->field, tuning));
    Assert::IsTrue(leader.slots.size() == 1 && leader.slots[0] == Species::FormationSlot{7, 51}, L"a new slot with the slot timer");
    for (int upkeep = 1; upkeep <= 50; ++upkeep)
    {
      Species::FormationUpkeep(leader, tuning);
    }
    Assert::IsTrue(Species::SlotOf(leader, 7).has_value(), L"held after 50 upkeeps");
    Species::FormationUpkeep(leader, tuning);
    Assert::IsFalse(Species::SlotOf(leader, 7).has_value(), L"emptied by the 51st");

    static_cast<void>(Species::SlotPositionFor(leader, 8, terrain->field, tuning));
    for (int tick = 0; tick < 500; ++tick)
    {
      static_cast<void>(Species::SlotPositionFor(leader, 8, terrain->field, tuning));
      Species::RefreshSlot(leader, 8);
      Species::FormationUpkeep(leader, tuning);
    }
    Assert::IsTrue(Species::SlotOf(leader, 8) == std::optional<std::uint32_t>(0), L"a member that registers keeps its slot, the first gap");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit178TooManyGapsEmptyEverySlot)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-178")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit178TooManyGapsEmptyEverySlot)
  {
    // AC-UNIT-L06: runs of 3 and 4 gaps in 10 slots count 2 + 3 = 5, not above 5; runs of 3 and 5 count 6.
    const Species::TuningMvp& tuning = CommittedTuning();
    Species::Figure kept = FormationLeader(10);
    for (const std::size_t gap : std::initializer_list<std::size_t>{1, 2, 3, 5, 6, 7, 8})
    {
      kept.slots[gap] = {};
    }
    Species::FormationUpkeep(kept, tuning);
    ExpectEqual(3,
                std::count_if(kept.slots.begin(), kept.slots.end(), [](const Species::FormationSlot& _slot) { return _slot.member != 0; }),
                L"kept");
    Species::Figure emptied = FormationLeader(10);
    for (const std::size_t gap : std::initializer_list<std::size_t>{1, 2, 3, 5, 6, 7, 8, 9})
    {
      emptied.slots[gap] = {};
    }
    Species::FormationUpkeep(emptied, tuning);
    Assert::IsTrue(emptied.slots.size() == 10 && std::all_of(emptied.slots.begin(), emptied.slots.end(),
                                                             [](const Species::FormationSlot& _slot) { return _slot.member == 0; }),
                   L"every slot emptied, the array kept");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit180AFullFormationGivesNoSlot)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-180")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-181")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit180AFullFormationGivesNoSlot)
  {
    // AC-UNIT-L09: 90 occupied slots are the team's formation size.
    const std::shared_ptr<const Species::RegionTerrain> terrain = FlatTerrain(50.0);
    const Species::TuningMvp& tuning = CommittedTuning();
    Species::Figure leader = FormationLeader(90);
    Assert::IsTrue(Species::FormationFull(leader, tuning), L"full");
    Assert::IsTrue(Species::SlotPositionFor(leader, 5000, terrain->field, tuning) == Neuron::Vec3{}, L"the zero vector");
    Assert::IsFalse(Species::SlotOf(leader, 5000).has_value(), L"no slot");
    leader.slots[40] = {};
    const Neuron::Vec3 slot = Species::SlotPositionFor(leader, 5000, terrain->field, tuning);
    Assert::IsTrue(Species::SlotOf(leader, 5000) == std::optional<std::uint32_t>(40) && slot != Neuron::Vec3{}, L"a gap is taken first");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit103TheVolleyRhythmFollowsTheSlotIndex)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-103")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-177")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit103TheVolleyRhythmFollowsTheSlotIndex)
  {
    // AC-UNIT-L13: with 8 slots, index 3 fires where (4 × (t + 3)) mod 8 < 4, on every tick where t + 3 is even.
    const Species::TuningMvp& tuning = CommittedTuning();
    for (std::uint64_t tick = 0; tick < 40; ++tick)
    {
      Assert::AreEqual((tick + 3) % 2 == 0, Species::VolleyFires(tick, 3, 8, tuning), L"8 slots, index 3");
      Assert::IsTrue(Species::VolleyFires(tick, 1, 3, tuning), L"fewer than 4 slots fire every tick");
    }
    // n = min(90, slots): 120 slots fire as 90, so index 0 fires where 4t mod 90 < 4.
    std::int64_t fired = 0;
    for (std::uint64_t tick = 0; tick < 900; ++tick)
    {
      fired += Species::VolleyFires(tick, 0, 120, tuning) ? 1 : 0;
    }
    ExpectEqual(40, fired, L"one tick in 22.5 over 900 ticks");
  }
};

} // namespace GameLogicTests
