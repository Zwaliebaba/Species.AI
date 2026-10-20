#include "pch.h"

#include "CommandQueue.h"
#include "TestSupport.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronServerTests
{

namespace
{

constexpr Neuron::SipHashKey TEST_KEY{0x0706050403020100, 0x0f0e0d0c0b0a0908};

// A command that names a target and carries a value; the target 0 is one that no longer exists.
struct TestOrder
{
  static constexpr Neuron::MessageTypeId TYPE = 40;
  static constexpr std::uint8_t VERSION = 2;
  static constexpr std::size_t MAX_BYTES = 16;

  std::uint32_t target = 0;
  std::uint32_t value = 0;

  void Encode(Neuron::BitWriter& _writer) const
  {
    _writer.WriteBits(target, 32);
    _writer.WriteBits(value, 32);
  }
  void Decode(Neuron::BitReader& _reader)
  {
    target = static_cast<std::uint32_t>(_reader.ReadBits(32));
    value = static_cast<std::uint32_t>(_reader.ReadBits(32));
  }
};

// A type that has no handler, and the test order at an older version.
struct UnknownCommand
{
  static constexpr Neuron::MessageTypeId TYPE = 41;
  static constexpr std::uint8_t VERSION = 2;
  static constexpr std::size_t MAX_BYTES = 16;
  void Encode(Neuron::BitWriter&) const {}
  void Decode(Neuron::BitReader&) {}
};
struct OldOrder
{
  static constexpr Neuron::MessageTypeId TYPE = TestOrder::TYPE;
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 16;
  void Encode(Neuron::BitWriter& _writer) const
  {
    _writer.WriteBits(1, 32);
    _writer.WriteBits(1, 32);
  }
  void Decode(Neuron::BitReader&) {}
};

struct Executed
{
  Neuron::SenderId sender = 0;
  std::uint32_t value = 0;
  std::uint64_t tick = 0;
};

std::vector<std::byte> Order(std::uint32_t _value, std::uint32_t _target = 1)
{
  return Neuron::EncodeMessage(TestOrder{_target, _value});
}

// A queue whose handler records what executed; the sender 99 may not command at all.
struct Fixture
{
  explicit Fixture(std::uint64_t _nextTick = 0)
    : queue(TEST_KEY, _nextTick)
  {
    queue.Register<TestOrder>([](Neuron::SenderId _sender, const TestOrder&) { return _sender != 99; },
                              [this](Neuron::SenderId _sender, const TestOrder& _order, std::uint64_t _tick)
                              {
                                if (_order.target == 0)
                                {
                                  return Neuron::CommandEffect::NoEffect;
                                }
                                executed.push_back({_sender, _order.value, _tick});
                                return Neuron::CommandEffect::Applied;
                              });
  }

  Neuron::CommandQueue queue;
  std::vector<Executed> executed;
};

bool RejectedWith(const std::vector<Neuron::CommandOutcome>& _outcomes, std::uint64_t _sequence, Neuron::CommandRejection _reason)
{
  for (const Neuron::CommandOutcome& outcome : _outcomes)
  {
    if (outcome.sequence == _sequence)
    {
      return outcome.rejection == _reason;
    }
  }
  return false;
}

} // namespace

TEST_CLASS(CommandQueueTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp094ACommandExecutesInTheNextTick)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-094")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp094ACommandExecutesInTheNextTick)
  {
    Fixture fixture(5);
    Assert::IsTrue(fixture.queue.Submit(1, 0, Order(10)));
    ExpectEqual(1, static_cast<std::int64_t>(fixture.queue.Pending()), L"count");
    Assert::IsTrue(fixture.executed.empty(), L"nothing executes on arrival");
    fixture.queue.Execute(5);
    ExpectEqual(1, static_cast<std::int64_t>(fixture.executed.size()), L"count");
    ExpectEqual(5, static_cast<std::int64_t>(fixture.executed[0].tick), L"the tick it executed in");
    const std::vector<Neuron::CommandOutcome> outcomes = fixture.queue.TakeOutcomes();
    ExpectEqual(1, static_cast<std::int64_t>(outcomes.size()), L"count");
    Assert::IsFalse(outcomes[0].rejection.has_value());
    Assert::IsTrue(outcomes[0].effect == Neuron::CommandEffect::Applied);
    ExpectEqual(5, static_cast<std::int64_t>(outcomes[0].tick), L"the outcome's tick");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp094ACommandIssuedDuringATickExecutesInTheNext)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-094")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp094ACommandIssuedDuringATickExecutesInTheNext)
  {
    // An AI settlement or the deputy issues its orders while the tick runs; they wait for the next tick like a
    // steward's (ADR-087 item 1).
    Neuron::CommandQueue queue(TEST_KEY, 0);
    std::vector<std::uint64_t> ticks;
    queue.Register<TestOrder>({},
                              [&](Neuron::SenderId, const TestOrder& _order, std::uint64_t _tick)
                              {
                                ticks.push_back(_tick);
                                if (_order.value == 1)
                                {
                                  Assert::IsTrue(queue.Submit(7, 1, Order(2)));
                                }
                                return Neuron::CommandEffect::Applied;
                              });
    Assert::IsTrue(queue.Submit(7, 0, Order(1)));
    queue.Execute(0);
    queue.Execute(1);
    ExpectEqual(2, static_cast<std::int64_t>(ticks.size()), L"count");
    ExpectEqual(0, static_cast<std::int64_t>(ticks[0]), L"the first order");
    ExpectEqual(1, static_cast<std::int64_t>(ticks[1]), L"the order issued during tick 0");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp094SendersExecuteInTheKeyedOrderOfTheirTick)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-094")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp094SendersExecuteInTheKeyedOrderOfTheirTick)
  {
    // Two senders whose commands reach the same tick win a contested action in shares within 45 % to 55 % over 1,000
    // ticks (ADR-087 confirmation), whatever their ids.
    Fixture fixture;
    std::uint64_t firstWins = 0;
    for (std::uint64_t tick = 0; tick < 1000; ++tick)
    {
      Assert::IsTrue(fixture.queue.Submit(2, tick, Order(2)));
      Assert::IsTrue(fixture.queue.Submit(1, tick, Order(1)));
      fixture.executed.clear();
      fixture.queue.Execute(tick);
      ExpectEqual(2, static_cast<std::int64_t>(fixture.executed.size()), L"count");
      const bool oneFirst = Neuron::SenderOrderValue(TEST_KEY, tick, 1) < Neuron::SenderOrderValue(TEST_KEY, tick, 2);
      ExpectEqual(oneFirst ? 1 : 2, static_cast<std::int64_t>(fixture.executed[0].sender), L"the first sender is the keyed one");
      firstWins += fixture.executed[0].sender == 1 ? 1 : 0;
    }
    Assert::IsTrue(firstWins >= 450 && firstWins <= 550, L"sender 1 goes first in 45 % to 55 % of ticks");
    Assert::IsTrue(Neuron::SenderOrderValue(TEST_KEY, 0, 1) != Neuron::SenderOrderValue({1, 2}, 0, 1), L"another key gives another order");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp094ASendersCommandsKeepTheirOrder)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-094")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp094ASendersCommandsKeepTheirOrder)
  {
    Fixture fixture;
    for (std::uint32_t i = 0; i < 6; ++i)
    {
      Assert::IsTrue(fixture.queue.Submit(i % 2 == 0 ? 3 : 4, i, Order(i)));
    }
    fixture.queue.Execute(9);
    ExpectEqual(6, static_cast<std::int64_t>(fixture.executed.size()), L"count");
    // One sender's three commands, then the other's, each in the order they arrived.
    for (std::size_t n = 0; n < 6; ++n)
    {
      Assert::IsTrue(fixture.executed[n / 3 * 3].sender == fixture.executed[n].sender, L"a sender's commands run together");
    }
    for (const std::size_t first : {std::size_t{0}, std::size_t{3}})
    {
      Assert::IsTrue(fixture.executed[first].value < fixture.executed[first + 1].value &&
                       fixture.executed[first + 1].value < fixture.executed[first + 2].value,
                     L"in arrival order");
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp094ArrivalChecksDropWithAReason)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-094")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp094ArrivalChecksDropWithAReason)
  {
    Fixture fixture(3);
    std::vector<std::byte> trailing = Order(5);
    trailing.push_back(std::byte{0x01}); // a byte after the last field
    std::vector<std::byte> oversized = Order(5);
    oversized.resize(TestOrder::MAX_BYTES + 1);
    std::vector<std::byte> truncated = Order(5);
    truncated.resize(4);

    Assert::IsFalse(fixture.queue.Submit(1, 1, Neuron::EncodeMessage(UnknownCommand{})));
    Assert::IsFalse(fixture.queue.Submit(1, 2, Neuron::EncodeMessage(OldOrder{})));
    Assert::IsFalse(fixture.queue.Submit(1, 3, oversized));
    Assert::IsFalse(fixture.queue.Submit(1, 4, trailing));
    Assert::IsFalse(fixture.queue.Submit(1, 5, truncated));
    Assert::IsFalse(fixture.queue.Submit(99, 6, Order(5)));
    Assert::IsTrue(fixture.queue.Submit(1, 7, Order(5)));
    Assert::IsFalse(fixture.queue.Submit(1, 7, Order(6)));
    Assert::IsFalse(fixture.queue.Submit(1, 8, {}));

    const std::vector<Neuron::CommandOutcome> outcomes = fixture.queue.TakeOutcomes();
    Assert::IsTrue(RejectedWith(outcomes, 1, Neuron::CommandRejection::UnknownType), L"an unknown type");
    Assert::IsTrue(RejectedWith(outcomes, 2, Neuron::CommandRejection::WrongVersion), L"another version");
    Assert::IsTrue(RejectedWith(outcomes, 3, Neuron::CommandRejection::Oversized), L"above the type's limit");
    Assert::IsTrue(RejectedWith(outcomes, 4, Neuron::CommandRejection::Malformed), L"a byte left over");
    Assert::IsTrue(RejectedWith(outcomes, 5, Neuron::CommandRejection::Malformed), L"a missing field");
    Assert::IsTrue(RejectedWith(outcomes, 6, Neuron::CommandRejection::NotPermitted), L"a sender without the right");
    Assert::IsTrue(RejectedWith(outcomes, 7, Neuron::CommandRejection::Duplicate), L"a sequence number used twice");
    Assert::IsTrue(RejectedWith(outcomes, 8, Neuron::CommandRejection::Malformed), L"no header");
    for (const Neuron::CommandOutcome& outcome : outcomes)
    {
      ExpectEqual(3, static_cast<std::int64_t>(outcome.tick), L"a refusal names the tick it would have executed in");
    }
    const Neuron::CommandRejectionCounts& counts = fixture.queue.Rejections();
    ExpectEqual(8, static_cast<std::int64_t>(counts.Total()), L"rejections counted");
    ExpectEqual(3, static_cast<std::int64_t>(counts.malformed), L"malformed");

    fixture.queue.Execute(3);
    ExpectEqual(1, static_cast<std::int64_t>(fixture.executed.size()), L"only the valid command executes");
    Assert::AreEqual(5u, fixture.executed[0].value, L"the first of the two with one sequence number");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp094ACommandWhoseTargetHasGoneDoesNothing)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-094")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp094ACommandWhoseTargetHasGoneDoesNothing)
  {
    Fixture fixture;
    Assert::IsTrue(fixture.queue.Submit(1, 0, Order(5, 0)));
    fixture.queue.Execute(0);
    Assert::IsTrue(fixture.executed.empty());
    const std::vector<Neuron::CommandOutcome> outcomes = fixture.queue.TakeOutcomes();
    ExpectEqual(1, static_cast<std::int64_t>(outcomes.size()), L"count");
    Assert::IsFalse(outcomes[0].rejection.has_value(), L"it executed");
    Assert::IsTrue(outcomes[0].effect == Neuron::CommandEffect::NoEffect, L"with no effect");
  }
};

} // namespace NeuronServerTests
