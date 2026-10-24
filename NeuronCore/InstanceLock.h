#pragma once

#include <optional>
#include <string_view>

namespace Neuron
{

/// A lock that one process of the user's session holds at a time, by its name (ADR-190): a named object of the session's
/// namespace, which exists while the process that made it holds it, and goes when it ends, however it ends. Moved,
/// never copied.
class InstanceLock
{
public:
  // The lock of _name, held until it ends; none when another process holds it or it cannot be made.
  [[nodiscard]] static std::optional<InstanceLock> TryTake(std::wstring_view _name);
  // Whether any process holds the lock of _name now.
  [[nodiscard]] static bool Held(std::wstring_view _name);

  InstanceLock(InstanceLock&& _other) noexcept;
  InstanceLock& operator=(InstanceLock&& _other) noexcept;
  InstanceLock(const InstanceLock&) = delete;
  InstanceLock& operator=(const InstanceLock&) = delete;
  ~InstanceLock();

private:
  explicit InstanceLock(void* _handle) noexcept;

  void* m_handle = nullptr;
};

/// A request from one process to another that it stop (ADR-190): a named event of the session's namespace, which the
/// process that may be stopped makes and waits on, and any other raises. Moved, never copied.
class StopSignal
{
public:
  // The signal of _name, not raised, for this process to wait on; none when it cannot be made.
  [[nodiscard]] static std::optional<StopSignal> Create(std::wstring_view _name);
  // Raises the signal of _name for the process that made it; false when no process has.
  static bool Raise(std::wstring_view _name);

  StopSignal(StopSignal&& _other) noexcept;
  StopSignal& operator=(StopSignal&& _other) noexcept;
  StopSignal(const StopSignal&) = delete;
  StopSignal& operator=(const StopSignal&) = delete;
  ~StopSignal();

  // Waits until the signal is raised, or _milliseconds have passed; whether it was raised.
  [[nodiscard]] bool Wait(unsigned long _milliseconds) const noexcept;

private:
  explicit StopSignal(void* _handle) noexcept;

  void* m_handle = nullptr;
};

} // namespace Neuron
