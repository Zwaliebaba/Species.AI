#include "pch.h"

#include "ClientSession.h"
#include "InProcessTransport.h"
#include "RegionTerrain.h"
#include "TestSupport.h"
#include "TickReport.h"
#include "WorldTerrain.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameClientTests
{

namespace
{

// The region of seed 1 as this client builds it: what an honest server sends.
const Species::RegionTerrain& SeedOne()
{
  static const Species::RegionTerrain REGION = []
  {
    auto generated = Species::GenerateRegionTerrain(1, CommittedTuning());
    Assert::IsTrue(generated.has_value(), L"seed 1 gives a region that passes");
    return generated ? std::move(*generated) : Species::RegionTerrain{};
  }();
  return REGION;
}

} // namespace

TEST_CLASS(ClientSessionTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp090TheSessionShowsTheServersLatestTick)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp090TheSessionShowsTheServersLatestTick)
  {
    auto [client, server] = Neuron::InProcessTransport::CreatePair({});
    std::vector<std::string> titles;
    Species::ClientSession::Desc desc;
    desc.transport = std::move(client);
    desc.showTitle = [&titles](std::string_view _title) { titles.emplace_back(_title); };
    desc.productName = "Species";
    desc.tuning = &CommittedTuning();
    Species::ClientSession session(std::move(desc));
    session.Update();
    Assert::IsFalse(session.ServerTick().has_value(), L"no report yet");
    Assert::IsTrue(titles.empty(), L"the title waits for the first report");
    Assert::IsTrue(session.Connected());

    for (std::uint64_t tick = 10; tick < 13; ++tick)
    {
      Assert::IsTrue(server->Send(Neuron::Channel::State, Neuron::EncodeMessage(Species::TickReport{tick})).has_value());
    }
    session.Update();
    Assert::IsTrue(session.ServerTick() == std::optional<std::uint64_t>{12}, L"the latest of the reports that arrived");
    Assert::IsTrue(titles.size() == 1 && titles[0] == "Species \u2014 tick 12", L"one title a frame, with the latest tick");

    // A message the session cannot read is counted and changes nothing.
    const std::vector<std::byte> garbage{std::byte{0xff}, std::byte{0xff}, std::byte{0x01}};
    Assert::IsTrue(server->Send(Neuron::Channel::Reliable, garbage).has_value());
    session.Update();
    Assert::IsTrue(session.ServerTick() == std::optional<std::uint64_t>{12});
    ExpectEqual(1, static_cast<std::int64_t>(titles.size()), L"an unchanged tick leaves the title");
    ExpectEqual(1, static_cast<std::int64_t>(session.Drops().Total()), L"drops");

    session.Close();
    Assert::IsFalse(session.Connected(), L"closed");
    Assert::IsTrue(server->State() == Neuron::ConnectionState::Closed, L"the server's end sees the close");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp090TheTitleNamesTheTick)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp090TheTitleNamesTheTick)
  {
    Assert::IsTrue(Species::SessionTitle("Species", std::nullopt, std::nullopt) == "Species", L"the product alone before a report");
    Assert::IsTrue(Species::SessionTitle("Species", std::nullopt, 1234) == "Species — tick 1234", L"the product and the tick");
    Assert::IsTrue(Species::SessionTitle("Species", 7, 1234) == "Species — seed 7 — tick 1234", L"the seed of MVP-005 too");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp011TheClientBuildsTheTerrainTheServerSent)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-011")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp011TheClientBuildsTheTerrainTheServerSent)
  {
    // The server's seed and digest; the client builds the region with its own library and data, and its digest matches.
    auto [client, server] = Neuron::InProcessTransport::CreatePair({});
    std::vector<std::string> titles;
    Species::ClientSession::Desc desc;
    desc.transport = std::move(client);
    desc.showTitle = [&titles](std::string_view _title) { titles.emplace_back(_title); };
    desc.productName = "Species";
    desc.tuning = &CommittedTuning();
    Species::ClientSession session(std::move(desc));
    const Species::WorldTerrain sent{SeedOne().seed, SeedOne().digest};
    Assert::IsTrue(server->Send(Neuron::Channel::Reliable, Neuron::EncodeMessage(sent)).has_value());
    Assert::IsTrue(server->Send(Neuron::Channel::State, Neuron::EncodeMessage(Species::TickReport{3})).has_value());
    session.Update();
    Assert::IsTrue(session.Terrain() != nullptr && session.Terrain()->digest == SeedOne().digest, L"the client's terrain");
    Assert::IsFalse(session.Refused(), L"accepted");
    Assert::IsTrue(session.Connected(), L"still connected");
    Assert::IsTrue(!titles.empty() && titles.back() == "Species — seed 1 — tick 3", L"the seed is shown");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp011AClientRefusesATerrainWhoseDigestDiffers)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-011")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp011AClientRefusesATerrainWhoseDigestDiffers)
  {
    // A server whose library or data differs sends another digest: the client refuses the world with a message, and
    // closes the connection.
    auto [client, server] = Neuron::InProcessTransport::CreatePair({});
    std::vector<std::string> titles;
    Species::ClientSession::Desc desc;
    desc.transport = std::move(client);
    desc.showTitle = [&titles](std::string_view _title) { titles.emplace_back(_title); };
    desc.productName = "Species";
    desc.tuning = &CommittedTuning();
    Species::ClientSession session(std::move(desc));
    Species::WorldTerrain sent{SeedOne().seed, SeedOne().digest};
    sent.digest[0] ^= std::byte{1};
    Assert::IsTrue(server->Send(Neuron::Channel::Reliable, Neuron::EncodeMessage(sent)).has_value());
    session.Update();
    Assert::IsTrue(session.Refused() && session.Terrain() == nullptr, L"refused");
    Assert::IsFalse(session.Connected(), L"the client closed");
    Assert::IsTrue(server->State() == Neuron::ConnectionState::Closed, L"the server's end sees the close");
    Assert::IsTrue(!titles.empty() && titles.back() == Species::RefusedTitle("Species"), L"the refusal's message");
  }
};

} // namespace GameClientTests
