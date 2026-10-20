#include "pch.h"

#include "MessageCodec.h"
#include "StableId.h"
#include "TestSupport.h"
#include "WorldTerrain.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameSharedTests
{

TEST_CLASS(WorldTerrainTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp011TheWorldTerrainRoundTrips)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-011")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp011TheWorldTerrainRoundTrips)
  {
    Species::WorldTerrain sent;
    sent.seed = 0xfedcba9876543210;
    for (std::size_t n = 0; n < sent.digest.size(); ++n)
    {
      sent.digest[n] = static_cast<std::byte>(n * 7 + 1);
    }
    const std::vector<std::byte> bytes = Neuron::EncodeMessage(sent);
    ExpectEqual(static_cast<std::int64_t>(Species::WorldTerrain::MAX_BYTES), static_cast<std::int64_t>(bytes.size()),
                L"its header, its seed and its digest");
    Neuron::MessageDispatcher dispatcher;
    std::optional<Species::WorldTerrain> received;
    dispatcher.Register<Species::WorldTerrain>([&received](const Species::WorldTerrain& _terrain) { received = _terrain; });
    Assert::IsTrue(dispatcher.Dispatch(bytes), L"it decodes whole");
    Assert::IsTrue(received && received->seed == sent.seed && received->digest == sent.digest, L"the same seed and digest");
    Assert::IsTrue(Species::WorldTerrain::TYPE >= Neuron::GAME_MESSAGE_TYPE_FIRST, L"a game message's id is above the engine's");
  }
};

TEST_CLASS(StableIdTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp093AStableIdIsItsOwnerAndCounter)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-093")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp093AStableIdIsItsOwnerAndCounter)
  {
    const Species::StableId id = Species::MakeStableId(Species::REGION_OWNER_ID_FIRST, 77);
    Assert::IsTrue(id == ((std::uint64_t{0x0100} << 48) | 77), L"the owner in the high 16 bits");
    Assert::IsTrue(Species::OwnerOf(id) == Species::REGION_OWNER_ID_FIRST, L"the owner");
    Assert::IsTrue(Species::CounterOf(id) == 77, L"the counter");
    Assert::IsTrue(Species::MakeStableId(Species::WORLD_OWNER_ID, 1) != 0, L"0 is never an id");
  }
};

} // namespace GameSharedTests
