#include "pch.h"

#include "Check.h"
#include "TestSupport.h"
#include "Tuning.h"

#include <mutex>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameClientTests
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

std::vector<std::string> TakeDevelopmentChecks()
{
  const std::lock_guard lock(g_checksMutex);
  std::vector<std::string> checks;
  checks.swap(g_checks);
  return checks;
}

std::filesystem::path RepositoryContentRoot()
{
  return std::filesystem::path(__FILE__).parent_path().parent_path() / L"GameData";
}

const Species::TuningMvp& CommittedTuning()
{
  static const Species::TuningMvp COMMITTED_TUNING = []
  {
    const Neuron::RecordResult<Species::TuningMvp> tuning = Species::LoadTuningMvp(RepositoryContentRoot());
    Assert::IsTrue(tuning.has_value(), L"the committed tuning record reads whole");
    return tuning.value_or(Species::TuningMvp{});
  }();
  return COMMITTED_TUNING;
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
