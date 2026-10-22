#include "pch.h"

#include "BitReader.h"
#include "BitWriter.h"
#include "MessageCodec.h"
#include "PresentationEvents.h"
#include "ReplicatedState.h"
#include "StableId.h"
#include "StateReplica.h"
#include "StateUpdate.h"
#include "TestSupport.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameSharedTests
{

namespace
{

Species::StableId Id(std::uint64_t _counter)
{
  return Species::MakeStableId(Species::REGION_OWNER_ID_FIRST, _counter);
}

Species::ReplicatedFigure Worker(std::uint8_t _team, std::int32_t _x, std::int32_t _z)
{
  Species::ReplicatedFigure figure;
  figure.team = _team;
  figure.position = {_x, 800, _z};
  figure.yaw = 100;
  figure.health = 50;
  figure.task = static_cast<std::uint8_t>(Species::WorkerState::Idle);
  figure.onGround = true;
  return figure;
}

// The record MakeRecord makes, which the test expects to exist.
Species::StateRecord RecordOf(Species::StableId _id, const Species::ReplicatedState& _state, const Species::ReplicatedState* _baseline)
{
  const std::optional<Species::StateRecord> record = Species::MakeRecord(_id, _state, _baseline);
  Assert::IsTrue(record.has_value(), L"the state differs from its baseline");
  return record.value_or(Species::StateRecord{});
}

// The record written and read back alone, after the bits it said it takes.
Species::StateRecord RoundTrip(const Species::StateRecord& _record, std::optional<Species::StableId> _previous)
{
  Neuron::BitWriter writer;
  Species::WriteRecord(writer, _record, _previous);
  ExpectEqual(static_cast<std::int64_t>(Species::RecordBits(_record, _previous)), static_cast<std::int64_t>(writer.BitCount()),
              L"the bits a record takes are counted exactly");
  Neuron::BitReader reader(writer.Bytes());
  Species::StateRecord read;
  Assert::IsTrue(Species::ReadRecord(reader, read, _previous), L"the record reads back");
  Assert::IsTrue(reader.Finish(), L"to its end");
  return read;
}

std::optional<Species::StateUpdate> Decoded(const std::vector<std::byte>& _bytes)
{
  Neuron::MessageDispatcher dispatcher;
  std::optional<Species::StateUpdate> received;
  dispatcher.Register<Species::StateUpdate>([&received](const Species::StateUpdate& _update) { received = _update; });
  static_cast<void>(dispatcher.Dispatch(_bytes));
  return received;
}

} // namespace

TEST_CLASS(StateUpdateTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr086APositionIsQuantizedTo16thsOfAUnit)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr086APositionIsQuantizedTo16thsOfAUnit)
  {
    const Neuron::Vec3 position{1234.567, -9.03, 4321.001};
    const Neuron::Vec3 back = Species::PositionOf(Species::QuantizePosition(position));
    Assert::IsTrue(std::abs(back.x - position.x) <= 1.0 / 32 && std::abs(back.y - position.y) <= 1.0 / 32 &&
                     std::abs(back.z - position.z) <= 1.0 / 32,
                   L"within half a step");
    const Species::QuantizedPosition clamped = Species::QuantizePosition({1e9, -1e9, std::numeric_limits<double>::quiet_NaN()});
    Assert::IsTrue(clamped.x == (1 << 23) - 1 && clamped.y == -(1 << 19) && clamped.z == 0, L"clamped to the fields, and NaN at 0");
    Assert::IsTrue(Species::QuantizeYaw(0.0, 1.0) == 0 && Species::QuantizeYaw(1.0, 0.0) == 256 && Species::QuantizeYaw(0.0, -1.0) == 512 &&
                     Species::QuantizeYaw(-1.0, 0.0) == 768 && Species::QuantizeYaw(0.0, 0.0) == 0,
                   L"0 faces +z, a quarter turn +x");
    Assert::IsTrue(std::abs(Species::YawRadians(256) - std::acos(0.0)) < 1e-12, L"a quarter turn in radians");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr086AWholeRecordOfEachClassRoundTrips)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr086AWholeRecordOfEachClassRoundTrips)
  {
    Species::ReplicatedFigure leader = Worker(1, -40'000, 70'000);
    leader.kind = Species::FigureKind::Leader;
    leader.task = static_cast<std::uint8_t>(Species::LeaderState::GivingOrders);
    leader.formation = true;
    leader.selected = true;
    leader.dead = true;
    leader.target = Species::QuantizedPosition{(1 << 23) - 1, -(1 << 19), 12};
    const Species::ReplicatedEffect ring{2, Species::EffectKind::ShockwaveRing, {1, -2, 3}, 1023, 7, 640};
    const Species::ReplicatedOrb orb{3, {(1 << 23) - 1, (1 << 19) - 1, -(1 << 23)}, Species::OrbState::Floating, 65'535};
    std::optional<Species::StableId> previous;
    std::uint64_t counter = 5;
    for (const Species::ReplicatedState& state :
         {Species::ReplicatedState{leader}, Species::ReplicatedState{ring}, Species::ReplicatedState{orb}})
    {
      const Species::StateRecord record = RecordOf(Id(counter), state, nullptr);
      Assert::IsTrue(record.kind == Species::RecordKind::Whole, L"no baseline: whole");
      Assert::IsTrue(RoundTrip(record, previous) == record, L"the same record");
      Assert::IsTrue(Species::ApplyRecord(record, nullptr) == state, L"it gives its state alone");
      previous = Id(counter);
      counter += 1000;
    }
    const Species::StateRecord removal = Species::RemovalRecord(Id(9));
    Assert::IsTrue(RoundTrip(removal, Id(8)) == removal, L"a removal");
    ExpectEqual(7, static_cast<std::int64_t>(Species::RecordBits(removal, Id(8))), L"a removal next to its neighbour takes 7 bits");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr086ADeltaCarriesOnlyWhatChanged)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr086ADeltaCarriesOnlyWhatChanged)
  {
    const Species::ReplicatedFigure baseline = Worker(0, 1000, 1000);
    Assert::IsFalse(
      Species::MakeRecord(Id(1), baseline, &static_cast<const Species::ReplicatedState&>(Species::ReplicatedState{baseline})).has_value(),
      L"no change, no record");
    const Species::ReplicatedState held{baseline};
    // A move of each width, the widest from one end of the fields to the other.
    for (const Species::QuantizedPosition& to :
         {Species::QuantizedPosition{1010, 800, 990}, Species::QuantizedPosition{1900, 600, 100},
          Species::QuantizedPosition{9000, -8000, 1000}, Species::QuantizedPosition{(1 << 23) - 1, -(1 << 19), -(1 << 23)}})
    {
      Species::ReplicatedFigure moved = baseline;
      moved.position = to;
      const Species::StateRecord record = RecordOf(Id(1), moved, &held);
      Assert::IsTrue(record.kind == Species::RecordKind::Delta && record.changed == Species::FIGURE_FIELD_POSITION, L"a move alone");
      const Species::StateRecord read = RoundTrip(record, std::nullopt);
      Assert::IsTrue(read == record, L"the same delta");
      Assert::IsTrue(Species::ApplyRecord(read, &held) == Species::ReplicatedState{moved}, L"applied to its baseline, the new state");
    }
    Species::ReplicatedFigure hurt = baseline;
    hurt.health = 3;
    hurt.task = static_cast<std::uint8_t>(Species::WorkerState::Burning);
    hurt.underControl = true;
    const Species::StateRecord record = RecordOf(Id(1), hurt, &held);
    Assert::IsTrue(record.changed == (Species::FIGURE_FIELD_HEALTH | Species::FIGURE_FIELD_TASK | Species::FIGURE_FIELD_FLAGS),
                   L"health, task and flags");
    Assert::IsTrue(Species::ApplyRecord(RoundTrip(record, std::nullopt), &held) == Species::ReplicatedState{hurt}, L"applied");
    Assert::IsTrue(Species::RecordBits(record, std::nullopt) < Species::RecordBits(RecordOf(Id(1), hurt, nullptr), std::nullopt),
                   L"a delta is smaller than the whole record");

    // A Leader's order and an orb's state and timer, each alone.
    Species::ReplicatedFigure ordered = baseline;
    ordered.order = static_cast<std::uint8_t>(Species::LeaderOrder::Follow);
    const Species::StateRecord order = RecordOf(Id(1), ordered, &held);
    Assert::IsTrue(order.changed == Species::FIGURE_FIELD_ORDER, L"the order alone");
    Assert::IsTrue(Species::ApplyRecord(RoundTrip(order, std::nullopt), &held) == Species::ReplicatedState{ordered}, L"applied");
    // ADR-125: the flag pointer's target alone, set and then cleared.
    Species::ReplicatedFigure aimed = baseline;
    aimed.target = Species::QuantizedPosition{-5000, 120, 7000};
    const Species::StateRecord target = RecordOf(Id(1), aimed, &held);
    Assert::IsTrue(target.changed == Species::FIGURE_FIELD_TARGET, L"the target alone");
    Assert::IsTrue(Species::ApplyRecord(RoundTrip(target, std::nullopt), &held) == Species::ReplicatedState{aimed}, L"applied");
    const Species::ReplicatedState aimedHeld{aimed};
    const Species::StateRecord cleared = RecordOf(Id(1), baseline, &aimedHeld);
    Assert::IsTrue(cleared.changed == Species::FIGURE_FIELD_TARGET, L"its clearing alone");
    Assert::IsTrue(Species::ApplyRecord(RoundTrip(cleared, std::nullopt), &aimedHeld) == held, L"and the target is gone");
    Assert::IsTrue(Species::RecordBits(cleared, std::nullopt) < Species::RecordBits(target, std::nullopt), L"a cleared target is one bit");
    const Species::ReplicatedState orb{Species::ReplicatedOrb{0, {1, 2, 3}, Species::OrbState::Birth, 4}};
    const Species::ReplicatedState rising{Species::ReplicatedOrb{0, {1, 2, 3}, Species::OrbState::Ascent, 30}};
    const Species::StateRecord ascent = RecordOf(Id(2), rising, &orb);
    Assert::IsTrue(ascent.changed == (Species::ORB_FIELD_STATE | Species::ORB_FIELD_TIMER), L"the orb's state and timer");
    Assert::IsTrue(Species::ApplyRecord(RoundTrip(ascent, std::nullopt), &orb) == rising, L"applied");
    Assert::IsTrue(RecordOf(Id(1), orb, &held).kind == Species::RecordKind::Whole, L"another class goes whole");
    Assert::IsFalse(Species::ApplyRecord(record, &orb).has_value(), L"and a delta never applies to another class");
    Assert::IsFalse(Species::ApplyRecord(record, nullptr).has_value(), L"nor to no baseline");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr139TheSettlementsClassesRoundTripWholeAndAsDeltas)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-097")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr139TheSettlementsClassesRoundTripWholeAndAsDeltas)
  {
    // M2 task T11: a structure, a settlement and a chunk of cells each go whole with every field at its bound, and as a
    // delta of the fields that changed; a Worker's load is one of its flags.
    Species::ReplicatedStructure turret;
    turret.owner = std::uint8_t{3};
    turret.kind = Species::StructureKind::Turret;
    turret.position = {(1 << 23) - 1, -(1 << 19), 17};
    turret.yaw = 1023;
    turret.built = false;
    turret.active = false;
    turret.progress = Species::PROGRESS_PERMILLE_MAX;
    turret.mannedPorts = 0xFFFF;
    turret.turretTeam = std::uint8_t{255};
    turret.turretHealth = 65'535;
    turret.aimYaw = 1023;
    turret.aimPitch = -512;
    Species::ReplicatedStructure site;
    site.kind = Species::StructureKind::ResourceSiteCentral;
    site.position = {100, 200, 300};
    site.mannedPorts = 0x0A5F;
    const Species::ReplicatedSettlement settlement{3, 3, 2, (std::uint64_t{1} << 40) - 1, 1'000'000, 65'535, 300, (1 << 20) - 1};
    const Species::ReplicatedCells cells{Species::CellLayer::Seen, 3, 255, 0, ~std::uint64_t{0}};
    std::optional<Species::StableId> previous;
    for (const auto& [id, state] :
         {std::pair{Id(4), Species::ReplicatedState{turret}}, std::pair{Id(90), Species::ReplicatedState{site}},
          std::pair{Species::SettlementObjectId(3), Species::ReplicatedState{settlement}},
          std::pair{Species::CellChunkObjectId(Species::CellLayer::Seen, 3, 255, 0), Species::ReplicatedState{cells}}})
    {
      const Species::StateRecord record = RecordOf(id, state, nullptr);
      Assert::IsTrue(RoundTrip(record, previous) == record, L"the same whole record");
      Assert::IsTrue(Species::ApplyRecord(record, nullptr) == state, L"it gives its state alone");
      previous = id;
    }
    Assert::IsTrue(Species::OwnerOf(Species::SettlementObjectId(0)) == Species::REPLICATION_OWNER_ID &&
                     Species::SettlementObjectId(3) < Species::CellChunkObjectId(Species::CellLayer::Territory, 0, 0, 0) &&
                     Species::CellChunkObjectId(Species::CellLayer::Territory, 3, 255, 255) <
                       Species::CellChunkObjectId(Species::CellLayer::Seen, 0, 0, 0) &&
                     Id(Species::STABLE_ID_COUNTER_MAX) < Species::SettlementObjectId(0),
                   L"every replication-only id sorts after every simulated object's, and apart from each other");

    // Deltas: a construction site's progress and its manned ports; a turret's aim alone; its team and health.
    const Species::ReplicatedState held{site};
    Species::ReplicatedStructure working = site;
    working.built = false;
    working.progress = 512;
    working.mannedPorts = 0x0001;
    const Species::StateRecord work = RecordOf(Id(90), working, &held);
    Assert::IsTrue(work.changed == (Species::STRUCTURE_FIELD_FLAGS | Species::STRUCTURE_FIELD_PROGRESS | Species::STRUCTURE_FIELD_PORTS),
                   L"the flags, the progress and the ports");
    Assert::IsTrue(Species::ApplyRecord(RoundTrip(work, std::nullopt), &held) == Species::ReplicatedState{working}, L"applied");
    const Species::ReplicatedState heldTurret{turret};
    Species::ReplicatedStructure aimed = turret;
    aimed.aimYaw = 12;
    aimed.aimPitch = 30;
    const Species::StateRecord aim = RecordOf(Id(4), aimed, &heldTurret);
    Assert::IsTrue(aim.changed == Species::STRUCTURE_FIELD_AIM, L"the aim alone");
    Assert::IsTrue(Species::ApplyRecord(RoundTrip(aim, std::nullopt), &heldTurret) == Species::ReplicatedState{aimed}, L"applied");
    Species::ReplicatedStructure unmanned = turret;
    unmanned.turretTeam.reset();
    unmanned.turretHealth = 40;
    const Species::StateRecord left = RecordOf(Id(4), unmanned, &heldTurret);
    Assert::IsTrue(left.changed == Species::STRUCTURE_FIELD_TURRET, L"its team and health");
    Assert::IsTrue(Species::ApplyRecord(RoundTrip(left, std::nullopt), &heldTurret) == Species::ReplicatedState{unmanned}, L"applied");

    // A settlement's stores, then its population and radius; a chunk's cells.
    const Species::ReplicatedState heldSettlement{settlement};
    Species::ReplicatedSettlement richer = settlement;
    richer.storesMilliRu = 12'345;
    const Species::StateRecord stores = RecordOf(Species::SettlementObjectId(3), richer, &heldSettlement);
    Assert::IsTrue(stores.changed == Species::SETTLEMENT_FIELD_STORES, L"the stores alone");
    Assert::IsTrue(Species::ApplyRecord(RoundTrip(stores, std::nullopt), &heldSettlement) == Species::ReplicatedState{richer}, L"applied");
    Species::ReplicatedSettlement grown = settlement;
    grown.population = 21;
    grown.radius = 4'000;
    const Species::StateRecord growth = RecordOf(Species::SettlementObjectId(3), grown, &heldSettlement);
    Assert::IsTrue(growth.changed == (Species::SETTLEMENT_FIELD_POPULATION | Species::SETTLEMENT_FIELD_RADIUS), L"population and radius");
    Assert::IsTrue(Species::ApplyRecord(RoundTrip(growth, std::nullopt), &heldSettlement) == Species::ReplicatedState{grown}, L"applied");
    const Species::ReplicatedState heldCells{cells};
    Species::ReplicatedCells fewer = cells;
    fewer.cells = 0x8000'0000'0000'0001;
    const Species::StateRecord chunk = RecordOf(Species::CellChunkObjectId(Species::CellLayer::Seen, 3, 255, 0), fewer, &heldCells);
    Assert::IsTrue(chunk.changed == Species::CELLS_FIELD_CELLS, L"the cells");
    Assert::IsTrue(Species::ApplyRecord(RoundTrip(chunk, std::nullopt), &heldCells) == Species::ReplicatedState{fewer}, L"applied");
    Assert::IsTrue(Species::RecordBits(chunk, std::nullopt) < Species::RecordBits(RecordOf(Id(1), fewer, nullptr), std::nullopt),
                   L"a chunk's delta is smaller than its whole record");

    // MVP-031: a Worker's load.
    const Species::ReplicatedFigure walking = Worker(3, 10, 10);
    const Species::ReplicatedState heldWorker{walking};
    Species::ReplicatedFigure carrying = walking;
    carrying.loaded = true;
    const Species::StateRecord loaded = RecordOf(Id(7), carrying, &heldWorker);
    Assert::IsTrue(loaded.changed == Species::FIGURE_FIELD_FLAGS, L"a load is a flag");
    Assert::IsTrue(Species::ApplyRecord(RoundTrip(loaded, std::nullopt), &heldWorker) == Species::ReplicatedState{carrying}, L"applied");
    Assert::IsTrue(RoundTrip(RecordOf(Id(7), carrying, nullptr), std::nullopt).state == Species::ReplicatedState{carrying}, L"and whole");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr088AnUpdateFillsDatagramsToTheirLimit)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr088AnUpdateFillsDatagramsToTheirLimit)
  {
    // 2,000 figures whole, as a join of ADR-080's synthetic battle: each datagram within 1,200 bytes, full but for the
    // last, and every record read back in order.
    std::vector<Species::StateRecord> records;
    records.reserve(2000);
    for (std::uint64_t n = 0; n < 2000; ++n)
    {
      records.push_back(RecordOf(Id(1 + n * 3), Worker(static_cast<std::uint8_t>(n % 2), static_cast<std::int32_t>(n * 17), 40), nullptr));
    }
    const std::vector<Species::StateUpdate> updates = Species::PackStateUpdates(77, 0, records);
    Assert::IsTrue(updates.size() > 1, L"more than one datagram");
    std::vector<Species::StateRecord> back;
    for (std::size_t u = 0; u < updates.size(); ++u)
    {
      const std::vector<std::byte> bytes = Neuron::EncodeMessage(updates[u]);
      Assert::IsTrue(bytes.size() <= Neuron::STATE_DATAGRAM_MAX_BYTES, L"within the datagram");
      if (u + 1 < updates.size())
      {
        Assert::IsTrue(bytes.size() * 8 + Species::RecordBits(updates[u + 1].records.front(), updates[u].records.back().id) >
                         Neuron::STATE_DATAGRAM_MAX_BYTES * 8 - 7,
                       L"the next record would not have fitted");
      }
      const std::optional<Species::StateUpdate> decoded = Decoded(bytes);
      Assert::IsTrue(decoded.has_value() && decoded->tick == 77 && decoded->baselineAge == 0, L"each decodes alone");
      const Species::StateUpdate update = decoded.value_or(Species::StateUpdate{});
      back.insert(back.end(), update.records.begin(), update.records.end());
    }
    Assert::IsTrue(back == records, L"every record, in order");
    const Species::StateUpdate empty{78, 0, {}};
    ExpectEqual(13, static_cast<std::int64_t>(Neuron::EncodeMessage(empty).size()), L"an empty update names its tick in 13 bytes");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr088ABrokenUpdateIsDroppedAsMalformed)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr088ABrokenUpdateIsDroppedAsMalformed)
  {
    const Species::ReplicatedState held{Worker(0, 0, 0)};
    const Species::StateRecord delta = RecordOf(Id(4), Worker(0, 16, 0), &held);
    // A delta that names no baseline.
    Neuron::BitWriter writer;
    Neuron::WriteMessageHeader(writer, {Species::StateUpdate::TYPE, Species::StateUpdate::VERSION});
    writer.WriteBits(5, Species::StateUpdate::TICK_BITS);
    writer.WriteBits(0, Species::StateUpdate::BASELINE_AGE_BITS);
    writer.WriteBounded(1, Species::StateUpdate::MAX_RECORDS);
    Species::WriteRecord(writer, delta, std::nullopt);
    Assert::IsFalse(Decoded(writer.Take()).has_value(), L"a delta without a baseline");
    // A baseline before the first tick.
    std::vector<std::byte> bytes = Neuron::EncodeMessage(Species::StateUpdate{5, 6, {delta}});
    Assert::IsFalse(Decoded(bytes).has_value(), L"a baseline before tick 0");
    // A short datagram.
    bytes = Neuron::EncodeMessage(Species::StateUpdate{50, 1, {delta}});
    Assert::IsTrue(Decoded(bytes).has_value(), L"the whole datagram decodes");
    bytes.pop_back();
    Assert::IsFalse(Decoded(bytes).has_value(), L"a short one does not");
    // A delta that changes nothing.
    Species::StateRecord nothing = delta;
    nothing.changed = 0;
    Neuron::BitWriter empty;
    Neuron::WriteMessageHeader(empty, {Species::StateUpdate::TYPE, Species::StateUpdate::VERSION});
    empty.WriteBits(50, Species::StateUpdate::TICK_BITS);
    empty.WriteBits(1, Species::StateUpdate::BASELINE_AGE_BITS);
    empty.WriteBounded(1, Species::StateUpdate::MAX_RECORDS);
    Species::WriteRecord(empty, nothing, std::nullopt);
    Assert::IsFalse(Decoded(empty.Take()).has_value(), L"a delta of no field");
  }
};

