#include "pch.h"

#include <intrin.h>

#include "Check.h"
#include "FatalPath.h"
#include "Log.h"
#include "TextEncoding.h"

#include <atomic>

namespace Neuron
{

namespace
{

#if ENGINE_DEVELOPMENT
std::atomic<DevelopmentCheckHook> g_developmentCheckHook{nullptr};
#endif

std::string_view FileName(const std::source_location& _location) noexcept
{
  const std::string_view path = _location.file_name();
  const std::size_t slash = path.find_last_of("\\/");
  return slash == std::string_view::npos ? path : path.substr(slash + 1);
}

} // namespace

void CheckFailed(const std::source_location& _location, std::string_view _condition, std::string_view _message)
{
  std::string text;
  if (_condition.empty())
  {
    text = std::format("abort: {} ({}:{})", _message, FileName(_location), _location.line());
  }
  else if (_message.empty())
  {
    text = std::format("check failed: {} ({}:{})", _condition, FileName(_location), _location.line());
  }
  else
  {
    text = std::format("check failed: {}: {} ({}:{})", _condition, _message, FileName(_location), _location.line());
  }
  EnterFatalPath(text);
}

void IndexCheckFailed(const std::source_location& _location, std::size_t _index, std::size_t _size)
{
  CheckFailed(_location, "index < size", std::format("index {} is not below the size {}", _index, _size));
}

#if ENGINE_DEVELOPMENT
void DevelopmentCheckFailed(const std::source_location& _location, std::string_view _condition)
{
  const std::string text =
    std::format("development check failed: {} ({}:{}, {})", _condition, FileName(_location), _location.line(), _location.function_name());
  if (const DevelopmentCheckHook hook = g_developmentCheckHook.load(std::memory_order_acquire))
  {
    hook(text);
    return;
  }
  Log(LogCategory::Diagnostics, LogLevel::Error, "{}", text);
  if (IsDebuggerPresent())
  {
    std::wstring line = ToUtf16(text);
    line.push_back(L'\n');
    OutputDebugStringW(line.c_str());
    __debugbreak();
  }
}

void SetDevelopmentCheckHook(DevelopmentCheckHook _hook) noexcept
{
  g_developmentCheckHook.store(_hook, std::memory_order_release);
}
#endif

} // namespace Neuron
