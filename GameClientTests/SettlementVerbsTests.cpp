#include "pch.h"

#include "SettlementVerbs.h"
#include "StructureCost.h"
#include "TestSupport.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <optional>
#include <variant>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameClientTests
{

namespace
{

constexpr std::uint8_t SITE = 2;
constexpr std::uint8_t TEAM = 2;
constexpr double CELL = 16.0; // mvp.territory.cell_size
// The territory's cells of the region raster: a square about the Wellspring's cell.
constexpr std::int32_t WELL_CELL = 100;
constexpr std::int32_t TERRITORY_HALF = 8;

// A region of 5,120 wu a side from (10,000, 20,000), as WLD-003's land cell has it.
Species::RegionLayout Layout()
{
  Species::RegionLayout layout;
  layout.originX = 10'000.0;
  layout.originZ = 20'000.0;
  layout.side = 5'120.0;
  return layout;
}

// The centre of the region raster's cell (_x, _z) on the ground.
Neuron::Vec3 CellCenter(std::int32_t _x, std::int32_t _z)
{
  return {Layout().originX + (_x + 0.5) * CELL, 0.0, Layout().originZ + (_z + 0.5) * CELL};
}

// A replica's tick holding the settlement of team TEAM with _storesMilliRu, its Wellspring at the centre of cell
// (WELL_CELL, WELL_CELL), its territory's square of cells about it, and _dwellings dwellings.
Species::StateReplica::View Replica(std::uint64_t _storesMilliRu, std::int32_t _dwellings = 0)
{
  Species::StateReplica::View view;
  Species::ReplicatedSettlement settlement;
  settlement.site = SITE;
  settlement.team = TEAM;
  settlement.storesMilliRu = _storesMilliRu;
  view.emplace(Species::SettlementObjectId(SITE), settlement);
  Species::ReplicatedStructure wellspring;
  wellspring.owner = TEAM;
  wellspring.kind = Species::StructureKind::Wellspring;
  wellspring.position = Species::QuantizePosition(CellCenter(WELL_CELL, WELL_CELL));
  view.emplace(Species::StableId{500}, wellspring);
  for (std::int32_t n = 0; n < _dwellings; ++n)
  {
    Species::ReplicatedStructure dwelling;
    dwelling.owner = TEAM;
    dwelling.kind = Species::StructureKind::Dwelling;
    view.emplace(Species::StableId{600} + static_cast<Species::StableId>(n), dwelling);
  }
  for (std::int32_t x = WELL_CELL - TERRITORY_HALF; x <= WELL_CELL + TERRITORY_HALF; ++x)
  {
    for (std::int32_t z = WELL_CELL - TERRITORY_HALF; z <= WELL_CELL + TERRITORY_HALF; ++z)
    {
      const auto chunkX = static_cast<std::uint8_t>(x / Species::CELL_CHUNK_SIDE);
      const auto chunkZ = static_cast<std::uint8_t>(z / Species::CELL_CHUNK_SIDE);
      const Species::StableId id = Species::CellChunkObjectId(Species::CellLayer::Territory, SITE, chunkX, chunkZ);
      auto& chunk = std::get<Species::ReplicatedCells>(
        view.try_emplace(id, Species::ReplicatedCells{Species::CellLayer::Territory, SITE, chunkX, chunkZ, 0}).first->second);
      chunk.cells |= std::uint64_t{1} << ((z % Species::CELL_CHUNK_SIDE) * Species::CELL_CHUNK_SIDE + x % Species::CELL_CHUNK_SIDE);
    }
  }
  return view;
}

Species::SettlementStatus Status(std::uint64_t _tick, std::size_t _queued = 0)
{
  Species::SettlementStatus status;
  status.tick = _tick;
  status.team = TEAM;
  status.priorities = {50, 30, 20};
  status.queue.resize(_queued);
  return status;
}

Species::StewardAreas AreasOf(const Species::StateReplica::View& _view)
{
  Species::StewardAreas areas;
  areas.Update(_view, TEAM, Layout(), CommittedTuning());
  return areas;
}

// A place inside the territory and outside the centre ring: six cells from the Wellspring, along +x.
Neuron::Vec3 BuildablePlace()
{
  return CellCenter(WELL_CELL + 6, WELL_CELL);
}

Species::HudZoneRect Zone(Species::HudZone _zone, std::uint8_t _index)
{
  return {_zone, _index, Neuron::UiRect::FromSize(0.0f, 0.0f, 10.0f, 10.0f)};
}

std::int64_t SumOf(const std::array<std::uint8_t, 3>& _shares)
{
  return std::int64_t{_shares[0]} + _shares[1] + _shares[2];
}

void ExpectShares(const std::array<std::uint8_t, 3>& _expected, const std::array<std::uint8_t, 3>& _actual, std::wstring_view _what)
{
  for (std::size_t task = 0; task < _expected.size(); ++task)
  {
    ExpectEqual(_expected[task], _actual[task], _what);
  }
}

} // namespace

TEST_CLASS(SettlementVerbsTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp036AShiftKeepsTheSharesAtAHundred)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-036")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp036AShiftKeepsTheSharesAtAHundred)
  {
    ExpectShares({60, 25, 15}, Species::ShiftPriority({50, 30, 20}, 0, 10), L"a raise takes half from each other task");
    ExpectShares({55, 27, 18}, Species::ShiftPriority({50, 30, 20}, 0, 5), L"the larger gives the odd point");
    ExpectShares({40, 35, 25}, Species::ShiftPriority({50, 30, 20}, 0, -10), L"a lower gives half to each");
    ExpectShares({45, 32, 23}, Species::ShiftPriority({50, 30, 20}, 0, -5), L"the smaller takes the odd point");
    ExpectShares({45, 25, 30}, Species::ShiftPriority({50, 30, 20}, 2, 10), L"guard duty raised");
    ExpectShares({100, 0, 0}, Species::ShiftPriority({95, 5, 0}, 0, 10), L"a raise takes what the others hold");
    ExpectShares({0, 50, 50}, Species::ShiftPriority({0, 50, 50}, 0, -10), L"no share below 0");
    for (std::uint8_t gather = 0; gather <= 100; gather += 5)
    {
      for (std::uint8_t construct = 0; gather + construct <= 100; construct += 5)
      {
        const std::array<std::uint8_t, 3> shares{gather, construct, static_cast<std::uint8_t>(100 - gather - construct)};
        for (std::size_t task = 0; task < shares.size(); ++task)
        {
          for (const std::int64_t step : {-15, -10, -1, 1, 10, 15})
          {
            ExpectEqual(100, SumOf(Species::ShiftPriority(shares, task, step)), L"every shift sums to 100");
          }
        }
      }
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp032TheClientForeseesWhatItCanOfARefusal)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-032")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-034")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp032TheClientForeseesWhatItCanOfARefusal)
  {
    const auto& tuning = CommittedTuning();
    const std::int64_t dwellingCost = Species::StructureCostMilliRu(tuning, Species::StructureKind::Dwelling, 0);
    const Species::StateReplica::View replica = Replica(static_cast<std::uint64_t>(dwellingCost));
    const Species::StewardAreas areas = AreasOf(replica);
    Species::PlacementView view = Species::PlacementViewOf(replica, TEAM, Status(10));
    Assert::IsTrue(view.settlement.has_value() && view.wellspring.has_value() && view.status.has_value(), L"the client's settlement");
    const auto foresee = [&](Species::StructureKind _kind, const Neuron::Vec3& _point)
    { return Species::PredictPlacement(view, areas, _kind, _point, tuning); };
    Assert::IsFalse(foresee(Species::StructureKind::Dwelling, BuildablePlace()).has_value(), L"a place every check passes");
    Assert::IsTrue(foresee(Species::StructureKind::Dwelling, CellCenter(WELL_CELL + TERRITORY_HALF + 2, WELL_CELL)) ==
                     Species::PlacementRefusal::OutsideTerritory,
                   L"a cell the territory does not hold");
    Assert::IsTrue(foresee(Species::StructureKind::Dwelling, CellCenter(WELL_CELL + 1, WELL_CELL)) ==
                     Species::PlacementRefusal::InCenterRing,
                   L"inside the centre ring");
    Assert::IsTrue(foresee(Species::StructureKind::Turret, BuildablePlace()) == Species::PlacementRefusal::CannotAfford,
                   L"a turret the stores cannot pay");
    // MVP-034: the cost escalates with the dwellings the settlement has.
    view = Species::PlacementViewOf(Replica(static_cast<std::uint64_t>(dwellingCost), 1), TEAM, Status(10));
    ExpectEqual(1, view.dwellings, L"its dwellings");
    Assert::IsTrue(foresee(Species::StructureKind::Dwelling, BuildablePlace()) == Species::PlacementRefusal::CannotAfford,
                   L"the second dwelling costs more");
    view = Species::PlacementViewOf(replica, TEAM, Status(10, static_cast<std::size_t>(tuning.mvp.construction.queueMax)));
    Assert::IsTrue(foresee(Species::StructureKind::Dwelling, BuildablePlace()) == Species::PlacementRefusal::QueueFull,
                   L"a full queue first");
    Assert::IsFalse(Species::PlacementViewOf(replica, std::uint8_t{1}, Status(10)).settlement.has_value(), L"another team's");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp101PlacingIsAModeThatTakesThePresses)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-101")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-032")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-461")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp101PlacingIsAModeThatTakesThePresses)
  {
    Species::SettlementVerbs verbs(CommittedTuning(), CommittedPresentation());
    const Species::StateReplica::View replica = Replica(1'000'000);
    const Species::StewardAreas areas = AreasOf(replica);
    const Species::PlacementView view = Species::PlacementViewOf(replica, TEAM, Status(10));
    Species::SettlementFrame frame;
    frame.ground = BuildablePlace();
    frame.primaryDown = true;
    Species::SettlementVerbResult result = verbs.Update(frame, TEAM, view, areas);
    Assert::IsFalse(result.tookPrimary || !result.commands.empty(), L"not placing, the press is the battle verbs'");

    // The construction bar's storehouse entry enters placing it; a press over the HUD reaches no world verb.
    frame.hudZone = Zone(Species::HudZone::Build, 1);
    result = verbs.Update(frame, TEAM, view, areas);
    Assert::IsTrue(result.tookPrimary && result.commands.empty(), L"the bar's press is taken");
    Assert::IsTrue(verbs.Placing() == Species::StructureKind::Storehouse, L"placing the storehouse");
    frame.hudZone.reset();
    frame.primaryDown = false;
    result = verbs.Update(frame, TEAM, view, areas);
    const Species::StructureGhost ghost = verbs.Ghost().value_or(Species::StructureGhost{});
    Assert::IsTrue(verbs.Ghost().has_value() && ghost.kind == Species::StructureKind::Storehouse, L"its ghost at the pointer");
    Assert::AreEqual(std::numbers::pi * 0.5, ghost.yawRadians, 1e-9, L"facing away from the Wellspring, +x");
    Assert::IsFalse(verbs.Foreseen().has_value(), L"no refusal foreseen");

    // The primary press places and stays in the mode; it never orders.
    frame.primaryDown = true;
    result = verbs.Update(frame, TEAM, view, areas);
    Assert::IsTrue(result.tookPrimary && result.commands.size() == 1, L"one placement");
    const auto* placed = std::get_if<Species::PlaceStructureCommand>(&result.commands.front());
    Assert::IsTrue(placed != nullptr && placed->team == TEAM && placed->kind == Species::StructureKind::Storehouse, L"the storehouse");
    Assert::AreEqual(BuildablePlace().x, placed->position.x, 1e-9, L"at the pointer ground point");
    Assert::IsTrue(verbs.Placing().has_value(), L"still placing");

    // A refusal foreseen is still sent, so that the reason shown is the server's (ADR-153).
    frame.ground = CellCenter(WELL_CELL + 1, WELL_CELL);
    result = verbs.Update(frame, TEAM, view, areas);
    Assert::IsTrue(verbs.Foreseen() == Species::PlacementRefusal::InCenterRing && result.commands.size() == 1, L"sent though foreseen");

    // The secondary press leaves the mode, as it releases a selection (O82), and reaches no battle verb.
    frame.primaryDown = false;
    frame.secondaryDown = true;
    result = verbs.Update(frame, TEAM, view, areas);
    Assert::IsTrue(result.tookSecondary && result.commands.empty() && !verbs.Placing().has_value(), L"the secondary press leaves");
    frame.secondaryDown = false;
    result = verbs.Update(frame, TEAM, view, areas);
    Assert::IsFalse(verbs.Ghost().has_value(), L"no ghost");

    // The same entry twice leaves the mode; with no settlement, no mode.
    frame.primaryDown = true;
    frame.hudZone = Zone(Species::HudZone::Build, 0);
    static_cast<void>(verbs.Update(frame, TEAM, view, areas));
    static_cast<void>(verbs.Update(frame, TEAM, view, areas));
    Assert::IsFalse(verbs.Placing().has_value(), L"the entry again leaves it");
    static_cast<void>(verbs.Update(frame, TEAM, view, areas));
    static_cast<void>(verbs.Update({}, std::nullopt, Species::PlacementView{}, areas));
    Assert::IsFalse(verbs.Placing().has_value(), L"no settlement of the client's, no placing");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp101ARefusalShowsItsReasonForItsTime)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-101")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-102")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp101ARefusalShowsItsReasonForItsTime)
  {
    Species::SettlementVerbs verbs(CommittedTuning(), CommittedPresentation());
    const Species::StateReplica::View replica = Replica(1'000'000);
    const Species::StewardAreas areas = AreasOf(replica);
    const Species::PlacementView view = Species::PlacementViewOf(replica, TEAM, Status(10));
    Species::PlaceStructureCommand placement;
    placement.team = TEAM;
    verbs.Sent(41, placement);
    verbs.Reported({40, 12, Species::CommandResult::NoEffect, static_cast<std::uint8_t>(Species::PlacementRefusal::Overlaps)});
    Assert::IsFalse(verbs.ForHud().refusal.has_value(), L"another command's outcome");
    verbs.Reported({41, 12, Species::CommandResult::NoEffect, static_cast<std::uint8_t>(Species::PlacementRefusal::Overlaps)});
    Assert::IsTrue(verbs.ForHud().refusal == Species::PlacementRefusal::Overlaps, L"the server's reason");
    Species::SettlementFrame frame;
    frame.realStepSeconds = CommittedPresentation().mvp.hud.noticeSeconds * 0.5;
    static_cast<void>(verbs.Update(frame, TEAM, view, areas));
    Assert::IsTrue(verbs.ForHud().refusal.has_value(), L"shown for mvp.hud.notice_seconds");
    static_cast<void>(verbs.Update(frame, TEAM, view, areas));
    Assert::IsFalse(verbs.ForHud().refusal.has_value(), L"and gone then");
    // An outcome with no placement reason shows as not buildable; an applied one clears the refusal.
    verbs.Sent(42, placement);
    verbs.Reported({42, 13, Species::CommandResult::NotPermitted, 0});
    Assert::IsTrue(verbs.ForHud().refusal == Species::PlacementRefusal::NotBuildable, L"no reason given");
    verbs.Sent(43, placement);
    verbs.Reported({43, 14, Species::CommandResult::Applied, 0});
    Assert::IsFalse(verbs.ForHud().refusal.has_value(), L"a placement applied");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp036TheArrowsSendTheSharesShownUntilAStatusHoldsThem)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-036")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-098")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp036TheArrowsSendTheSharesShownUntilAStatusHoldsThem)
  {
    Species::SettlementVerbs verbs(CommittedTuning(), CommittedPresentation());
    const Species::StateReplica::View replica = Replica(1'000'000);
    const Species::StewardAreas areas = AreasOf(replica);
    Species::PlacementView view = Species::PlacementViewOf(replica, TEAM, Status(10));
    Species::SettlementFrame frame;
    frame.primaryDown = true;
    frame.hudZone = Zone(Species::HudZone::PriorityRaise, 1);
    Species::SettlementVerbResult result = verbs.Update(frame, TEAM, view, areas);
    Assert::IsTrue(result.tookPrimary && result.commands.size() == 1, L"the raise arrow sends the shares");
    const auto* sent = std::get_if<Species::SetPrioritiesCommand>(&result.commands.front());
    const auto step = static_cast<std::int64_t>(CommittedPresentation().mvp.hud.priorityStep);
    Assert::IsTrue(sent != nullptr && sent->team == TEAM, L"its team's priorities");
    ExpectShares(Species::ShiftPriority({50, 30, 20}, 1, step), sent->shares, L"construction raised by mvp.hud.priority_step");
    verbs.Sent(7, result.commands.front());
    ExpectShares(sent->shares, verbs.ForHud().shares.value_or(std::array<std::uint8_t, 3>{}), L"shown at once");

    // A second press moves from the shares sent, not the status's.
    frame.hudZone = Zone(Species::HudZone::PriorityLower, 0);
    result = verbs.Update(frame, TEAM, view, areas);
    const auto* second = std::get_if<Species::SetPrioritiesCommand>(&result.commands.front());
    Assert::IsTrue(second != nullptr, L"a second change");
    ExpectShares(Species::ShiftPriority(Species::ShiftPriority({50, 30, 20}, 1, step), 0, -step), second->shares, L"from the shares sent");
    verbs.Sent(8, result.commands.front());
    verbs.Reported({7, 20, Species::CommandResult::Applied, 0});
    verbs.Reported({8, 21, Species::CommandResult::Applied, 0});
    frame = {};
    view.status = Status(20);
    static_cast<void>(verbs.Update(frame, TEAM, view, areas));
    Assert::IsTrue(verbs.ForHud().shares.has_value(), L"a status older than the change does not hold them");
    view.status = Status(21);
    static_cast<void>(verbs.Update(frame, TEAM, view, areas));
    Assert::IsFalse(verbs.ForHud().shares.has_value(), L"the status of the change's tick does");

    // An arrow that cannot move its share sends nothing.
    Species::SettlementStatus full = Status(30);
    full.priorities = {100, 0, 0};
    view.status = full;
    frame.primaryDown = true;
    frame.hudZone = Zone(Species::HudZone::PriorityRaise, 0);
    Assert::IsTrue(verbs.Update(frame, TEAM, view, areas).commands.empty(), L"gathering is at 100 already");
  }
};

} // namespace GameClientTests
