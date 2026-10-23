#include "pch.h"

#include "Files.h"
#include "TestSupport.h"
#include "WorldSaves.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameClientTests
{

namespace
{

// A file of _text in _folder.
void Write(const std::filesystem::path& _path, std::string_view _text)
{
  Assert::IsTrue(Neuron::ReplaceFileAtomically(_path, std::as_bytes(std::span(_text))).has_value(), L"written");
}

} // namespace

TEST_CLASS(WorldSavesTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp103NewWorldsAreNumberedAndDescribed)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-103")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-005")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-012")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp103NewWorldsAreNumberedAndDescribed)
  {
    const TemporaryFolder saves;
    std::optional<Species::SavedWorld> first = Species::CreateWorld(saves.Path(), std::nullopt, 2, 3);
    const std::optional<Species::SavedWorld> second = Species::CreateWorld(saves.Path(), std::uint64_t{18'446'744'073'709'551'615u}, 0, 0);
    if (!first.has_value() || !second.has_value())
    {
      Assert::Fail(L"both made");
      return;
    }
    Assert::IsTrue(first->folder.filename() == L"world-0001" && second->folder.filename() == L"world-0002", L"numbered in order");
    std::vector<Species::SavedWorld> worlds = Species::ListWorlds(saves.Path());
    Assert::IsTrue(worlds.size() == 2 && worlds[0].number == 2 && worlds[1].number == 1, L"the highest number first");
    Assert::IsTrue(worlds[0].seed == second->seed && worlds[0].site == 0 && worlds[0].aiSettlements == 0, L"a seed typed is kept");
    Assert::IsTrue(!worlds[1].seed.has_value() && worlds[1].site == 2 && worlds[1].aiSettlements == 3,
                   L"a drawn seed is not known yet; the AI settlements are kept (O103)");
    // MVP-005: the seed the terrain gave, recorded once the world runs.
    Assert::IsTrue(Species::RecordWorldSeed(*first, 4'294'967'296), L"recorded");
    worlds = Species::ListWorlds(saves.Path());
    Assert::IsTrue(worlds[1].seed == std::optional<std::uint64_t>(4'294'967'296) && worlds[1].site == 2 && worlds[1].aiSettlements == 3,
                   L"and read back");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp103ContinueOpensTheLatestWorldThatHasASnapshot)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-103")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-075")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp103ContinueOpensTheLatestWorldThatHasASnapshot)
  {
    const TemporaryFolder saves;
    Assert::IsFalse(Species::LatestWorld(saves.Path()).has_value(), L"no world, nothing to continue");
    const std::optional<Species::SavedWorld> first = Species::CreateWorld(saves.Path(), 7, 1, 3);
    const std::optional<Species::SavedWorld> second = Species::CreateWorld(saves.Path(), 8, 3, 3);
    if (!first.has_value() || !second.has_value())
    {
      Assert::Fail(L"made");
      return;
    }
    Assert::IsFalse(Species::LatestWorld(saves.Path()).has_value(), L"a world that saved no snapshot is not continued");
    Write(first->folder / L"world-00000000000000000600.snapshot", "a snapshot's bytes");
    const std::optional<Species::SavedWorld> latest = Species::LatestWorld(saves.Path());
    Assert::IsTrue(latest.has_value() && latest->number == 1 && latest->seed == std::optional<std::uint64_t>(7), L"the one that saved");
    Write(second->folder / L"world-00000000000000000300.snapshot", "a snapshot's bytes");
    Assert::IsTrue(Species::LatestWorld(saves.Path()).value_or(Species::SavedWorld{}).number == 2, L"the highest number that saved");

    // A folder whose description does not read is no world, and a name of another form is not one; a new world is
    // numbered above every folder of the form, so none is reused.
    std::filesystem::create_directories(saves.Path() / L"world-0007");
    Write(saves.Path() / L"world-0007" / L"world.txt", "world.site = 9\n");
    std::filesystem::create_directories(saves.Path() / L"world-0006");
    Write(saves.Path() / L"world-0006" / L"world.txt", "world.site = 1\nworld.ai_settlements = 4\n");
    std::filesystem::create_directories(saves.Path() / L"notes");
    Assert::AreEqual(std::size_t{2}, Species::ListWorlds(saves.Path()).size(), L"only the two worlds, not one of four AI settlements");
    const std::optional<Species::SavedWorld> third = Species::CreateWorld(saves.Path(), std::nullopt, 0, 3);
    Assert::IsTrue(third.has_value() && third->number == 8, L"numbered above world-0007");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp063AnEndedRunIsRecordedAndListed)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-063")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp063AnEndedRunIsRecordedAndListed)
  {
    // ADR-170: the run's end is written into the description, kept with the seed, and the ended worlds that hold a
    // snapshot are listed, the highest number first.
    const TemporaryFolder saves;
    std::optional<Species::SavedWorld> first = Species::CreateWorld(saves.Path(), 7, 1, 3);
    std::optional<Species::SavedWorld> second = Species::CreateWorld(saves.Path(), 8, 3, 3);
    std::optional<Species::SavedWorld> third = Species::CreateWorld(saves.Path(), 9, 0, 0);
    if (!first.has_value() || !second.has_value() || !third.has_value())
    {
      Assert::Fail(L"made");
      return;
    }
    Assert::IsTrue(Species::EndedWorlds(saves.Path()).empty(), L"no run has ended");
    Assert::IsTrue(Species::RecordRunEnded(*first) && Species::RecordRunEnded(*second) && first->ended, L"recorded");
    Assert::IsTrue(Species::RecordWorldSeed(*first, 70), L"and the seed after it");
    Write(first->folder / L"world-00000000000000000600.snapshot", "a snapshot's bytes");
    Write(third->folder / L"world-00000000000000000600.snapshot", "a snapshot's bytes");
    const std::vector<Species::SavedWorld> worlds = Species::ListWorlds(saves.Path());
    Assert::IsTrue(worlds.size() == 3 && !worlds[0].ended && worlds[1].ended && worlds[2].ended, L"read back");
    Assert::IsTrue(worlds[2].seed == std::optional<std::uint64_t>(70) && worlds[2].site == 1, L"with the seed and site");
    const std::vector<Species::SavedWorld> ended = Species::EndedWorlds(saves.Path());
    Assert::IsTrue(ended.size() == 1 && ended[0].number == 1, L"only the ended world that saved");
    Assert::IsTrue(Species::LatestWorld(saves.Path()).value_or(Species::SavedWorld{}).number == 3, L"Continue still opens the latest");
  }
};

} // namespace GameClientTests
