#include "pch.h"

#include "RelationMessages.h"
#include "SettlementVerbs.h"
#include "StructureCost.h"
#include "TestSupport.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <optional>
#include <variant>
#include <vector>

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

// ADR-167: a valley status for the settlement of site SITE (2), whose pairs are 2 and 3 (areas 2 and 3) and 0 and 2
// (areas 4 and 5): site 3 holds its half (area 3), which SITE may claim; SITE claims site 0's half (area 4), which 0
// holds; SITE holds the central site; site 3 holds a grievance against SITE; and SITE's expedition is held.
constexpr Species::StableId CLAIM_ID = 900;
constexpr Species::StableId EXPEDITION_ID = 901;

Species::ValleyStatus Valley()
{
  Species::ValleyStatus valley;
  valley.tick = 1'000;
  for (std::uint8_t site = 0; site < 4; ++site)
  {
    valley.renown.push_back({site});
  }
  valley.areas[2].holder = SITE;
  valley.areas[3].holder = std::uint8_t{3};
  valley.areas[4].holder = std::uint8_t{0};
  valley.areas[5].holder = SITE;
  valley.areas[Species::CLAIM_AREA_CENTRAL].holder = SITE;
  valley.claims = {{CLAIM_ID, SITE, 4, 500, Species::ClaimState::Open}};
  valley.grievances = {{3, SITE, Species::GrievanceCause::Pressure, 0, false, false, 7'500'000}};
  valley.expeditions = {{EXPEDITION_ID, SITE, 1, 0, 900, Species::ExpeditionState::Held, true, 10}};
  return valley;
}

// The place of the first relations row of _kind for _site in _rows.
std::uint8_t PlaceOf(const std::vector<Species::RelationRow>& _rows, Species::RelationRowKind _kind, std::uint8_t _site)
{
  const auto row = std::ranges::find_if(_rows, [&](const Species::RelationRow& _row) { return _row.kind == _kind && _row.site == _site; });
  Assert::IsTrue(row != _rows.end(), L"the row is listed");
  return static_cast<std::uint8_t>(row - _rows.begin());
}

// A frame that presses the button _slot of the row at _place, with the conflict view of Valley() and two Leaders.
Species::SettlementFrame Press(const std::vector<Species::RelationRow>& _rows, std::uint8_t _place, std::uint8_t _slot)
{
  Species::SettlementFrame frame;
  frame.primaryDown = true;
  frame.hudZone = Zone(Species::HudZone::RelationAction, static_cast<std::uint8_t>(_place * 2 + _slot));
  frame.relations = _rows;
  frame.conflict.valley = Valley();
  frame.conflict.site = SITE;
  frame.conflict.leaders = {Species::StableId{31}, Species::StableId{30}};
  frame.conflict.storesMilliRu = 250'000;
  return frame;
}

