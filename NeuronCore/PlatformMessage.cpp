#include "pch.h"

#include <CommCtrl.h>

#include "NativeWindowHandle.h"
#include "PlatformLogSink.h"
#include "PlatformMessage.h"
#include "TextEncoding.h"

#include <atomic>
#include <format>

namespace Neuron
{

namespace
{

std::atomic<const PlatformMessageContext*> g_context{nullptr};
std::atomic<NativeWindowHandle> g_owner{nullptr};
std::atomic<bool> g_unattended{false};

using TaskDialogFunction = HRESULT(WINAPI*)(HWND, HINSTANCE, PCWSTR, PCWSTR, PCWSTR, TASKDIALOG_COMMON_BUTTON_FLAGS, PCWSTR, int*);

std::string Substitute(std::string_view _text, std::span<const MessageArgument> _arguments)
{
  std::string result;
  result.reserve(_text.size());
  std::size_t position = 0;
  while (position < _text.size())
  {
    const std::size_t open = _text.find('{', position);
    const std::size_t close = open == std::string_view::npos ? open : _text.find('}', open + 1);
    if (close == std::string_view::npos)
    {
      result.append(_text.substr(position));
      break;
    }
    result.append(_text.substr(position, open - position));
    const std::string_view name = _text.substr(open + 1, close - open - 1);
    const MessageArgument* argument = nullptr;
    for (const MessageArgument& candidate : _arguments)
    {
      if (candidate.name == name)
      {
        argument = &candidate;
        break;
      }
    }
    if (argument != nullptr)
    {
      result.append(argument->value);
    }
    else
    {
      result.append(_text.substr(open, close - open + 1));
    }
    position = close + 1;
  }
  return result;
}

// A service or a test host runs without a visible window station, and a dialog there would wait for nobody.
bool HasInteractiveDesktop() noexcept
{
  HWINSTA station = GetProcessWindowStation();
  USEROBJECTFLAGS flags{};
  DWORD needed = 0;
  return station != nullptr && GetUserObjectInformationW(station, UOI_FLAGS, &flags, sizeof(flags), &needed) != FALSE &&
         (flags.dwFlags & WSF_VISIBLE) != 0;
}

// The task dialog lives in version 6 of the common controls, which the application manifest selects; it is looked up
// at run time so that a process without that manifest, a test host for one, still loads and falls back to a message box.
void ShowDialog(HWND _owner, const std::wstring& _title, const std::wstring& _text, MessageSeverity _severity) noexcept
{
  HMODULE controls = LoadLibraryW(L"comctl32.dll");
  // GetProcAddress returns a generic function pointer; void (*)() is the type a function pointer is cast through.
  const FARPROC found = controls != nullptr ? GetProcAddress(controls, "TaskDialog") : nullptr;
  const auto taskDialog = found != nullptr ? reinterpret_cast<TaskDialogFunction>(reinterpret_cast<void (*)()>(found)) : nullptr;
  if (taskDialog != nullptr)
  {
    PCWSTR icon = _severity == MessageSeverity::Notice ? TD_INFORMATION_ICON : TD_ERROR_ICON;
    int pressed = 0;
    if (SUCCEEDED(taskDialog(_owner, nullptr, _title.c_str(), _text.c_str(), nullptr, TDCBF_OK_BUTTON, icon, &pressed)))
    {
      return;
    }
  }
  const UINT icon = _severity == MessageSeverity::Notice ? MB_ICONINFORMATION : MB_ICONERROR;
  MessageBoxW(_owner, _text.c_str(), _title.c_str(), MB_OK | icon);
}

} // namespace

void SetPlatformMessagesUnattended(bool _unattended) noexcept
{
  g_unattended.store(_unattended, std::memory_order_release);
}

void SetPlatformMessageContext(const PlatformMessageContext* _context) noexcept
{
  g_context.store(_context, std::memory_order_release);
}

std::uint64_t PlatformMessageRunId() noexcept
{
  const PlatformMessageContext* context = g_context.load(std::memory_order_acquire);
  return context != nullptr ? context->runId : 0;
}

void SetPlatformMessageOwner(NativeWindowHandle _window) noexcept
{
  g_owner.store(_window, std::memory_order_release);
}

NativeWindowHandle PlatformMessageOwner() noexcept
{
  return g_owner.load(std::memory_order_acquire);
}

std::string ComposePlatformMessage(std::span<const CompiledText> _texts, std::string_view _stringId,
                                   std::span<const MessageArgument> _arguments)
{
  for (const CompiledText& entry : _texts)
  {
    if (entry.stringId == _stringId)
    {
      return Substitute(entry.text, _arguments);
    }
  }
  std::string text(_stringId);
  for (const MessageArgument& argument : _arguments)
  {
    text += std::format(" {}={}", argument.name, argument.value);
  }
  return text;
}

void ShowPlatformMessage(std::string_view _stringId, std::span<const MessageArgument> _arguments, MessageSeverity _severity)
{
  const PlatformMessageContext* context = g_context.load(std::memory_order_acquire);
  const std::span<const CompiledText> texts = context != nullptr ? context->texts : std::span<const CompiledText>{};
  const std::string text = ComposePlatformMessage(texts, _stringId, _arguments);
  PlatformLog(_severity == MessageSeverity::Notice ? PlatformLogLevel::Warning : PlatformLogLevel::Error, "pre-UI message {}: {}",
              _stringId, text);
  if (g_unattended.load(std::memory_order_acquire) || !HasInteractiveDesktop())
  {
    return;
  }
  const std::wstring title = ToUtf16(context != nullptr ? context->title : std::string_view{});
  ShowDialog(PlatformMessageOwner(), title, ToUtf16(text), _severity);
}

void EndWithMessage(std::string_view _stringId, std::span<const MessageArgument> _arguments)
{
  ShowPlatformMessage(_stringId, _arguments, MessageSeverity::Fatal);
  ExitProcess(static_cast<UINT>(EXIT_CODE_FATAL));
}

} // namespace Neuron