TEST_CLASS(StateReplicaTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr084TheReplicaHoldsTheTwoLatestTicks)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr084TheReplicaHoldsTheTwoLatestTicks)
  {
    Species::StateReplica replica;
    const Species::ReplicatedFigure a = Worker(0, 0, 0);
    const Species::ReplicatedFigure b = Worker(1, 160, 0);
    // The join: every object whole.
    replica.Apply({10, 0, {RecordOf(Id(1), a, nullptr), RecordOf(Id(2), b, nullptr)}});
    Assert::IsTrue(replica.LatestTick() == 10 && !replica.PreviousTick().has_value(), L"one tick");
    ExpectEqual(2, static_cast<std::int64_t>(replica.Latest().size()), L"both figures");
    // Tick 11 against tick 10: a moves, b does not and has no record.
    Species::ReplicatedFigure a11 = a;
    a11.position.x = 32;
    const Species::ReplicatedState a10{a};
    replica.Apply({11, 1, {RecordOf(Id(1), a11, &a10)}});
    Assert::IsTrue(replica.LatestTick() == 11 && replica.PreviousTick() == 10, L"two ticks");
    Assert::IsTrue(replica.Latest().at(Id(1)) == Species::ReplicatedState{a11} && replica.Previous().at(Id(1)) == a10, L"a at both");
    Assert::IsTrue(replica.Latest().at(Id(2)) == Species::ReplicatedState{b}, L"b keeps its state");
    // Tick 12: the datagram against tick 11 was lost on the way, so the server sends tick 13 against tick 10, the
    // newest it knows the client holds; b leaves.
    Species::ReplicatedFigure a13 = a;
    a13.position.x = 96;
    replica.Apply({13, 3, {RecordOf(Id(1), a13, &a10), Species::RemovalRecord(Id(2))}});
    Assert::IsTrue(replica.Latest().at(Id(1)) == Species::ReplicatedState{a13}, L"decoded against tick 10");
    Assert::IsTrue(replica.Previous().at(Id(1)) == Species::ReplicatedState{a11}, L"the previous view is tick 11's");
    Assert::IsFalse(replica.Latest().contains(Id(2)) || replica.Previous().contains(Id(2)), L"b is gone");
    ExpectEqual(0, static_cast<std::int64_t>(replica.Totals().missingBaselines), L"every baseline was there");

    // A delta against a tick the replica never received is skipped and counted.
    Species::ReplicatedFigure elsewhere = a;
    elsewhere.position.x = 400;
    replica.Apply({14, 2, {RecordOf(Id(1), elsewhere, &a10)}});
    ExpectEqual(1, static_cast<std::int64_t>(replica.Totals().missingBaselines), L"tick 12 never arrived");
    Assert::IsTrue(replica.Latest().at(Id(1)) == Species::ReplicatedState{a13}, L"a holds");
    // A late update is a baseline only.
    replica.Apply({12, 2, {RecordOf(Id(1), elsewhere, &a10)}});
    ExpectEqual(1, static_cast<std::int64_t>(replica.Totals().lateUpdates), L"late");
    Assert::IsTrue(replica.LatestTick() == 14 && replica.Latest().at(Id(1)) == Species::ReplicatedState{a13}, L"the view stays");
    // An object that enters whole is drawn from its first state: the previous view does not hold it.
    replica.Apply({15, 0, {RecordOf(Id(1), a, nullptr)}});
    Assert::IsFalse(replica.Previous().contains(Id(1)), L"re-entered whole");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr086TheReplicaKeepsItsBaselinesForTheirWindow)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr086TheReplicaKeepsItsBaselinesForTheirWindow)
  {
    Species::StateReplica replica;
    const Species::ReplicatedFigure a = Worker(0, 0, 0);
    const Species::ReplicatedState held{a};
    replica.Apply({100, 0, {RecordOf(Id(1), a, nullptr)}});
    Species::ReplicatedFigure moved = a;
    moved.position.z = 16;
    // Empty updates move the latest tick on; a delta against tick 100 still decodes 31 ticks later.
    for (std::uint64_t tick = 101; tick < 131; ++tick)
    {
      replica.Apply({tick, 0, {}});
    }
    replica.Apply({131, 31, {RecordOf(Id(1), moved, &held)}});
    Assert::IsTrue(replica.Latest().at(Id(1)) == Species::ReplicatedState{moved}, L"within the window");
    ExpectEqual(0, static_cast<std::int64_t>(replica.Totals().missingBaselines), L"no baseline missing");
  }
};

