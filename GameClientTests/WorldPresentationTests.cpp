#include "pch.h"

#include "ContentKey.h"
#include "Files.h"
#include "LooseContent.h"
#include "RegionTerrain.h"
#include "TestSupport.h"
#include "Tuning.h"
#include "WorldPresentation.h"

#include <cmath>
#include <filesystem>
#include <numbers>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameClientTests
{

namespace
{

const Species::TuningPresentation& CommittedPresentation()
{
  static const Species::TuningPresentation PRESENTATION = []
  {
    const auto presentation = Species::ReadTuningPresentationSource(RepositoryContentRoot());
    Assert::IsTrue(presentation.has_value(), L"the committed presentation record reads whole");
    return presentation.value_or(Species::TuningPresentation{});
  }();
  return PRESENTATION;
}

// A loose-content folder holding every texture of the content root under its key, as the content build packs the
// textures as authored (ADR-108), removed again at the end of the test.
class TextureContent
{
public:
  TextureContent()
    : m_folder(std::filesystem::temp_directory_path() / L"GameClientTests-world-content")
  {
    std::error_code error;
    std::filesystem::remove_all(m_folder, error);
    const std::filesystem::path root = RepositoryContentRoot();
    std::vector<Neuron::CookedEntry> entries;
    for (const auto& file : std::filesystem::recursive_directory_iterator(root / L"Textures"))
    {
      if (!file.is_regular_file() || file.path().extension() != L".dds")
      {
        continue;
      }
      auto bytes = Neuron::ReadWholeFile(file.path());
      Assert::IsTrue(bytes.has_value());
      Neuron::CookedEntry entry;
      entry.entry.key = Neuron::KeyOfSource(std::filesystem::relative(file.path(), root).generic_string());
      entry.entry.type = Neuron::ContentType::Texture;
      entry.bytes = std::move(*bytes);
      entries.push_back(std::move(entry));
    }
    Assert::IsTrue(Neuron::WriteLooseContent(m_folder, std::move(entries)).has_value(), L"the loose-content folder is written");
  }
  ~TextureContent()
  {
    std::error_code error;
    std::filesystem::remove_all(m_folder, error);
  }
  TextureContent(const TextureContent&) = delete;
  TextureContent& operator=(const TextureContent&) = delete;

  [[nodiscard]] const std::filesystem::path& Folder() const noexcept
  {
    return m_folder;
  }

private:
  std::filesystem::path m_folder;
};

} // namespace

TEST_CLASS(WorldPresentationTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Rnd222TheFogEndsARegionsSidePlusTheMarginFromTheCamera)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-222")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Rnd222TheFogEndsARegionsSidePlusTheMarginFromTheCamera)
  {
    // O59: e = max(4000, 5120 + 1000) = 6120, the start a third of it, and half the fog at a view depth of 4080.
    const Neuron::WorldLook look = Species::WorldLookOf(CommittedTuning(), CommittedPresentation(), {30'720.0, 0.0, 30'720.0});
    Assert::AreEqual(6'120.0, look.fogEnd);
    Assert::AreEqual(2'040.0, look.fogStart, 1e-9);
    Assert::AreEqual(0.5, (look.fogEnd - 4'080.0) / (look.fogEnd - look.fogStart), 1e-9);
    Assert::AreEqual(15'000.0, look.planes.farPlane);
    Assert::AreEqual(std::numbers::pi, look.planes.fovMaxRadians, 1e-12, L"180° as radians (RND-212)");
    // The sky's clouds are anchored at the region's centre and its grid on the region's lattice (ADR-115).
    Assert::AreEqual(30'720.0 + 2'560.0, look.sky.regionCenter.x);
    Assert::AreEqual(30'720.0, look.sky.regionOriginZ);
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp081TheDevelopmentViewLooksAcrossTheValleyFromBeyondTheCoast)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-081")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp081TheDevelopmentViewLooksAcrossTheValleyFromBeyondTheCoast)
  {
    const Neuron::CameraPose pose = Species::DevelopmentView({30'720.0, 0.0, 30'720.0}, 5'120.0);
    Assert::AreEqual(33'280.0, pose.position.x);
    Assert::AreEqual(36'240.0, pose.position.z, L"400 wu beyond the region's +z coast");
    Assert::IsTrue(pose.position.y < 800.0, L"below the lowest cloud layer, so every sky layer faces it");
    const Neuron::Vec3 front = Neuron::ViewDirection(pose);
    Assert::AreEqual(-0.4472135955, front.y, 1e-9);
    Assert::AreEqual(-0.894427191, front.z, 1e-9);
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp081TheWorldIsBuiltFromTheRegionAndTheContentsTextures)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-081")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"TER-185")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp081TheWorldIsBuiltFromTheRegionAndTheContentsTextures)
  {
    auto region = Species::GenerateRegionTerrain(1, CommittedTuning());
    Assert::IsTrue(region.has_value());
    const TextureContent textures;
    const auto content = Neuron::LooseContent::Mount(textures.Folder());
    Assert::IsTrue(content.has_value());
    const auto first = Species::BuildWorldScene(*region, CommittedTuning(), CommittedPresentation(), *content);
    Assert::IsTrue(first != nullptr, L"every texture of the world frame is in the content under its key");
    Assert::IsTrue(first->terrain.triangles > 100'000, L"the valley's facets");
    Assert::AreEqual(std::size_t{25}, first->terrain.chunks.size(), L"the region's 25 chunks of 1,024 wu");
    Assert::IsTrue(!first->sea.flatVertices.empty() && first->sea.waveTriangles > 0, L"the sea ring and its waves");
    Assert::AreEqual(std::size_t{400}, first->sea.waveColors.size(), L"the shore-wave gradient's row");
    Assert::IsTrue(!first->seaTile.empty() && !first->terrainGrid.empty() && !first->skyCloud.empty() && !first->beamShort.empty());
    // TER-185: the same field gives the same colours.
    const auto second = Species::BuildWorldScene(*region, CommittedTuning(), CommittedPresentation(), *content);
    Assert::IsTrue(second != nullptr);
    bool same = true;
    for (std::size_t c = 0; c < first->terrain.chunks.size(); ++c)
    {
      const auto& a = first->terrain.chunks[c].vertices;
      const auto& b = second->terrain.chunks[c].vertices;
      same = same && a.size() == b.size();
      for (std::size_t v = 0; same && v < a.size(); ++v)
      {
        same = a[v].color == b[v].color;
      }
    }
    Assert::IsTrue(same, L"two builds of one field give the same facet colours");
  }
};

} // namespace GameClientTests
