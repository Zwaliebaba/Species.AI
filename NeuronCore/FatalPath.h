#pragma once

#include "PlatformMessage.h"

#include <span>
#include <string_view>

namespace Neuron
{

// Start-up step 1 (PLT-118): the terminate, invalid-parameter and pure-call handlers, each of which enters the fatal
// path with its reason. No unhandled-exception filter is installed: an unhandled exception on any thread reaches
// Windows Error Reporting untouched (PLT-117).
void InstallFatalHandlers() noexcept;

// Steps 1 and 2 of the fatal path through the log sink's RecordFatal when one is set, then FailFast (DBG-073).
[[noreturn]] void EnterFatalPath(std::string_view _reason) noexcept;

// The platform's last step of the fatal path (PLT-127): break into an attached debugger, or else, when a window owns
// pre-UI messages, show `ui.error.internal` with the run id and never the reason; then end the process with a
// fail-fast exception that Windows Error Reporting captures. A second entry fails fast at once.
[[noreturn]] void FailFast(std::string_view _reason) noexcept;

// A fatal exit (DBG-074): a failure of the environment, not a defect (PLT-044). The string id and the arguments become
// a fatal log record through the log sink, which flushes it, and EndWithMessage then shows the message and ends the
// process with plt.lifecycle.exit_code.fatal and no report. The black box comes with crash reporting.
[[noreturn]] void FatalExit(std::string_view _stringId, std::span<const MessageArgument> _arguments);

} // namespace Neuron
