#include "pch.h"

#include "FloatEnvironment.h"
#include "PlatformLogSink.h"
#include "PlatformThread.h"
#include "TextEncoding.h"

#include <format>

namespace Neuron
{

namespace
{

thread_local std::string g_threadName;

} // namespace

void PlatformThread::Join()
{
  if (m_thread.joinable())
  {
    m_thread.join();
  }
}

void PlatformThread::Enter(const std::string& _name)
{
  // The floating-point environment is the thread's first action (PLT-083, PLT-087).
  ApplyDefaultFloatEnvironment();
  NameCurrentThread(_name);
  PlatformLog(PlatformLogLevel::Debug, "thread {} started", _name);
}

void PlatformThread::Leave(const std::string& _name)
{
  PlatformLog(PlatformLogLevel::Debug, "thread {} ended", _name);
}

const std::string& CurrentThreadName()
{
  if (g_threadName.empty())
  {
    g_threadName = std::format("thread {}", GetCurrentThreadId());
  }
  return g_threadName;
}

void NameCurrentThread(std::string_view _name)
{
  g_threadName = _name;
  SetThreadDescription(GetCurrentThread(), ToUtf16(_name).c_str());
}

} // namespace Neuron
