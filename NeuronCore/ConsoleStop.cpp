#include "pch.h"

#include "ConsoleStop.h"

#include <atomic>

namespace Neuron
{

namespace
{

// Process-wide, because the system calls the handler on a thread of its own and may do so after main has returned.
std::atomic<bool> g_stopRequested{false};
std::atomic<bool> g_stopFinished{false};

BOOL WINAPI HandleConsoleControl(DWORD _event) noexcept
{
  g_stopRequested.store(true, std::memory_order_release);
  g_stopRequested.notify_all();
  switch (_event)
  {
  case CTRL_CLOSE_EVENT:
  case CTRL_LOGOFF_EVENT:
  case CTRL_SHUTDOWN_EVENT:
    // Returning lets the system end the process at once; it ends it anyway when its own time limit passes.
    g_stopFinished.wait(false, std::memory_order_acquire);
    return TRUE;
  default:
    return TRUE; // Ctrl+C and Ctrl+Break: handled here, so the default handler does not end the process
  }
}

} // namespace

bool InstallConsoleStopHandler() noexcept
{
  return SetConsoleCtrlHandler(&HandleConsoleControl, TRUE) != FALSE;
}

void RequestConsoleStop() noexcept
{
  g_stopRequested.store(true, std::memory_order_release);
  g_stopRequested.notify_all();
}

void WaitForConsoleStop() noexcept
{
  g_stopRequested.wait(false, std::memory_order_acquire);
}

bool ConsoleStopRequested() noexcept
{
  return g_stopRequested.load(std::memory_order_acquire);
}

void ConsoleStopFinished() noexcept
{
  g_stopFinished.store(true, std::memory_order_release);
  g_stopFinished.notify_all();
}

} // namespace Neuron
