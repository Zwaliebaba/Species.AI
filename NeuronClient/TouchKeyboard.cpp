#include "pch.h"

#include <inputpaneinterop.h>
#include <roapi.h>
#include <windows.ui.viewmanagement.h>
#include <winstring.h>

#include "Log.h"
#include "TouchKeyboard.h"

#include <cstdint>
#include <iterator>
#include <utility>

#pragma comment(lib, "runtimeobject.lib")

namespace Neuron
{

namespace
{

using ABI::Windows::UI::ViewManagement::IInputPane2;

} // namespace

TouchKeyboard::~TouchKeyboard()
{
  m_pane.Reset();
  if (m_runtime)
  {
    RoUninitialize();
  }
}

void TouchKeyboard::Follow(bool _editing, bool _touchMode, bool _tapped)
{
  const TouchKeyboardRequest request = TouchKeyboardRequestOf(m_editing, _editing, _touchMode, _tapped);
  m_editing = _editing;
  if (request == TouchKeyboardRequest::Show)
  {
    Ask(true);
  }
  else if (request == TouchKeyboardRequest::Hide && m_pane)
  {
    Ask(false);
  }
}

bool TouchKeyboard::Acquire()
{
  if (m_pane)
  {
    return true;
  }
  if (m_unavailable)
  {
    return false;
  }
  // The main thread pumps the window's messages, so it is a single-threaded apartment; one that some earlier code put in
  // the multithreaded apartment stays there.
  HRESULT result = RoInitialize(RO_INIT_SINGLETHREADED);
  m_runtime = SUCCEEDED(result);
  if (result == RPC_E_CHANGED_MODE)
  {
    result = S_OK;
  }
  HSTRING_HEADER header{};
  HSTRING className = nullptr;
  if (SUCCEEDED(result))
  {
    result = WindowsCreateStringReference(RuntimeClass_Windows_UI_ViewManagement_InputPane,
                                          static_cast<UINT32>(std::size(RuntimeClass_Windows_UI_ViewManagement_InputPane) - 1), &header,
                                          &className);
  }
  Microsoft::WRL::ComPtr<IInputPaneInterop> interop;
  if (SUCCEEDED(result))
  {
    result = RoGetActivationFactory(className, IID_PPV_ARGS(&interop));
  }
  Microsoft::WRL::ComPtr<IInputPane2> pane;
  if (SUCCEEDED(result))
  {
    result = interop->GetForWindow(m_window, IID_PPV_ARGS(&pane));
  }
  if (FAILED(result))
  {
    m_unavailable = true;
    Log(LogCategory::Input, LogLevel::Warning, "the touch keyboard is unavailable: {:#010x} (ADR-181)", static_cast<std::uint32_t>(result));
    return false;
  }
  m_pane = pane;
  return true;
}

void TouchKeyboard::Ask(bool _show)
{
  if (!Acquire())
  {
    return;
  }
  Microsoft::WRL::ComPtr<IInputPane2> pane;
  HRESULT result = m_pane.As(&pane);
  boolean done = 0;
  if (SUCCEEDED(result))
  {
    result = _show ? pane->TryShow(&done) : pane->TryHide(&done);
  }
  // Windows may decline to show it, as it does with a hardware keyboard attached; the first refusal or failure is logged
  // for T19's run. A hide declined is the keyboard already closed by its own key.
  if ((FAILED(result) || (_show && done == 0)) && !std::exchange(m_refusalLogged, true))
  {
    Log(LogCategory::Input, LogLevel::Info, "the touch keyboard did not {}: {:#010x} (ADR-181)", _show ? "show" : "hide",
        static_cast<std::uint32_t>(result));
  }
}

} // namespace Neuron