TEST_CLASS(PresentationEventsTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr084EventsRoundTripInTheirOrder)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr084EventsRoundTripInTheirOrder)
  {
    std::vector<Species::EffectEvent> events;
    for (std::uint32_t n = 0; n < 600; ++n)
    {
      Species::EffectEvent event;
      event.kind = n % 3 == 0 ? Species::EffectEventKind::LaserFired : Species::EffectEventKind::FigureDied;
      event.team = static_cast<std::uint8_t>(n % 2);
      event.subject = n % 3 == 0 ? 0 : Id(n);
      event.position = {100.0 + n, 20.5, 300.0};
      event.vector = n % 3 == 0 ? Neuron::Vec3{150.0, 0.0, -0.25} : Neuron::Vec3{};
      event.value = n % 5 == 0 ? 2.5 : 0.0;
      events.push_back(event);
    }
    const std::vector<Species::PresentationEvents> messages = Species::PackPresentationEvents(9, events);
    Assert::IsTrue(messages.size() > 1, L"600 events take more than one datagram");
    std::vector<Species::EffectEvent> back;
    for (const Species::PresentationEvents& message : messages)
    {
      const std::vector<std::byte> bytes = Neuron::EncodeMessage(message);
      Assert::IsTrue(bytes.size() <= Neuron::STATE_DATAGRAM_MAX_BYTES, L"within the datagram");
      Neuron::MessageDispatcher dispatcher;
      std::optional<Species::PresentationEvents> received;
      dispatcher.Register<Species::PresentationEvents>([&received](const Species::PresentationEvents& _events) { received = _events; });
      Assert::IsTrue(dispatcher.Dispatch(bytes) && received && received->tick == 9, L"each decodes alone");
      const Species::PresentationEvents decoded = received.value_or(Species::PresentationEvents{});
      back.insert(back.end(), decoded.events.begin(), decoded.events.end());
    }
    Assert::IsTrue(back.size() == events.size(), L"every event");
    for (std::size_t n = 0; n < events.size(); ++n)
    {
      Assert::IsTrue(back[n].kind == events[n].kind && back[n].team == events[n].team && back[n].subject == events[n].subject &&
                       back[n].position.x == events[n].position.x && back[n].position.y == events[n].position.y &&
                       back[n].vector.x == events[n].vector.x && back[n].vector.z == events[n].vector.z && back[n].value == events[n].value,
                     L"in order, each the same");
    }
    Assert::IsTrue(Species::IsCommandersEvent(Species::EffectEventKind::LeaderSelected) &&
                     !Species::IsCommandersEvent(Species::EffectEventKind::FigureDied),
                   L"selections and orders are the commander's own");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr088AnEventOfNoKindIsMalformed)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr088AnEventOfNoKindIsMalformed)
  {
    Neuron::BitWriter writer;
    Neuron::WriteMessageHeader(writer, {Species::PresentationEvents::TYPE, Species::PresentationEvents::VERSION});
    writer.WriteBits(3, Species::PresentationEvents::TICK_BITS);
    writer.WriteBounded(1, Species::PresentationEvents::MAX_EVENTS);
    writer.WriteBounded(0, Species::EFFECT_EVENT_KIND_MAX);
    writer.WriteBits(0, 8);
    writer.WriteBits(0, 4);
    Neuron::MessageDispatcher dispatcher;
    dispatcher.Register<Species::PresentationEvents>([](const Species::PresentationEvents&) {});
    Assert::IsFalse(dispatcher.Dispatch(writer.Take()), L"dropped");
    ExpectEqual(1, static_cast<std::int64_t>(dispatcher.Drops().malformed), L"as malformed");
  }
};

} // namespace GameSharedTests
