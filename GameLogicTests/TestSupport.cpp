#include "pch.h"

#include "Check.h"
#include "Files.h"
#include "PlatformThread.h"
#include "TerrainField.h"
#include "TestSupport.h"
#include "Tuning.h"

#include <atomic>
#include <exception>
#include <fstream>
#include <mutex>
#include <stop_token>
#include <system_error>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

std::mutex g_checksMutex;
std::vector<std::string> g_checks;

#if ENGINE_DEVELOPMENT
void OnDevelopmentCheck(std::string_view _message)
{
  const std::lock_guard lock(g_checksMutex);
  g_checks.emplace_back(_message);
}
#endif

} // namespace

namespace
{

std::atomic<std::uint64_t> g_folderCount{0};

} // namespace

TemporaryFolder::TemporaryFolder()
{
  std::error_code error;
  m_path = std::filesystem::temp_directory_path(error) / L"GameLogicTests" /
           std::format(L"{}-{}", GetCurrentProcessId(), g_folderCount.fetch_add(1, std::memory_order_relaxed));
  (void)std::filesystem::remove_all(m_path, error);
  (void)std::filesystem::create_directories(m_path, error);
}

TemporaryFolder::~TemporaryFolder()
{
  try
  {
    std::error_code error;
    (void)std::filesystem::remove_all(m_path, error);
  }
  catch (...)
  {
    // A folder left behind is harmless: the next run of this process id removes it first.
    OutputDebugStringW(L"GameLogicTests: a temporary folder was not removed\n");
  }
}

std::filesystem::path RepositoryContentRoot()
{
  return std::filesystem::path(__FILE__).parent_path().parent_path() / L"GameData";
}

const Species::TuningMvp& CommittedTuning()
{
  static const Species::TuningMvp COMMITTED_TUNING = []
  {
    const Neuron::RecordResult<Species::TuningMvp> tuning = Species::ReadTuningMvpSource(RepositoryContentRoot());
    Assert::IsTrue(tuning.has_value(), L"the committed tuning record reads whole");
    return tuning.value_or(Species::TuningMvp{});
  }();
  return COMMITTED_TUNING;
}

std::shared_ptr<const Species::RegionTerrain> FlatTerrain(double _height)
{
  const Species::TuningMvp& tuning = CommittedTuning();
  Neuron::TerrainDefinition definition;
  definition.cellSize = tuning.wld.terrain.cellSize;
  definition.originX = static_cast<double>(tuning.mvp.world.regionCell) * tuning.wld.region.side;
  definition.originZ = definition.originX;
  definition.worldSizeX = static_cast<std::int32_t>(tuning.wld.region.side);
  definition.worldSizeZ = definition.worldSizeX;
  definition.outsideHeight = _height;
  auto terrain = std::make_shared<Species::RegionTerrain>();
  terrain->field = Neuron::TerrainField::Generate(definition, {}, {});
  return terrain;
}

void RunConcurrently(const std::vector<std::function<void()>>& _jobs)
{
  std::vector<std::exception_ptr> failures(_jobs.size());
  std::vector<Neuron::PlatformThread> threads;
  threads.reserve(_jobs.size());
  for (std::size_t index = 0; index < _jobs.size(); ++index)
  {
    threads.push_back(Neuron::PlatformThread::Start("test.concurrent",
                                                    [&_jobs, &failures, index](const std::stop_token&)
                                                    {
                                                      try
                                                      {
                                                        _jobs[index]();
                                                      }
                                                      catch (...)
                                                      {
                                                        failures[index] = std::current_exception();
                                                      }
                                                    }));
  }
  for (Neuron::PlatformThread& thread : threads)
  {
    thread.Join();
  }
  for (const std::exception_ptr& failure : failures)
  {
    if (failure)
    {
      std::rethrow_exception(failure);
    }
  }
}

std::shared_ptr<const Species::RegionTerrain> ValleyTerrain()
{
  static const std::shared_ptr<const Species::RegionTerrain> VALLEY =
    std::make_shared<const Species::RegionTerrain>(*Species::GenerateRegionTerrain(1, CommittedTuning()));
  return VALLEY;
}

std::string ReadText(const std::filesystem::path& _path)
{
  const auto bytes = Neuron::ReadWholeFile(_path);
  if (!bytes)
  {
    return {};
  }
  return std::string(reinterpret_cast<const char*>(bytes->data()), bytes->size());
}

void WriteText(const std::filesystem::path& _path, std::string_view _text)
{
  std::error_code error;
  (void)std::filesystem::create_directories(_path.parent_path(), error);
  std::ofstream file(_path, std::ios::binary | std::ios::trunc);
  file.write(_text.data(), static_cast<std::streamsize>(_text.size()));
}

std::vector<std::string> TakeDevelopmentChecks()
{
  const std::lock_guard lock(g_checksMutex);
  std::vector<std::string> checks;
  checks.swap(g_checks);
  return checks;
}

// Development checks become test failures (DBG-072): the hook records them, and every test class checks the record.
TEST_MODULE_INITIALIZE(ModuleInitialize)
{
#if ENGINE_DEVELOPMENT
  Neuron::SetDevelopmentCheckHook(&OnDevelopmentCheck);
#endif
}

TEST_MODULE_CLEANUP(ModuleCleanup)
{
#if ENGINE_DEVELOPMENT
  Neuron::SetDevelopmentCheckHook(nullptr);
#endif
}

} // namespace GameLogicTests
