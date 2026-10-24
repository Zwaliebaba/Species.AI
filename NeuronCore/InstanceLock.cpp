#include "pch.h"

#include "InstanceLock.h"

#include <string>
#include <utility>

namespace Neuron
{

namespace
{

// The session's namespace: the user's own processes see the name, other sessions' do not.
std::wstring SessionName(std::wstring_view _name)
{
  return L"Local\\" + std::wstring(_name);
}

void CloseIfOpen(void* _handle) noexcept
{
  if (_handle != nullptr)
  {
    static_cast<void>(CloseHandle(_handle));
  }
}

} // namespace

std::optional<InstanceLock> InstanceLock::TryTake(std::wstring_view _name)
{
  const std::wstring name = SessionName(_name);
  HANDLE handle = CreateMutexW(nullptr, FALSE, name.c_str());
  if (handle == nullptr)
  {
    return std::nullopt;
  }
  if (GetLastError() == ERROR_ALREADY_EXISTS)
  {
    CloseIfOpen(handle);
    return std::nullopt;
  }
  return InstanceLock(handle);
}

bool InstanceLock::Held(std::wstring_view _name)
{
  const std::wstring name = SessionName(_name);
  HANDLE handle = OpenMutexW(SYNCHRONIZE, FALSE, name.c_str());
  CloseIfOpen(handle);
  return handle != nullptr;
}

InstanceLock::InstanceLock(void* _handle) noexcept
  : m_handle(_handle)
{
}

InstanceLock::InstanceLock(InstanceLock&& _other) noexcept
  : m_handle(std::exchange(_other.m_handle, nullptr))
{
}

InstanceLock& InstanceLock::operator=(InstanceLock&& _other) noexcept
{
  if (this != &_other)
  {
    CloseIfOpen(m_handle);
    m_handle = std::exchange(_other.m_handle, nullptr);
  }
  return *this;
}

InstanceLock::~InstanceLock()
{
  CloseIfOpen(m_handle);
}

std::optional<StopSignal> StopSignal::Create(std::wstring_view _name)
{
  const std::wstring name = SessionName(_name);
  HANDLE handle = CreateEventW(nullptr, TRUE, FALSE, name.c_str());
  if (handle == nullptr)
  {
    return std::nullopt;
  }
  // A signal raised for a process that has since ended is not this one's.
  static_cast<void>(ResetEvent(handle));
  return StopSignal(handle);
}

bool StopSignal::Raise(std::wstring_view _name)
{
  const std::wstring name = SessionName(_name);
  HANDLE handle = OpenEventW(EVENT_MODIFY_STATE, FALSE, name.c_str());
  if (handle == nullptr)
  {
    return false;
  }
  const bool raised = SetEvent(handle) != FALSE;
  CloseIfOpen(handle);
  return raised;
}

StopSignal::StopSignal(void* _handle) noexcept
  : m_handle(_handle)
{
}

StopSignal::StopSignal(StopSignal&& _other) noexcept
  : m_handle(std::exchange(_other.m_handle, nullptr))
{
}

StopSignal& StopSignal::operator=(StopSignal&& _other) noexcept
{
  if (this != &_other)
  {
    CloseIfOpen(m_handle);
    m_handle = std::exchange(_other.m_handle, nullptr);
  }
  return *this;
}

StopSignal::~StopSignal()
{
  CloseIfOpen(m_handle);
}

bool StopSignal::Wait(unsigned long _milliseconds) const noexcept
{
  return m_handle != nullptr && WaitForSingleObject(m_handle, _milliseconds) == WAIT_OBJECT_0;
}

} // namespace Neuron
