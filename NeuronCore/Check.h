#pragma once

#include "Development.h"

#include <cstddef>
#include <format>
#include <source_location>
#include <string>
#include <string_view>
#include <utility>

namespace Neuron
{

// A failed release check, bounds check or abort: the fatal path of DBG-073, in Debug and Release alike.
[[noreturn]] void CheckFailed(const std::source_location& _location, std::string_view _condition, std::string_view _message);
[[noreturn]] void IndexCheckFailed(const std::source_location& _location, std::size_t _index, std::size_t _size);

inline std::string CheckMessage()
{
  return {};
}

template <class... Args> std::string CheckMessage(std::format_string<Args...> _format, Args&&... _args)
{
  return std::format(_format, std::forward<Args>(_args)...);
}

inline void CheckIndex(std::size_t _index, std::size_t _size, const std::source_location& _location)
{
  if (_index >= _size) [[unlikely]]
  {
    IndexCheckFailed(_location, _index, _size);
  }
}

#if ENGINE_DEVELOPMENT
// A failed development check (DBG-070): breaks into an attached debugger; otherwise logs an error record with the
// condition, file and line and continues. A test project installs a hook instead, which turns it into a test failure
// (DBG-072). Test hooks are compiled out of Release (table 3.1-B).
void DevelopmentCheckFailed(const std::source_location& _location, std::string_view _condition);

using DevelopmentCheckHook = void (*)(std::string_view);
void SetDevelopmentCheckHook(DevelopmentCheckHook _hook) noexcept;
#endif

} // namespace Neuron

// The check levels of DBG-070. A check states a program invariant; content errors never reach one (DBG-071).
#if ENGINE_DEVELOPMENT
#   define ENGINE_ASSERT(condition) \
     ((condition) ? static_cast<void>(0) : ::Neuron::DevelopmentCheckFailed(std::source_location::current(), #condition))
#else
// Not evaluated; sizeof keeps the names it uses referenced, so a Release build reports no unused variable.
#   define ENGINE_ASSERT(condition) static_cast<void>(sizeof(static_cast<bool>(condition)))
#endif

// A release check, with an optional message formatted from a compile-time format string; without one, the message
// is the condition text, file and line.
#define ENGINE_CHECK(condition, ...)  \
  ((condition) ? static_cast<void>(0) \
               : ::Neuron::CheckFailed(std::source_location::current(), #condition, ::Neuron::CheckMessage(__VA_ARGS__)))

// A container bounds check, fatal in every build. A negative index converts to a size no container reaches.
#define ENGINE_CHECK_INDEX(index, size) \
  ::Neuron::CheckIndex(static_cast<std::size_t>(index), static_cast<std::size_t>(size), std::source_location::current())

// An abort: a release check that always fails, with its message.
#define ENGINE_ABORT(...) ::Neuron::CheckFailed(std::source_location::current(), "", ::Neuron::CheckMessage(__VA_ARGS__))
