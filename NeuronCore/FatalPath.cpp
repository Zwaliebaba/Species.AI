#include "pch.h"

#include <intrin.h>

#include "FatalPath.h"
#include "PlatformLogSink.h"
#include "PlatformMessage.h"
#include "RunId.h"

#include <array>
#include <atomic>
#include <cstdlib>
#include <exception>
#include <format>
#include <string>

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

// PLT-127 step 2. The message names the run id, never the reason, and a failure while composing it is ignored: the
// process ends either way.
void ShowInternalError() noexcept
{
  try
  {
    const std::array arguments{MessageArgument{"runId", FormatRunId(PlatformMessageRunId())}};
    ShowPlatformMessage("ui.error.internal", arguments, MessageSeverity::Fatal);
  }
  catch (...)
  {
    return;
  }
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
  if (!g_failingFast.exchange(true, std::memory_order_acq_rel))
  {
    if (IsDebuggerPresent())
    {
      __debugbreak();
    }
    else if (PlatformMessageOwner() != nullptr)
    {
      ShowInternalError();
    }
  }
  RaiseFailFastException(nullptr, nullptr, FAIL_FAST_GENERATE_EXCEPTION_ADDRESS);
  __fastfail(FAST_FAIL_FATAL_APP_EXIT);
}

void FatalExit(std::string_view _stringId, std::span<const MessageArgument> _arguments)
{
  if (PlatformLogSink* sink = CurrentPlatformLogSink())
  {
    std::string record = std::format("fatal exit: {}", _stringId);
    for (const MessageArgument& argument : _arguments)
    {
      record += std::format(" {}={}", argument.name, argument.value);
    }
    sink->RecordFatal(record);
  }
  EndWithMessage(_stringId, _arguments);
}

} // namespace Neuron
