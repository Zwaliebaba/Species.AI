#pragma once

#include <string_view>

namespace Neuron
{

// Start-up step 1 (PLT-118): the terminate, invalid-parameter and pure-call handlers, each of which enters the fatal
// path with its reason. No unhandled-exception filter is installed: an unhandled exception on any thread reaches
// Windows Error Reporting untouched (PLT-117).
void InstallFatalHandlers() noexcept;

// Steps 1 and 2 of the fatal path through the log sink's RecordFatal when one is set, then FailFast (DBG-073).
[[noreturn]] void EnterFatalPath(std::string_view _reason) noexcept;

// The platform's last step of the fatal path (PLT-127): break into an attached debugger, then end the process with a
// fail-fast exception that Windows Error Reporting captures. A second entry fails fast at once. The pre-UI message
// `ui.error.internal` for a process with a window arrives with the window (M0 task T4 onwards).
[[noreturn]] void FailFast(std::string_view _reason) noexcept;

} // namespace Neuron
