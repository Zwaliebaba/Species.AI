#include "pch.h"

#include "Check.h"
#include "TestSupport.h"
#include "Tuning.h"

#include <atomic>
#include <mutex>
#include <system_error>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameClientTests
{

namespace
{

std::mutex g_checksMutex;
std::atomic<std::uint32_t> g_folderCount{0}; // the temporary folders made by this process
std::vector<std::string> g_checks;

#if ENGINE_DEVELOPMENT
void OnDevelopmentCheck(std::string_view _message)
{
  const std::lock_guard lock(g_checksMutex);
  g_checks.emplace_back(_message);
}
#endif

} // namespace

std::vector<std::string> TakeDevelopmentChecks()
{
  const std::lock_guard lock(g_checksMutex);
  std::vector<std::string> checks;
  checks.swap(g_checks);
  return checks;
}

TemporaryFolder::TemporaryFolder()
{
  std::error_code error;
  m_path = std::filesystem::temp_directory_path(error) / L"GameClientTests" /
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
    OutputDebugStringW(L"GameClientTests: a temporary folder was not removed\n");
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

const Species::TuningPresentation& CommittedPresentation()
{
  static const Species::TuningPresentation COMMITTED_PRESENTATION = []
  {
    const auto presentation = Species::ReadTuningPresentationSource(RepositoryContentRoot());
    Assert::IsTrue(presentation.has_value(), L"the committed presentation record reads whole");
    return presentation.value_or(Species::TuningPresentation{});
  }();
  return COMMITTED_PRESENTATION;
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

} // namespace GameClientTests
