#include "pch.h"

#include <intrin.h>

#include "FatalPath.h"
#include "PlatformLogSink.h"

#include <atomic>
#include <cstdlib>
#include <exception>

namespace Neuron
{

namespace
{

std::atomic<bool> g_failingFast{false};
std::atomic<bool> g_inFatalPath{false};

[[noreturn]] void OnTerminate() noexcept
{
  EnterFatalPath("std::terminate was called");
}

[[noreturn]] void OnPureCall() noexcept
{
  EnterFatalPath("a pure virtual function was called");
}

void OnInvalidParameter(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, std::uintptr_t) noexcept
{
  EnterFatalPath("the C runtime reported an invalid parameter");
}

} // namespace

void InstallFatalHandlers() noexcept
{
  std::set_terminate(&OnTerminate);
  _set_purecall_handler(&OnPureCall);
  _set_invalid_parameter_handler(&OnInvalidParameter);
}

void EnterFatalPath(std::string_view _reason) noexcept
{
  // A failure inside the sink's own fatal record must not record again.
  if (!g_inFatalPath.exchange(true, std::memory_order_acq_rel))
  {
    if (PlatformLogSink* sink = CurrentPlatformLogSink())
    {
      sink->RecordFatal(_reason);
    }
  }
  FailFast(_reason);
}

void FailFast([[maybe_unused]] std::string_view _reason) noexcept
{
  if (!g_failingFast.exchange(true, std::memory_order_acq_rel) && IsDebuggerPresent())
  {
    __debugbreak();
  }
  RaiseFailFastException(nullptr, nullptr, FAIL_FAST_GENERATE_EXCEPTION_ADDRESS);
  __fastfail(FAST_FAIL_FATAL_APP_EXIT);
}

} // namespace Neuron
