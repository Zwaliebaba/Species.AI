#include "pch.h"

#include "AwayMessages.h"
#include "CommandMessages.h"
#include "JoinMessages.h"
#include "MessageCodec.h"
#include "PresentationEvents.h"
#include "RelationMessages.h"
#include "ServerStats.h"
#include "SettlementMessages.h"
#include "StateUpdate.h"
#include "TestSupport.h"
#include "WorldTerrain.h"

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameSharedTests
{

namespace
{

// The inputs each message's parser is given in CI; the owner's run of ADR-088's confirmation gives 10^7 (M4 task T11).
constexpr std::uint64_t INPUTS_PER_MESSAGE = 4'000;

// A xorshift generator of the test's own, so that a failing input is found again from its seed.
class FuzzRandom
{
public:
  explicit FuzzRandom(std::uint64_t _seed) noexcept
    : m_state(_seed | 1U)
  {
  }
  std::uint64_t operator()() noexcept
  {
    m_state ^= m_state << 13U;
    m_state ^= m_state >> 7U;
    m_state ^= m_state << 17U;
    return m_state;
  }

private:
  std::uint64_t m_state;
};

// ADR-088 item 5: _inputs inputs to T's parser, each either delivered or dropped with a reason, never fatal: half are a
// header of T's type and version over random bytes of up to its bound and a little more, half a valid T with one bit
// flipped. The count of what was delivered and what was dropped.
template <class T> std::uint64_t Fuzz(FuzzRandom& _random, std::uint64_t _inputs)
{
  Neuron::MessageDispatcher dispatcher;
  std::uint64_t delivered = 0;
  dispatcher.Register<T>([&delivered](const T&) { ++delivered; });
  const std::vector<std::byte> valid = Neuron::EncodeMessage(T{});
  const std::vector<std::byte> header(valid.begin(), valid.begin() + 3);
  for (std::uint64_t input = 0; input < _inputs; ++input)
  {
    std::vector<std::byte> bytes;
    if (input % 2 == 0)
    {
      bytes = header;
      bytes.resize(header.size() + _random() % (T::MAX_BYTES + 8));
      for (std::size_t index = header.size(); index < bytes.size(); ++index)
      {
        bytes[index] = static_cast<std::byte>(_random() & 0xFFU);
      }
    }
    else
    {
      bytes = valid;
      const std::size_t bit = _random() % (bytes.size() * 8);
      bytes[bit / 8] ^= static_cast<std::byte>(1U << (bit % 8));
    }
    static_cast<void>(dispatcher.Dispatch(bytes));
  }
  return delivered + dispatcher.Drops().Total();
}

template <class... Ts> void FuzzEach(FuzzRandom& _random)
{
  ((ExpectEqual(static_cast<std::int64_t>(INPUTS_PER_MESSAGE), static_cast<std::int64_t>(Fuzz<Ts>(_random, INPUTS_PER_MESSAGE)),
                L"each input delivered or dropped with its reason")),
   ...);
}

} // namespace

TEST_CLASS(MessageFuzzTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp090EveryGameMessagesParserTakesAnyInput)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-071")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp090EveryGameMessagesParserTakesAnyInput)
  {
    // ADR-088 item 5, ADR-069: every game message's parser, the client's and the server's alike, is given random and
    // mutated inputs, and none is fatal.
    FuzzRandom random(0x5EC1E5);
    FuzzEach<Species::PresenceReportCommand, Species::ReadAwayReportCommand, Species::AwayReport, Species::CommandReport,
             Species::SelectCommand, Species::PromoteWorkerCommand, Species::DemoteLeaderCommand, Species::OrderUnitCommand,
             Species::ToggleUnitCommand, Species::SelectWorkersCommand, Species::OrderWorkersCommand, Species::ControlStateCommand,
             Species::AiOrderWorkerCommand, Species::AiOrderLeaderCommand, Species::AiLeaderFieldsCommand, Species::SandboxRestartCommand,
             Species::JoinCommand, Species::JoinAnswer, Species::PresentationEvents, Species::MakeClaimCommand,
             Species::WithdrawClaimCommand, Species::DeclareAttackCommand, Species::WithdrawAttackCommand, Species::SendTributeCommand,
             Species::ReadValleyStatusCommand, Species::ValleyStatus, Species::BattleResultNotice, Species::ReadEpilogueCommand,
             Species::Epilogue, Species::ServerStats, Species::PlaceStructureCommand, Species::RebuildTurretCommand,
             Species::SetPrioritiesCommand, Species::HoldGrowthCommand, Species::ReadSettlementStatusCommand, Species::SettlementStatus,
             Species::SettlementNotice, Species::StateUpdate, Species::WorldTerrain>(random);
  }
};

} // namespace GameSharedTests
