#include "pch.h"

#include "Check.h"
#include "TestSupport.h"

#include <atomic>
#include <format>
#include <mutex>
#include <system_error>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronServerTests
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

std::atomic<std::uint64_t> g_folderCount{0};

} // namespace

TemporaryFolder::TemporaryFolder()
{
  std::error_code error;
  m_path = std::filesystem::temp_directory_path(error) / L"NeuronServerTests" /
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
    OutputDebugStringW(L"NeuronServerTests: a temporary folder was not removed\n");
  }
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

} // namespace NeuronServerTests