std::vector<Species::RelationRow> RowsOf(const Species::ValleyStatus& _valley)
{
  return Species::RelationRowsOf(_valley, SITE, _valley.tick, 0.1, 300.0, 40);
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

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp101TheConflictVerbsSendTheirCommands)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-101")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-053")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-054")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp101TheConflictVerbsSendTheirCommands)
  {
    // ADR-167: a claimable area's button claims it; the steward's claim's first button withdraws it; a grievance's
    // offers the stores as tribute, of which the server takes what eases it whole; the expedition's withdraws it.
    Species::SettlementVerbs verbs(CommittedTuning(), CommittedPresentation());
    const Species::StateReplica::View replica = Replica(1'000'000);
    const Species::StewardAreas areas = AreasOf(replica);
    const Species::PlacementView view = Species::PlacementViewOf(replica, TEAM, Status(10));
    const std::vector<Species::RelationRow> rows = RowsOf(Valley());
    const auto only = [&](const Species::SettlementFrame& _frame)
    {
      Species::SettlementVerbResult result = verbs.Update(_frame, TEAM, view, areas);
      Assert::IsTrue(result.tookPrimary && result.commands.size() == 1, L"one command, the press taken");
      return result.commands.front();
    };
    const auto claim = only(Press(rows, PlaceOf(rows, Species::RelationRowKind::Claimable, SITE), 0));
    const auto* made = std::get_if<Species::MakeClaimCommand>(&claim);
    Assert::IsTrue(made != nullptr && made->team == TEAM && made->area == 3, L"a claim on site 3's half");
    const auto withdrawal = only(Press(rows, PlaceOf(rows, Species::RelationRowKind::Claim, SITE), 0));
    const auto* withdrawn = std::get_if<Species::WithdrawClaimCommand>(&withdrawal);
    Assert::IsTrue(withdrawn != nullptr && withdrawn->team == TEAM && withdrawn->claim == CLAIM_ID, L"its claim withdrawn");
    const auto tribute = only(Press(rows, PlaceOf(rows, Species::RelationRowKind::Grievance, 3), 0));
    const auto* sent = std::get_if<Species::SendTributeCommand>(&tribute);
    Assert::IsTrue(sent != nullptr && sent->team == TEAM && sent->to == 3 && sent->cause == 0 && sent->subject == 0, L"to site 3");
    ExpectEqual(250'000, sent->amountMilliRu, L"the stores offered");
    const auto recall = only(Press(rows, PlaceOf(rows, Species::RelationRowKind::Expedition, 1), 0));
    Assert::IsTrue(std::holds_alternative<Species::WithdrawAttackCommand>(recall), L"its expedition withdrawn");
    // A button no row has sends nothing; with no settlement of the client's, nothing at all.
    Species::SettlementFrame beyond = Press(rows, 120, 1);
    Assert::IsTrue(verbs.Update(beyond, TEAM, view, areas).commands.empty(), L"no row there");
    Assert::IsTrue(verbs.Update(Press(rows, 0, 0), std::nullopt, Species::PlacementView{}, areas).commands.empty(), L"no settlement");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp042AnAttackIsDeclaredInTwoPresses)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-042")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-101")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-063")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp042AnAttackIsDeclaredInTwoPresses)
  {
    // ADR-167: the first press shows the attack with its cost and whether it is unprovoked; a second on the same button
    // declares it with the Leaders that lead it; another press over the HUD, or the secondary press, drops it.
    Species::SettlementVerbs verbs(CommittedTuning(), CommittedPresentation());
    const Species::StateReplica::View replica = Replica(1'000'000);
    const Species::StewardAreas areas = AreasOf(replica);
    const Species::PlacementView view = Species::PlacementViewOf(replica, TEAM, Status(10));
    const std::vector<Species::RelationRow> rows = RowsOf(Valley());
    const Species::SettlementFrame raid = Press(rows, PlaceOf(rows, Species::RelationRowKind::Cladan, 3), 0);
    Species::SettlementVerbResult result = verbs.Update(raid, TEAM, view, areas);
    Assert::IsTrue(result.tookPrimary && result.commands.empty(), L"the first press sends nothing");
    const Species::PendingAttack pending = verbs.Pending().value_or(Species::PendingAttack{});
    const auto& tuning = CommittedTuning();
    Assert::IsTrue(verbs.Pending().has_value() && pending.target == 3 && !pending.area.has_value() && pending.leaders == 2, L"a raid on 3");
    Assert::IsTrue(pending.unprovoked, L"unprovoked: no grievance against 3 and no claim on what 3 holds");
    ExpectEqual(std::llround(tuning.mvp.conflict.attackCostRu * tuning.pil.conflict.unprovokedCostFactor * 1'000.0), pending.costMilliRu,
                L"its cost, raised for an unprovoked attack");
    Assert::IsTrue(verbs.ForHud().pending.has_value(), L"shown on the HUD");
    result = verbs.Update(raid, TEAM, view, areas);
    Assert::IsTrue(result.commands.size() == 1, L"the second press declares it");
    const auto* declared = std::get_if<Species::DeclareAttackCommand>(&result.commands.front());
    Assert::IsTrue(declared != nullptr && declared->team == TEAM && declared->target == 3 && declared->claim == 0, L"the raid");
    Assert::IsTrue(declared->leaders == std::array<Species::StableId, 3>{31, 30, 0}, L"led by its Leaders");
    Assert::IsFalse(verbs.Pending().has_value(), L"no longer pending");

    // A raid on site 0, whose half SITE claims, is provoked; another press over the HUD drops it.
    const Species::SettlementFrame provoked = Press(rows, PlaceOf(rows, Species::RelationRowKind::Cladan, 0), 0);
    static_cast<void>(verbs.Update(provoked, TEAM, view, areas));
    Assert::IsTrue(verbs.Pending().has_value() && !verbs.Pending()->unprovoked, L"provoked by its claim");
    ExpectEqual(std::llround(tuning.mvp.conflict.attackCostRu * 1'000.0), verbs.Pending().value_or(Species::PendingAttack{}).costMilliRu,
                L"at the standard cost");
    Species::SettlementFrame elsewhere = provoked;
    elsewhere.hudZone = Zone(Species::HudZone::Build, 0);
    static_cast<void>(verbs.Update(elsewhere, TEAM, view, areas));
    Assert::IsFalse(verbs.Pending().has_value(), L"dropped by another press over the HUD");
    static_cast<void>(verbs.Update(elsewhere, TEAM, view, areas)); // leaves placing again

    // The claim's battle is fought against the area's holder, and declared with the claim; the secondary press drops it.
    const Species::SettlementFrame battle = Press(rows, PlaceOf(rows, Species::RelationRowKind::Claim, SITE), 1);
    static_cast<void>(verbs.Update(battle, TEAM, view, areas));
    Assert::IsTrue(verbs.Pending().has_value() && verbs.Pending()->target == 0 &&
                     verbs.Pending()->area == std::optional<std::uint8_t>(std::uint8_t{4}),
                   L"against site 0, which holds area 4");
    Species::SettlementFrame secondary;
    secondary.secondaryDown = true;
    result = verbs.Update(secondary, TEAM, view, areas);
    Assert::IsTrue(result.tookSecondary && !verbs.Pending().has_value(), L"the secondary press drops it and reaches no battle verb");
    static_cast<void>(verbs.Update(battle, TEAM, view, areas));
    result = verbs.Update(battle, TEAM, view, areas);
    const auto* fought = result.commands.empty() ? nullptr : std::get_if<Species::DeclareAttackCommand>(&result.commands.front());
    Assert::IsTrue(fought != nullptr && fought->target == 0 && fought->claim == CLAIM_ID, L"the claim battle declared");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp101AConflictRefusalShowsItsReason)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-101")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-102")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp101AConflictRefusalShowsItsReason)
  {
    // ADR-167: a conflict command's refusal is kept with its kind and reason for mvp.hud.notice_seconds, as a
    // placement's is; an applied one clears it; another command's outcome is not one of theirs.
    Species::SettlementVerbs verbs(CommittedTuning(), CommittedPresentation());
    const Species::StateReplica::View replica = Replica(1'000'000);
    const Species::StewardAreas areas = AreasOf(replica);
    const Species::PlacementView view = Species::PlacementViewOf(replica, TEAM, Status(10));
    verbs.Sent(50, Species::MakeClaimCommand{TEAM, 3});
    verbs.Reported({49, 12, Species::CommandResult::NoEffect, 1});
    Assert::IsFalse(verbs.ForHud().conflictRefusal.has_value(), L"another command's outcome");
    verbs.Reported({50, 12, Species::CommandResult::NoEffect, static_cast<std::uint8_t>(Species::ClaimRefusal::CoolingDown)});
    const auto refusal = verbs.ForHud().conflictRefusal;
    Assert::IsTrue(refusal.has_value() && refusal->first == Species::ConflictKind::Claim &&
                     refusal->second == static_cast<std::uint8_t>(Species::ClaimRefusal::CoolingDown),
                   L"the claim's reason");
    Assert::IsFalse(verbs.ForHud().refusal.has_value(), L"not a placement's");
    Species::SettlementFrame frame;
    frame.realStepSeconds = CommittedPresentation().mvp.hud.noticeSeconds;
    static_cast<void>(verbs.Update(frame, TEAM, view, areas));
    Assert::IsFalse(verbs.ForHud().conflictRefusal.has_value(), L"gone after mvp.hud.notice_seconds");
    verbs.Sent(51, Species::SendTributeCommand{TEAM, 3, 0, 0, 1'000});
    verbs.Reported({51, 13, Species::CommandResult::NoEffect, static_cast<std::uint8_t>(Species::TributeRefusal::CannotAfford)});
    const auto tribute = verbs.ForHud().conflictRefusal;
    Assert::IsTrue(tribute.has_value() && tribute->first == Species::ConflictKind::Tribute, L"a tribute's");
    verbs.Sent(52, Species::DeclareAttackCommand{TEAM, {31, 0, 0}, 3, 0});
    verbs.Reported({52, 14, Species::CommandResult::Applied, 0});
    Assert::IsFalse(verbs.ForHud().conflictRefusal.has_value(), L"an applied one clears it");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(O98AnExpeditionIsLedByUpToThreeLeaders)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-041")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(O98AnExpeditionIsLedByUpToThreeLeaders)
  {
    // ADR-167: the living Leaders of the team, the selected one first, then the others in id order, at most three.
    Species::StateReplica::View replica = Replica(1'000'000);
    const auto leader = [&](Species::StableId _id, std::uint8_t _team, bool _dead)
    {
      Species::ReplicatedFigure figure;
      figure.team = _team;
      figure.kind = Species::FigureKind::Leader;
      figure.dead = _dead;
      replica.emplace(_id, figure);
    };
    leader(30, TEAM, false);
    leader(31, TEAM, false);
    leader(32, TEAM, true);
    leader(33, TEAM, false);
    leader(34, 1, false);
    leader(35, TEAM, false);
    Assert::IsTrue(Species::ExpeditionLeadersOf(replica, TEAM, Species::StableId{33}) == std::vector<Species::StableId>{33, 30, 31},
                   L"the selected first, then by id, three");
    Assert::IsTrue(Species::ExpeditionLeadersOf(replica, TEAM, std::nullopt) == std::vector<Species::StableId>{30, 31, 33}, L"by id");
    Assert::IsTrue(Species::ExpeditionLeadersOf(replica, TEAM, Species::StableId{34}) == std::vector<Species::StableId>{30, 31, 33},
                   L"another team's Leader leads none of it");
  }
};

} // namespace GameClientTests
