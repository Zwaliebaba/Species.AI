#include "pch.h"

#include "BitWriter.h"
#include "JoinMessages.h"
#include "MessageCodec.h"
#include "TestSupport.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameSharedTests
{

namespace
{

Species::JoinIdentity IdentityOf(std::uint8_t _build, std::uint8_t _content)
{
  Species::JoinIdentity identity;
  identity.build.fill(std::byte{_build});
  identity.content.fill(std::byte{_content});
  identity.content.back() = std::byte{0x5A};
  return identity;
}

} // namespace

TEST_CLASS(JoinMessageTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp071AJoinAndItsAnswerRoundTripAtTheirBounds)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-071")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp071AJoinAndItsAnswerRoundTripAtTheirBounds)
  {
    // ADR-189: the join carries the protocol, the build and the content; the answer the refusal and the server's own.
    const Species::JoinIdentity client = IdentityOf(0x11, 0x22);
    const std::vector<std::byte> join = Neuron::EncodeMessage(Species::JoinCommand{client});
    ExpectEqual(static_cast<std::int64_t>(Species::JoinCommand::MAX_BYTES), static_cast<std::int64_t>(join.size()),
                L"the join at its bound");
    const Species::JoinAnswer refusal{Species::JoinRefusal::Content, IdentityOf(0x11, 0x33)};
    const std::vector<std::byte> answer = Neuron::EncodeMessage(refusal);
    ExpectEqual(static_cast<std::int64_t>(Species::JoinAnswer::MAX_BYTES), static_cast<std::int64_t>(answer.size()),
                L"the answer at its bound");
    std::optional<Species::JoinCommand> readJoin;
    std::optional<Species::JoinAnswer> readAnswer;
    Neuron::MessageDispatcher dispatcher;
    dispatcher.Register<Species::JoinCommand>([&readJoin](const Species::JoinCommand& _join) { readJoin = _join; });
    dispatcher.Register<Species::JoinAnswer>([&readAnswer](const Species::JoinAnswer& _answer) { readAnswer = _answer; });
    Assert::IsTrue(dispatcher.Dispatch(join) && dispatcher.Dispatch(answer), L"both decode");
    Assert::IsTrue(readJoin.has_value() && readJoin->identity == client, L"the join reads back whole");
    Assert::IsTrue(readAnswer.has_value() && readAnswer->refusal == refusal.refusal && readAnswer->server == refusal.server,
                   L"the answer reads back whole");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp071AnAnswerOfAnUnknownRefusalIsMalformed)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-071")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp071AnAnswerOfAnUnknownRefusalIsMalformed)
  {
    Neuron::BitWriter writer;
    Neuron::WriteMessageHeader(writer, {Species::JoinAnswer::TYPE, Species::JoinAnswer::VERSION});
    writer.WriteBounded(Species::JOIN_REFUSAL_MAX, Species::JOIN_REFUSAL_MAX);
    writer.WriteBits(Species::GAME_PROTOCOL_VERSION, 16);
    for (int byte = 0; byte < 64; ++byte)
    {
      writer.WriteBits(0, 8);
    }
    Neuron::MessageDispatcher dispatcher;
    bool received = false;
    dispatcher.Register<Species::JoinAnswer>([&received](const Species::JoinAnswer&) { received = true; });
    Assert::IsFalse(dispatcher.Dispatch(writer.Take()) || received, L"a refusal this build does not know fails the reading");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp071AJoinIsRefusedForItsFirstDifference)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-071")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp071AJoinIsRefusedForItsFirstDifference)
  {
    // ADR-189: the protocol is compared first, then the build, then the content.
    const Species::JoinIdentity server = IdentityOf(1, 2);
    Assert::IsTrue(Species::JoinRefusalOf(server, server) == Species::JoinRefusal::None, L"the same identity joins");
    Species::JoinIdentity other = IdentityOf(3, 4);
    other.protocol = Species::GAME_PROTOCOL_VERSION + 1;
    Assert::IsTrue(Species::JoinRefusalOf(server, other) == Species::JoinRefusal::Protocol, L"another protocol first");
    other.protocol = server.protocol;
    Assert::IsTrue(Species::JoinRefusalOf(server, other) == Species::JoinRefusal::Build, L"then another build");
    other.build = server.build;
    Assert::IsTrue(Species::JoinRefusalOf(server, other) == Species::JoinRefusal::Content, L"then other content");
    Assert::IsTrue(Species::JoinRefusalName(Species::JoinRefusal::Build) == "another build", L"each refusal has its name");
  }
};

} // namespace GameSharedTests
