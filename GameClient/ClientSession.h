#pragma once

#include "MessageCodec.h"
#include "Transport.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace Species
{

/// The client's session with one server (ADR-083 item 2's client-session scope, ADR-084): it owns the client's end of
/// the transport and decodes what the server sends, and never touches server state (MVP-090). Until the UI exists it
/// shows the server's tick in the window title (M0 task T7). Used on the main thread.
class ClientSession
{
public:
  struct Desc
  {
    std::unique_ptr<Neuron::Transport> transport;
    std::function<void(std::string_view)> showTitle; // sets the window's title; none in a test
    std::string productName;                         // the title's first part (PLT-057)
  };

  explicit ClientSession(Desc _desc);

  // Once per frame: every message that has arrived, then the title when the tick changed.
  void Update();
  void Close();

  [[nodiscard]] std::optional<std::uint64_t> ServerTick() const noexcept
  {
    return m_serverTick;
  }
  [[nodiscard]] bool Connected() const;
  [[nodiscard]] const Neuron::MessageDropCounts& Drops() const noexcept
  {
    return m_dispatcher.Drops();
  }

private:
  std::unique_ptr<Neuron::Transport> m_transport;
  std::function<void(std::string_view)> m_showTitle;
  std::string m_productName;
  Neuron::MessageDispatcher m_dispatcher;
  std::optional<std::uint64_t> m_serverTick;
  std::optional<std::uint64_t> m_shownTick;
};

// "<product> — tick <n>", or the product alone before the first report: the interim title of M0 (ADR-096).
[[nodiscard]] std::string SessionTitle(std::string_view _productName, std::optional<std::uint64_t> _serverTick);

} // namespace Species
