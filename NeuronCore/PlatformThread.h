#pragma once

#include <stop_token>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

namespace Neuron
{

/// A named thread that stops and joins when it is destroyed (PLT-083). Every thread of the client and of the server
/// is one: the body receives a stop token and returns when stop is requested. No affinity and no raised priority
/// (PLT-084).
class PlatformThread
{
public:
  PlatformThread() noexcept = default;
  ~PlatformThread() = default;

  PlatformThread(PlatformThread&&) noexcept = default;
  PlatformThread& operator=(PlatformThread&&) noexcept = default;
  PlatformThread(const PlatformThread&) = delete;
  PlatformThread& operator=(const PlatformThread&) = delete;

  // Starts _body(std::stop_token) on a new thread named _name, after the default floating-point environment.
  template <class Body> [[nodiscard]] static PlatformThread Start(std::string_view _name, Body&& _body);

  void RequestStop() noexcept
  {
    m_thread.request_stop();
  }
  void Join();
  [[nodiscard]] bool Joinable() const noexcept
  {
    return m_thread.joinable();
  }

private:
  static void Enter(const std::string& _name);
  static void Leave(const std::string& _name);

  std::jthread m_thread;
};

// The name of the calling thread: the one PlatformThread gave it, the one NameCurrentThread set, or else its id.
[[nodiscard]] const std::string& CurrentThreadName();

// Names a thread that PlatformThread did not start: the main thread, at start-up.
void NameCurrentThread(std::string_view _name);

template <class Body> PlatformThread PlatformThread::Start(std::string_view _name, Body&& _body)
{
  PlatformThread thread;
  thread.m_thread = std::jthread(
    [name = std::string{_name}, body = std::forward<Body>(_body)](const std::stop_token& _stop) mutable
    {
      Enter(name);
      body(_stop);
      Leave(name);
    });
  return thread;
}

} // namespace Neuron
