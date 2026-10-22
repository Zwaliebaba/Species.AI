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
};

} // namespace GameSharedTests
