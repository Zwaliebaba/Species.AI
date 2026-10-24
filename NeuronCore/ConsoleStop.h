#pragma once

namespace Neuron
{

// The headless server's stop request (ADR-100): Ctrl+C or Ctrl+Break in its console, the console window closing, the
// user logging off or the system shutting down. Installed once by a console program's composition root, before it
// waits.
[[nodiscard]] bool InstallConsoleStopHandler() noexcept;

// A stop that the program requests itself, as a stop signal raised by another process does (ADR-187).
void RequestConsoleStop() noexcept;

// Blocks the calling thread until a stop is requested.
void WaitForConsoleStop() noexcept;
[[nodiscard]] bool ConsoleStopRequested() noexcept;

// The program has stopped cleanly. The system ends a process as soon as its handler returns from a close, log-off or
// shutdown event, so the handler holds that event until this is called and the world has frozen.
void ConsoleStopFinished() noexcept;

} // namespace Neuron
