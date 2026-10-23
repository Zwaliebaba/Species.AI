#include "pch.h"

#include "BitWriter.h"
#include "MessageCodec.h"
#include "RelationMessages.h"
#include "TestSupport.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameSharedTests
{

namespace
{

// What the bytes decode to as a T, or none when they do not decode whole.
template <class T> std::optional<T> Decoded(const std::vector<std::byte>& _bytes)
{
  Neuron::MessageDispatcher dispatcher;
  std::optional<T> received;
  dispatcher.Register<T>([&received](const T& _message) { received = _message; });
  return dispatcher.Dispatch(_bytes) ? received : std::nullopt;
}

// A message of type T's header and the fields _write writes.
template <class T, class Fn> std::vector<std::byte> Forged(Fn _write)
{
  T bound;
  std::vector<std::byte> bytes = Neuron::EncodeMessage(bound);
  Neuron::BitWriter fields;
  _write(fields);
  bytes.resize(3);
  bytes.insert(bytes.end(), fields.Bytes().begin(), fields.Bytes().end());
  return bytes;
}

} // namespace

TEST_CLASS(RelationMessageTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp053TheRelationsCommandsRoundTripAtTheirBounds)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-053")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-054")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-101")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp053TheRelationsCommandsRoundTripAtTheirBounds)
  {
    // ADR-160: each command at the largest values of its fields is exactly its bound, and reads back as it was written.
    const Species::MakeClaimCommand claim{255, Species::CLAIM_AREA_CENTRAL};
    const std::vector<std::byte> claimBytes = Neuron::EncodeMessage(claim);
    ExpectEqual(static_cast<std::int64_t>(Species::MakeClaimCommand::MAX_BYTES), static_cast<std::int64_t>(claimBytes.size()), L"a claim");
    const auto claimRead = Decoded<Species::MakeClaimCommand>(claimBytes);
    Assert::IsTrue(claimRead && claimRead->team == 255 && claimRead->area == Species::CLAIM_AREA_CENTRAL, L"its fields");

    const Species::WithdrawClaimCommand withdrawal{255, std::numeric_limits<std::uint64_t>::max()};
    const std::vector<std::byte> withdrawalBytes = Neuron::EncodeMessage(withdrawal);
    ExpectEqual(static_cast<std::int64_t>(Species::WithdrawClaimCommand::MAX_BYTES), static_cast<std::int64_t>(withdrawalBytes.size()),
                L"a withdrawal");
    const auto withdrawalRead = Decoded<Species::WithdrawClaimCommand>(withdrawalBytes);
    Assert::IsTrue(withdrawalRead && withdrawalRead->claim == withdrawal.claim, L"its claim");

    const Species::SendTributeCommand tribute{255, 3, 2, std::numeric_limits<std::uint64_t>::max(),
                                              std::numeric_limits<std::int64_t>::max()};
    const std::vector<std::byte> tributeBytes = Neuron::EncodeMessage(tribute);
    ExpectEqual(static_cast<std::int64_t>(Species::SendTributeCommand::MAX_BYTES), static_cast<std::int64_t>(tributeBytes.size()),
                L"a tribute");
    const auto tributeRead = Decoded<Species::SendTributeCommand>(tributeBytes);
    Assert::IsTrue(tributeRead && tributeRead->to == 3 && tributeRead->cause == 2 && tributeRead->subject == tribute.subject &&
                     tributeRead->amountMilliRu == tribute.amountMilliRu,
                   L"its fields");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp041TheAttackCommandsRoundTripAtTheirBounds)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-041")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-045")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-094")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp041TheAttackCommandsRoundTripAtTheirBounds)
  {
    // ADR-161: a declaration of three Leaders at the largest values of its fields, and a withdrawal, are exactly their
    // bounds and read back as they were written; a declaration cut short does not decode.
    constexpr std::uint64_t LARGEST = std::numeric_limits<std::uint64_t>::max();
    const Species::DeclareAttackCommand declaration{255, {LARGEST, LARGEST - 1, LARGEST - 2}, 3, LARGEST};
    const std::vector<std::byte> declarationBytes = Neuron::EncodeMessage(declaration);
    ExpectEqual(static_cast<std::int64_t>(Species::DeclareAttackCommand::MAX_BYTES), static_cast<std::int64_t>(declarationBytes.size()),
                L"a declaration");
    const auto declarationRead = Decoded<Species::DeclareAttackCommand>(declarationBytes);
    Assert::IsTrue(declarationRead && declarationRead->team == 255 && declarationRead->leaders == declaration.leaders &&
                     declarationRead->target == 3 && declarationRead->claim == LARGEST,
                   L"its fields");
    const Species::WithdrawAttackCommand withdrawal{255};
    const std::vector<std::byte> withdrawalBytes = Neuron::EncodeMessage(withdrawal);
    ExpectEqual(static_cast<std::int64_t>(Species::WithdrawAttackCommand::MAX_BYTES), static_cast<std::int64_t>(withdrawalBytes.size()),
                L"a withdrawal");
    const auto withdrawalRead = Decoded<Species::WithdrawAttackCommand>(withdrawalBytes);
    Assert::IsTrue(withdrawalRead && withdrawalRead->team == 255, L"its team");
    std::vector<std::byte> cut = declarationBytes;
    cut.resize(cut.size() - 2);
    Assert::IsFalse(Decoded<Species::DeclareAttackCommand>(cut).has_value(), L"a declaration cut short");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp094ARelationsCommandOutOfItsBoundsIsMalformed)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-094")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp094ARelationsCommandOutOfItsBoundsIsMalformed)
  {
    // ADR-160: an area beyond the central resource site, a withdrawal of claim 0 and a negative tribute do not decode.
    Assert::IsFalse(Decoded<Species::MakeClaimCommand>(Forged<Species::MakeClaimCommand>(
                                                         [](Neuron::BitWriter& _out)
                                                         {
                                                           _out.WriteBits(2, 8);
                                                           _out.WriteBits(Species::CLAIM_AREA_MAX + 1,
                                                                          Neuron::BitsFor(Species::CLAIM_AREA_MAX));
                                                         }))
                      .has_value(),
                    L"an area out of the region's");
    Assert::IsFalse(Decoded<Species::WithdrawClaimCommand>(Neuron::EncodeMessage(Species::WithdrawClaimCommand{2, 0})).has_value(),
                    L"claim 0, which is no stable id");
    Assert::IsFalse(Decoded<Species::SendTributeCommand>(Neuron::EncodeMessage(Species::SendTributeCommand{2, 0, 2, 1, -1})).has_value(),
                    L"a negative amount");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp098TheValleyStatusRoundTripsAtItsBound)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-098")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-102")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp098TheValleyStatusRoundTripsAtItsBound)
  {
    // ADR-165: the request reads back, and a status with every list at its bound and every field at its largest is
    // exactly its bound and reads back as it was written; a claim or an expedition of id 0 does not decode.
    const std::optional<Species::ReadValleyStatusCommand> request =
      Decoded<Species::ReadValleyStatusCommand>(Neuron::EncodeMessage(Species::ReadValleyStatusCommand{255}));
    Assert::IsTrue(request && request->team == 255 && Neuron::EncodeMessage(*request).size() == Species::ReadValleyStatusCommand::MAX_BYTES,
                   L"the request");
    constexpr std::uint64_t MAX = std::numeric_limits<std::uint64_t>::max();
    constexpr std::uint32_t MAX32 = std::numeric_limits<std::uint32_t>::max();
    const Species::QuantizedPosition edge{(1 << 23) - 1, -(1 << 19), -(1 << 23)};
    Species::ValleyStatus status;
    status.tick = MAX;
    status.team = 255;
    status.runEndTick = MAX;
    status.quietTick = MAX - 1;
    status.runEnded = true;
    for (std::uint8_t site = 0; site < Species::VALLEY_SETTLEMENTS_MAX; ++site)
    {
      status.renown.push_back({site, MAX32, MAX32 - 1, MAX32 - 2, MAX32 - 3});
      status.expeditions.push_back({MAX - site, 3, site, MAX, MAX, Species::ExpeditionState::Fighting, true, MAX32});
      status.battles.push_back(
        {MAX - site, site, 3, MAX, MAX, true, edge, edge, (1U << Species::SETTLEMENT_RADIUS_BITS) - 1, MAX32, MAX32, MAX32});
      status.respites.push_back({site, MAX});
    }
    for (std::uint8_t area = 0; area < Species::VALLEY_AREAS; ++area)
    {
      status.areas[area] = {std::uint8_t{3}, MAX - area};
    }
    status.areas[4].holder.reset();
    for (std::uint64_t n = 0; n < Species::VALLEY_CLAIMS_MAX; ++n)
    {
      status.claims.push_back({MAX - n, 3, static_cast<std::uint8_t>(Species::CLAIM_AREA_MAX), MAX, Species::ClaimState::Contested});
    }
    for (std::uint64_t n = 0; n < Species::VALLEY_GRIEVANCES_MAX; ++n)
    {
      status.grievances.push_back({3, 2, Species::GrievanceCause::Attack, MAX - n, true, true, std::numeric_limits<std::int64_t>::max()});
    }
    const std::vector<std::byte> bytes = Neuron::EncodeMessage(status);
    ExpectEqual(static_cast<std::int64_t>(Species::ValleyStatus::MAX_BYTES), static_cast<std::int64_t>(bytes.size()), L"at its bound");
    const std::optional<Species::ValleyStatus> read = Decoded<Species::ValleyStatus>(bytes);
    Assert::IsTrue(read && read->tick == MAX && read->team == 255 && read->runEndTick == MAX && read->quietTick == MAX - 1 &&
                     read->runEnded && read->renown == status.renown && read->areas == status.areas && read->claims == status.claims &&
                     read->grievances == status.grievances && read->expeditions == status.expeditions && read->battles == status.battles &&
                     read->respites == status.respites,
                   L"every field");
    Species::ValleyStatus nameless;
    nameless.claims.push_back({0, 1, 2, 3, Species::ClaimState::Open});
    Assert::IsFalse(Decoded<Species::ValleyStatus>(Neuron::EncodeMessage(nameless)).has_value(), L"a claim of id 0");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp048ABattleResultRoundTripsAtItsBound)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-048")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp048ABattleResultRoundTripsAtItsBound)
  {
    // ADR-165: a result at its largest is exactly its bound and reads back whole; a raid that moved an area, a battle that
    // ended before it started, and expedition 0 do not decode.
    constexpr std::uint64_t MAX = std::numeric_limits<std::uint64_t>::max();
    constexpr std::uint32_t MAX32 = std::numeric_limits<std::uint32_t>::max();
    Species::BattleResultNotice result;
    result.expedition = MAX;
    result.attacker = 3;
    result.defender = 2;
    result.area = static_cast<std::uint8_t>(Species::CLAIM_AREA_MAX);
    result.end = Species::BattleEnd::Time;
    result.attackerWon = true;
    result.startTick = MAX - 1;
    result.endTick = MAX;
    result.areaMoved = true;
    result.side = true;
    result.attackerLost = MAX32;
    result.defenderLost = MAX32 - 1;
    result.storesMovedMilliRu = std::numeric_limits<std::int64_t>::max();
    result.essenceMoved = MAX32;
    result.grievanceMicro = std::numeric_limits<std::int64_t>::max() - 1;
    result.respiteUntil = MAX;
    result.renown = MAX32;
    const std::vector<std::byte> bytes = Neuron::EncodeMessage(result);
    ExpectEqual(static_cast<std::int64_t>(Species::BattleResultNotice::MAX_BYTES), static_cast<std::int64_t>(bytes.size()),
                L"at its bound");
    const std::optional<Species::BattleResultNotice> read = Decoded<Species::BattleResultNotice>(bytes);
    Assert::IsTrue(read && read->expedition == MAX && read->attacker == 3 && read->defender == 2 && read->area == result.area &&
                     read->end == Species::BattleEnd::Time && read->attackerWon && read->startTick == MAX - 1 && read->endTick == MAX &&
                     read->areaMoved && read->side && read->attackerLost == MAX32 && read->defenderLost == MAX32 - 1 &&
                     read->storesMovedMilliRu == result.storesMovedMilliRu && read->essenceMoved == MAX32 &&
                     read->grievanceMicro == result.grievanceMicro && read->respiteUntil == MAX && read->renown == MAX32,
                   L"every field");
    Species::BattleResultNotice raid = result;
    raid.area.reset();
    Assert::IsFalse(Decoded<Species::BattleResultNotice>(Neuron::EncodeMessage(raid)).has_value(), L"a raid that moved an area");
    raid.areaMoved = false;
    Assert::IsTrue(Decoded<Species::BattleResultNotice>(Neuron::EncodeMessage(raid)).has_value(), L"a raid");
    raid.endTick = raid.startTick - 1;
    Assert::IsFalse(Decoded<Species::BattleResultNotice>(Neuron::EncodeMessage(raid)).has_value(), L"an end before the start");
    raid.endTick = MAX;
    raid.expedition = 0;
    Assert::IsFalse(Decoded<Species::BattleResultNotice>(Neuron::EncodeMessage(raid)).has_value(), L"expedition 0");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp063TheEpilogueRoundTripsAtItsBound)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-063")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-062")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp063TheEpilogueRoundTripsAtItsBound)
  {
    // ADR-169: the request and the epilogue at their largest are exactly their bounds and read back whole; a strength
    // below zero does not decode.
    constexpr std::uint64_t MAX = std::numeric_limits<std::uint64_t>::max();
    constexpr std::uint32_t MAX32 = std::numeric_limits<std::uint32_t>::max();
    const std::optional<Species::ReadEpilogueCommand> request =
      Decoded<Species::ReadEpilogueCommand>(Neuron::EncodeMessage(Species::ReadEpilogueCommand{255}));
    Assert::IsTrue(request && request->team == 255 && Neuron::EncodeMessage(*request).size() == Species::ReadEpilogueCommand::MAX_BYTES,
                   L"the request");
    Species::Epilogue epilogue;
    epilogue.tick = MAX;
    epilogue.team = 255;
    epilogue.runEndTick = MAX - 1;
    epilogue.runEnded = true;
    for (std::uint8_t site = 0; site < Species::VALLEY_SETTLEMENTS_MAX; ++site)
    {
      Species::EpilogueLine line;
      line.site = static_cast<std::uint8_t>(3 - site);
      line.renown = {line.site, MAX32, MAX32 - 1, MAX32 - 2, MAX32 - 3};
      line.battlesWon = MAX32;
      line.battlesLost = 1;
      line.claimsMade = 2;
      line.claimsWon = 3;
      line.claimsLost = 4;
      line.claimsWithdrawn = 5;
      line.structures = 6;
      line.grievances = 7;
      line.workersMost = 300;
      line.workersLast = 299;
      line.strengthMilliRu.assign(Species::EPILOGUE_SAMPLES_MAX, std::numeric_limits<std::int64_t>::max());
      epilogue.settlements.push_back(line);
    }
    epilogue.growthSteps = MAX32;
    epilogue.largestGapTicks = MAX;
    epilogue.firstHalfwayTick = MAX - 2;
    const std::vector<std::byte> bytes = Neuron::EncodeMessage(epilogue);
    ExpectEqual(static_cast<std::int64_t>(Species::Epilogue::MAX_BYTES), static_cast<std::int64_t>(bytes.size()), L"at its bound");
    const std::optional<Species::Epilogue> read = Decoded<Species::Epilogue>(bytes);
    Assert::IsTrue(read && read->tick == MAX && read->team == 255 && read->runEndTick == MAX - 1 && read->runEnded &&
                     read->settlements == epilogue.settlements && read->growthSteps == MAX32 && read->largestGapTicks == MAX &&
                     read->firstHalfwayTick == MAX - 2,
                   L"every field");
    Species::Epilogue negative = epilogue;
    negative.settlements[1].strengthMilliRu[7] = -1;
    Assert::IsFalse(Decoded<Species::Epilogue>(Neuron::EncodeMessage(negative)).has_value(), L"a strength below zero");
  }
};

} // namespace GameSharedTests
