#include "pch.h"

// msquic.h turns off two of its own warnings for the rest of the unit (C4201, C4214); the push and pop keep that to it.
#pragma warning(push)
#include <msquic.h>
#pragma warning(pop)

#include "MessageCodec.h"
#include "QuicTransport.h"
#include "ServerCertificate.h"
#include "StreamFraming.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <exception>
#include <filesystem>
#include <format>
#include <mutex>
#include <utility>
#include <vector>

namespace Neuron
{

/// MsQuic, loaded while any end or listener uses it (ADR-089): the pinned msquic.dll beside the module that holds this
/// code, its function table and the one registration every connection belongs to. Ending it waits for every
/// connection of the registration to be closed, which each end and listener does before it lets go of it.
class QuicLibrary
{
public:
  QuicLibrary() noexcept = default;
  QuicLibrary(const QuicLibrary&) = delete;
  QuicLibrary& operator=(const QuicLibrary&) = delete;
  ~QuicLibrary();

  // The process's library, loaded and registered when no end holds it; or why it cannot be.
  [[nodiscard]] static std::expected<std::shared_ptr<QuicLibrary>, std::string> Acquire();

  [[nodiscard]] const QUIC_API_TABLE& Api() const noexcept
  {
    return *m_api;
  }
  [[nodiscard]] HQUIC Registration() const noexcept
  {
    return m_registration;
  }

private:
  HMODULE m_module = nullptr;
  MsQuicCloseFn m_close = nullptr;
  const QUIC_API_TABLE* m_api = nullptr;
  HQUIC m_registration = nullptr;
};

/// One connection's state, shared by its end and MsQuic's callbacks, behind one lock. The callbacks hold a raw pointer
/// to it, which stays valid until the end has closed the connection's handles, after which MsQuic calls back no more.
/// No MsQuic function is called while the lock is held, because MsQuic may run a callback, which takes it, inside the
/// call, or wait for one.
struct QuicLink
{
  // A buffer MsQuic holds until it reports the send complete or the datagram's fate final.
  struct SendBlock
  {
    QUIC_BUFFER buffer{};
    std::vector<std::byte> bytes;
    std::uint64_t datagram = 0; // its number on the state channel; 0 on the reliable channel
    bool decided = false;       // its fate reported
  };

  std::shared_ptr<QuicLibrary> library;
  bool server = false;
  Sha256Digest pinned{};         // the client's: the server certificate's fingerprint
  HQUIC configuration = nullptr; // the client's own; a server's connections use their listener's
  HQUIC connection = nullptr;    // set before MsQuic calls back, and closed by the end

  mutable std::mutex mutex;
  std::condition_variable changed;
  HQUIC stream = nullptr;            // the reliable channel, closed by the end
  bool connected = false;            // the handshake completed
  bool certificateChecked = false;   // the client checked the server's certificate against its pin
  bool certificateRefused = false;   // and refused it
  bool ending = false;               // the end is closing its handles: no stream is taken any more
  bool closed = false;               // this end called Close, or the peer or the transport ended the connection
  bool sendShutdownComplete = false; // the stream's sending side ended: every reliable byte acknowledged, or abandoned
  bool shutdownComplete = false;     // MsQuic finished with the connection
  bool receivePaused = false;        // the reliable bytes not yet taken passed QUIC_RELIABLE_BACKLOG_BYTES
  bool datagramsEnabled = false;     // the peer takes datagrams
  std::uint16_t datagramMaxSendBytes = 0;
  CloseReason reason = CloseReason::None;
  std::vector<std::byte> waiting; // framed reliable messages that wait for the stream
  std::size_t inFlightBytes = 0;  // framed reliable bytes MsQuic holds
  StreamFrameReader reader{MESSAGE_MAX_BYTES};
  std::deque<ReceivedMessage> inbound;
  std::size_t inboundReliableBytes = 0;
  std::vector<DatagramOutcome> outcomes;
  std::uint64_t nextDatagram = 1;
  std::uint64_t datagramsRefused = 0;
  std::uint64_t datagramsSent = 0; // ADR-192: put in packets, as MsQuic reports them sent
  std::uint64_t datagramBytesSent = 0;
  TransportStats stats;

  // Under the lock: the connection is ending, for _reason unless an earlier one was recorded.
  void MarkClosed(CloseReason _reason)
  {
    closed = true;
    if (reason == CloseReason::None)
    {
      reason = _reason;
    }
  }
};

/// A listener's state: its configuration, which holds the server's certificate, and the connections that arrived, in
/// their order, until Accept takes them. The listener's callback holds a raw pointer to it, valid until the listener's
/// handle is closed.
struct QuicEndpoint
{
  std::shared_ptr<QuicLibrary> library;
  HQUIC configuration = nullptr;
  HQUIC listener = nullptr;
  std::uint16_t port = 0;
  std::mutex mutex;
  std::deque<std::unique_ptr<QuicTransport>> arrived;

  QuicEndpoint() = default;
  QuicEndpoint(const QuicEndpoint&) = delete;
  QuicEndpoint& operator=(const QuicEndpoint&) = delete;
  ~QuicEndpoint();

  // On MsQuic's thread: a client's connection has arrived.
  QUIC_STATUS Arrived(HQUIC _connection);
  // The first arrived connection whose handshake has completed; those that ended without one are closed.
  std::unique_ptr<Transport> Take();
};

namespace
{

std::string Failure(std::string_view _step, QUIC_STATUS _status)
{
  return std::format("{} failed with {:#010x}", _step, static_cast<std::uint32_t>(_status));
}

// The folder of the module that holds this code: the program, or the test library a test host loaded, beside which
// the build copies msquic.dll (ADR-089).
std::filesystem::path ModuleFolder()
{
  HMODULE module = nullptr;
  if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                         reinterpret_cast<LPCWSTR>(&ModuleFolder), &module) == FALSE)
  {
    return {};
  }
  std::vector<wchar_t> path(MAX_PATH);
  while (true)
  {
    const DWORD length = GetModuleFileNameW(module, path.data(), static_cast<DWORD>(path.size()));
    if (length == 0)
    {
      return {};
    }
    if (length < path.size())
    {
      return std::filesystem::path(std::wstring(path.data(), length)).parent_path();
    }
    path.resize(path.size() * 2);
  }
}

// The ALPN of a client's session as MsQuic takes it, which it reads only during the call it is given to.
class Alpn
{
public:
  Alpn() noexcept
  {
    std::ranges::transform(QUIC_ALPN_PLAY, m_bytes.begin(), [](char _letter) { return static_cast<std::uint8_t>(_letter); });
    m_buffer = QUIC_BUFFER{static_cast<std::uint32_t>(m_bytes.size()), m_bytes.data()};
  }
  Alpn(const Alpn&) = delete;
  Alpn& operator=(const Alpn&) = delete;
  ~Alpn() = default;

  [[nodiscard]] const QUIC_BUFFER* Buffer() const noexcept
  {
    return &m_buffer;
  }

private:
  std::array<std::uint8_t, QUIC_ALPN_PLAY.size()> m_bytes{};
  QUIC_BUFFER m_buffer{};
};

// ADR-088 item 3: settings both ends state. Send buffering is off, so that MsQuic completes a send once its bytes are
// acknowledged, which is what the reliable backlog counts.
QUIC_SETTINGS CommonSettings() noexcept
{
  QUIC_SETTINGS settings{};
  settings.DatagramReceiveEnabled = TRUE;
  settings.IsSet.DatagramReceiveEnabled = TRUE;
  settings.SendBufferingEnabled = FALSE;
  settings.IsSet.SendBufferingEnabled = TRUE;
  settings.PeerUnidiStreamCount = 0;
  settings.IsSet.PeerUnidiStreamCount = TRUE;
  return settings;
}

QUIC_SETTINGS ClientSettings() noexcept
{
  QUIC_SETTINGS settings = CommonSettings();
  settings.IdleTimeoutMs = QUIC_CLIENT_IDLE_TIMEOUT_MS;
  settings.IsSet.IdleTimeoutMs = TRUE;
  settings.KeepAliveIntervalMs = QUIC_CLIENT_KEEP_ALIVE_MS;
  settings.IsSet.KeepAliveIntervalMs = TRUE;
  settings.PeerBidiStreamCount = 0; // the server opens no stream
  settings.IsSet.PeerBidiStreamCount = TRUE;
  return settings;
}

QUIC_SETTINGS ServerSettings() noexcept
{
  QUIC_SETTINGS settings = CommonSettings();
  settings.IdleTimeoutMs = QUIC_SERVER_IDLE_TIMEOUT_MS;
  settings.IsSet.IdleTimeoutMs = TRUE;
  settings.PeerBidiStreamCount = 1; // the client's reliable channel
  settings.IsSet.PeerBidiStreamCount = TRUE;
  settings.ServerResumptionLevel = static_cast<std::uint8_t>(QUIC_SERVER_NO_RESUME); // no 0-RTT data (ADR-088 item 4)
  settings.IsSet.ServerResumptionLevel = TRUE;
  return settings;
}

QUIC_STATUS QUIC_API OnStreamEvent(HQUIC _stream, void* _context, QUIC_STREAM_EVENT* _event)
{
  try
  {
    QuicLink& link = *static_cast<QuicLink*>(_context);
    const QUIC_API_TABLE& api = link.library->Api();
    switch (_event->Type)
    {
    case QUIC_STREAM_EVENT_RECEIVE:
    {
      bool corrupt = false;
      bool pause = false;
      {
        const std::lock_guard lock(link.mutex);
        for (const QUIC_BUFFER& buffer : std::span(_event->RECEIVE.Buffers, _event->RECEIVE.BufferCount))
        {
          link.reader.Feed(std::as_bytes(std::span(buffer.Buffer, buffer.Length)));
        }
        while (std::optional<std::vector<std::byte>> message = link.reader.Next())
        {
          link.inboundReliableBytes += message->size();
          link.inbound.push_back(ReceivedMessage{Channel::Reliable, std::move(*message)});
        }
        // ADR-088 item 5: a stream whose framing breaks cannot be read on, and the connection closes.
        corrupt = link.reader.Corrupt() && link.reason != CloseReason::ProtocolError;
        if (corrupt)
        {
          link.reason = CloseReason::ProtocolError;
          link.closed = true;
        }
        pause = !link.receivePaused && link.inboundReliableBytes > QUIC_RELIABLE_BACKLOG_BYTES;
        link.receivePaused = link.receivePaused || pause;
      }
      if (corrupt)
      {
        api.ConnectionShutdown(link.connection, QUIC_CONNECTION_SHUTDOWN_FLAG_NONE, QUIC_PROTOCOL_ERROR_CODE);
      }
      if (pause)
      {
        static_cast<void>(api.StreamReceiveSetEnabled(_stream, FALSE));
      }
      break;
    }
    case QUIC_STREAM_EVENT_SEND_COMPLETE:
    {
      const std::unique_ptr<QuicLink::SendBlock> block(static_cast<QuicLink::SendBlock*>(_event->SEND_COMPLETE.ClientContext));
      const std::lock_guard lock(link.mutex);
      link.inFlightBytes -= block->bytes.size();
      break;
    }
    case QUIC_STREAM_EVENT_SEND_SHUTDOWN_COMPLETE:
    {
      // Every reliable byte this end sent was acknowledged (or the stream was aborted): a closing end now ends the
      // connection, after its messages reached the other end.
      bool shutDown = false;
      {
        const std::lock_guard lock(link.mutex);
        link.sendShutdownComplete = true;
        shutDown = link.reason == CloseReason::ClosedLocally;
      }
      link.changed.notify_all();
      if (shutDown)
      {
        api.ConnectionShutdown(link.connection, QUIC_CONNECTION_SHUTDOWN_FLAG_NONE, 0);
      }
      break;
    }
    case QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE:
    {
      // A stream the link refused is closed here; the link's own is closed by its end.
      bool refused = false;
      {
        const std::lock_guard lock(link.mutex);
        refused = _stream != link.stream;
      }
      if (refused)
      {
        api.StreamClose(_stream);
      }
      break;
    }
    default:
      break;
    }
    return QUIC_STATUS_SUCCESS;
  }
  catch (...)
  {
    std::terminate(); // an allocation that fails on MsQuic's thread is a defect, and it ends through the fatal path
  }
}

QUIC_STATUS QUIC_API OnConnectionEvent(HQUIC _connection, void* _context, QUIC_CONNECTION_EVENT* _event)
{
  try
  {
    QuicLink& link = *static_cast<QuicLink*>(_context);
    const QUIC_API_TABLE& api = link.library->Api();
    switch (_event->Type)
    {
    case QUIC_CONNECTION_EVENT_PEER_CERTIFICATE_RECEIVED:
    {
      // ADR-088 item 4: the operating system's chain validation is off for the self-signed certificate, and the client
      // takes the server it pins and no other.
      const QUIC_CERTIFICATE* certificate = _event->PEER_CERTIFICATE_RECEIVED.Certificate;
      const bool pinned = certificate != nullptr && CertificateFingerprint(certificate) == link.pinned;
      const std::lock_guard lock(link.mutex);
      link.certificateChecked = true;
      link.certificateRefused = !pinned;
      return pinned ? QUIC_STATUS_SUCCESS : QUIC_STATUS_BAD_CERTIFICATE;
    }
    case QUIC_CONNECTION_EVENT_CONNECTED:
    {
      bool refuse = false;
      {
        const std::lock_guard lock(link.mutex);
        // A client whose pin was never checked refuses the server, so that the pin cannot be skipped.
        refuse = !link.server && !link.certificateChecked;
        if (refuse)
        {
          link.certificateRefused = true;
          link.MarkClosed(CloseReason::ProtocolError);
        }
        else
        {
          link.connected = true;
        }
      }
      if (refuse)
      {
        api.ConnectionShutdown(_connection, QUIC_CONNECTION_SHUTDOWN_FLAG_NONE, QUIC_PROTOCOL_ERROR_CODE);
      }
      break;
    }
    case QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED:
    {
      // ADR-088 item 2: the server takes the client's one bidirectional stream as the reliable channel; any other is
      // refused, and closed when it has shut down.
      const HQUIC stream = _event->PEER_STREAM_STARTED.Stream;
      api.SetCallbackHandler(stream, reinterpret_cast<void*>(&OnStreamEvent), &link);
      bool taken = false;
      {
        const std::lock_guard lock(link.mutex);
        taken = link.server && !link.ending && link.stream == nullptr &&
                (_event->PEER_STREAM_STARTED.Flags & QUIC_STREAM_OPEN_FLAG_UNIDIRECTIONAL) == 0;
        if (taken)
        {
          link.stream = stream;
        }
      }
      if (!taken)
      {
        static_cast<void>(api.StreamShutdown(stream, QUIC_STREAM_SHUTDOWN_FLAG_ABORT, QUIC_PROTOCOL_ERROR_CODE));
      }
      break;
    }
    case QUIC_CONNECTION_EVENT_DATAGRAM_STATE_CHANGED:
    {
      const std::lock_guard lock(link.mutex);
      link.datagramsEnabled = _event->DATAGRAM_STATE_CHANGED.SendEnabled != FALSE;
      link.datagramMaxSendBytes = _event->DATAGRAM_STATE_CHANGED.MaxSendLength;
      break;
    }
    case QUIC_CONNECTION_EVENT_DATAGRAM_RECEIVED:
    {
      const QUIC_BUFFER& buffer = *_event->DATAGRAM_RECEIVED.Buffer;
      const std::lock_guard lock(link.mutex);
      if (buffer.Length == 0 || buffer.Length > STATE_DATAGRAM_MAX_BYTES)
      {
        ++link.datagramsRefused;
        break;
      }
      const std::span<const std::byte> bytes = std::as_bytes(std::span(buffer.Buffer, buffer.Length));
      link.inbound.push_back(ReceivedMessage{Channel::State, std::vector<std::byte>(bytes.begin(), bytes.end())});
      break;
    }
    case QUIC_CONNECTION_EVENT_DATAGRAM_SEND_STATE_CHANGED:
    {
      // ADR-088 item 2: a datagram's fate is reported once, lost from the first suspicion; a later spurious
      // acknowledgement changes nothing, because the sender has already given up its update as a baseline.
      auto* block = static_cast<QuicLink::SendBlock*>(_event->DATAGRAM_SEND_STATE_CHANGED.ClientContext);
      const QUIC_DATAGRAM_SEND_STATE state = _event->DATAGRAM_SEND_STATE_CHANGED.State;
      {
        const std::lock_guard lock(link.mutex);
        if (state == QUIC_DATAGRAM_SEND_SENT)
        {
          ++link.datagramsSent;
          link.datagramBytesSent += block->bytes.size();
        }
        if (!block->decided && state != QUIC_DATAGRAM_SEND_UNKNOWN && state != QUIC_DATAGRAM_SEND_SENT)
        {
          const bool acknowledged = state == QUIC_DATAGRAM_SEND_ACKNOWLEDGED || state == QUIC_DATAGRAM_SEND_ACKNOWLEDGED_SPURIOUS;
          link.outcomes.push_back({block->datagram, acknowledged ? DatagramFate::Acknowledged : DatagramFate::Lost});
          ++(acknowledged ? link.stats.datagramsAcknowledged : link.stats.datagramsLost);
          block->decided = true;
        }
      }
      if (QUIC_DATAGRAM_SEND_STATE_IS_FINAL(state))
      {
        delete block; // MsQuic holds it no more
      }
      break;
    }
    case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_TRANSPORT:
    {
      const std::lock_guard lock(link.mutex);
      link.MarkClosed(link.certificateRefused ? CloseReason::ProtocolError : CloseReason::ClosedByPeer);
      break;
    }
    case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_PEER:
    {
      const std::lock_guard lock(link.mutex);
      link.MarkClosed(CloseReason::ClosedByPeer);
      break;
    }
    case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE:
    {
      {
        const std::lock_guard lock(link.mutex);
        link.MarkClosed(CloseReason::ClosedByPeer);
        link.shutdownComplete = true;
      }
      link.changed.notify_all();
      break;
    }
    default:
      break;
    }
    return QUIC_STATUS_SUCCESS;
  }
  catch (...)
  {
    std::terminate();
  }
}

QUIC_STATUS QUIC_API OnListenerEvent(HQUIC /*_listener*/, void* _context, QUIC_LISTENER_EVENT* _event)
{
  try
  {
    if (_event->Type == QUIC_LISTENER_EVENT_NEW_CONNECTION)
    {
      return static_cast<QuicEndpoint*>(_context)->Arrived(_event->NEW_CONNECTION.Connection);
    }
    return QUIC_STATUS_SUCCESS;
  }
  catch (...)
  {
    std::terminate();
  }
}

} // namespace

QuicLibrary::~QuicLibrary()
{
  if (m_registration != nullptr)
  {
    m_api->RegistrationClose(m_registration);
  }
  if (m_api != nullptr)
  {
    m_close(m_api);
  }
  if (m_module != nullptr)
  {
    static_cast<void>(FreeLibrary(m_module));
  }
}

std::expected<std::shared_ptr<QuicLibrary>, std::string> QuicLibrary::Acquire()
{
  static std::mutex g_mutex;
  static std::weak_ptr<QuicLibrary> g_library;
  const std::lock_guard lock(g_mutex);
  if (std::shared_ptr<QuicLibrary> held = g_library.lock())
  {
    return held;
  }
  auto library = std::make_shared<QuicLibrary>();
  // PLT-100: only the transport loads MsQuic, and only the pinned copy beside it; the system's own is never taken.
  const std::filesystem::path path = ModuleFolder() / L"msquic.dll";
  library->m_module = LoadLibraryExW(path.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
  if (library->m_module == nullptr)
  {
    const DWORD error = GetLastError();
    return std::unexpected(std::format("cannot load {}: error {}", path.string(), error));
  }
  // GetProcAddress returns a generic function pointer; void (*)() is the type a function pointer is cast through.
  const auto open =
    reinterpret_cast<MsQuicOpenVersionFn>(reinterpret_cast<void (*)()>(GetProcAddress(library->m_module, "MsQuicOpenVersion")));
  library->m_close = reinterpret_cast<MsQuicCloseFn>(reinterpret_cast<void (*)()>(GetProcAddress(library->m_module, "MsQuicClose")));
  if (open == nullptr || library->m_close == nullptr)
  {
    return std::unexpected(std::format("{} has no MsQuicOpenVersion or MsQuicClose", path.string()));
  }
  const void* table = nullptr;
  if (const QUIC_STATUS status = open(QUIC_API_VERSION_2, &table); QUIC_FAILED(status))
  {
    return std::unexpected(Failure("MsQuicOpenVersion", status));
  }
  library->m_api = static_cast<const QUIC_API_TABLE*>(table);
  // ADR-088 item 4: every new address is first sent a stateless Retry.
  const std::uint16_t retryMemoryPercent = 0;
  if (const QUIC_STATUS status = library->m_api->SetParam(nullptr, QUIC_PARAM_GLOBAL_RETRY_MEMORY_PERCENT,
                                                          static_cast<std::uint32_t>(sizeof(retryMemoryPercent)), &retryMemoryPercent);
      QUIC_FAILED(status))
  {
    return std::unexpected(Failure("QUIC_PARAM_GLOBAL_RETRY_MEMORY_PERCENT", status));
  }
  const QUIC_REGISTRATION_CONFIG registration{"Species", QUIC_EXECUTION_PROFILE_LOW_LATENCY};
  if (const QUIC_STATUS status = library->m_api->RegistrationOpen(&registration, &library->m_registration); QUIC_FAILED(status))
  {
    return std::unexpected(Failure("RegistrationOpen", status));
  }
  g_library = library;
  return library;
}

QuicTransport::QuicTransport(std::shared_ptr<QuicLink> _link) noexcept
  : m_link(std::move(_link))
{
}

std::expected<std::unique_ptr<QuicTransport>, std::string> QuicTransport::Connect(const ConnectDesc& _desc)
{
  std::expected<std::shared_ptr<QuicLibrary>, std::string> library = QuicLibrary::Acquire();
  if (!library)
  {
    return std::unexpected(std::move(library.error()));
  }
  auto link = std::make_shared<QuicLink>();
  link->library = *library;
  link->pinned = _desc.serverFingerprint;
  // From here the end owns each handle it opens, and its destructor closes them, on success or failure.
  std::unique_ptr<QuicTransport> transport(new QuicTransport(link));
  const QUIC_API_TABLE& api = link->library->Api();
  const Alpn alpn;
  const QUIC_SETTINGS settings = ClientSettings();
  if (const QUIC_STATUS status = api.ConfigurationOpen(link->library->Registration(), alpn.Buffer(), 1, &settings,
                                                       static_cast<std::uint32_t>(sizeof(settings)), nullptr, &link->configuration);
      QUIC_FAILED(status))
  {
    return std::unexpected(Failure("ConfigurationOpen", status));
  }
  QUIC_CREDENTIAL_CONFIG credential{};
  credential.Type = QUIC_CREDENTIAL_TYPE_NONE;
  credential.Flags =
    QUIC_CREDENTIAL_FLAG_CLIENT | QUIC_CREDENTIAL_FLAG_NO_CERTIFICATE_VALIDATION | QUIC_CREDENTIAL_FLAG_INDICATE_CERTIFICATE_RECEIVED;
  if (const QUIC_STATUS status = api.ConfigurationLoadCredential(link->configuration, &credential); QUIC_FAILED(status))
  {
    return std::unexpected(Failure("ConfigurationLoadCredential", status));
  }
  if (const QUIC_STATUS status = api.ConnectionOpen(link->library->Registration(), &OnConnectionEvent, link.get(), &link->connection);
      QUIC_FAILED(status))
  {
    return std::unexpected(Failure("ConnectionOpen", status));
  }
  const std::string host(QUIC_LOOPBACK_HOST);
  if (const QUIC_STATUS status =
        api.ConnectionStart(link->connection, link->configuration, QUIC_ADDRESS_FAMILY_INET, host.c_str(), _desc.port);
      QUIC_FAILED(status))
  {
    return std::unexpected(Failure("ConnectionStart", status));
  }
  return transport;
}

QuicTransport::~QuicTransport()
{
  try
  {
    QuicLink& link = *m_link;
    const QUIC_API_TABLE& api = link.library->Api();
    HQUIC stream = nullptr;
    bool open = false;
    {
      std::unique_lock lock(link.mutex);
      // An end that was closed gives the reliable messages on its stream a moment to be acknowledged before it ends them.
      if (link.reason == CloseReason::ClosedLocally && link.stream != nullptr)
      {
        static_cast<void>(link.changed.wait_for(lock, std::chrono::milliseconds(QUIC_CLOSE_WAIT_MS),
                                                [&link] { return link.sendShutdownComplete || link.shutdownComplete; }));
      }
      link.ending = true;
      stream = link.stream;
      open = !link.closed;
    }
    // An end that ends open tells its peer, which would otherwise wait for its idle timeout: closing the handle alone
    // ends the connection silently.
    if (open && link.connection != nullptr)
    {
      api.ConnectionShutdown(link.connection, QUIC_CONNECTION_SHUTDOWN_FLAG_NONE, 0);
    }
    if (stream != nullptr)
    {
      api.StreamClose(stream);
    }
    if (link.connection != nullptr)
    {
      api.ConnectionClose(link.connection);
    }
    if (link.configuration != nullptr)
    {
      api.ConfigurationClose(link.configuration);
    }
  }
  catch (...)
  {
    std::terminate();
  }
}

void QuicTransport::Flush()
{
  QuicLink& link = *m_link;
  const QUIC_API_TABLE& api = link.library->Api();
  bool open = false;
  {
    const std::lock_guard lock(link.mutex);
    open = !link.server && link.connected && !link.closed && link.stream == nullptr;
  }
  if (open)
  {
    // ADR-088 item 2: the client opens the reliable channel once the handshake completes, on its own thread, so that
    // the stream's handle is made and closed by the end alone. The handle is the link's before the stream starts, so
    // that a callback never meets a stream the link does not know.
    HQUIC opened = nullptr;
    QUIC_STATUS status = api.StreamOpen(link.connection, QUIC_STREAM_OPEN_FLAG_NONE, &OnStreamEvent, &link, &opened);
    {
      const std::lock_guard lock(link.mutex);
      link.stream = opened;
    }
    if (QUIC_SUCCEEDED(status))
    {
      status = api.StreamStart(opened, QUIC_STREAM_START_FLAG_IMMEDIATE);
    }
    if (QUIC_FAILED(status))
    {
      {
        const std::lock_guard lock(link.mutex);
        link.MarkClosed(CloseReason::ClosedLocally);
      }
      api.ConnectionShutdown(link.connection, QUIC_CONNECTION_SHUTDOWN_FLAG_NONE, 0);
      return;
    }
  }
  std::vector<std::byte> bytes;
  HQUIC stream = nullptr;
  {
    const std::lock_guard lock(link.mutex);
    if (link.stream == nullptr || link.waiting.empty() || link.sendShutdownComplete)
    {
      return;
    }
    stream = link.stream;
    bytes = std::exchange(link.waiting, {});
    link.inFlightBytes += bytes.size();
  }
  auto block = std::make_unique<QuicLink::SendBlock>();
  block->bytes = std::move(bytes);
  block->buffer = QUIC_BUFFER{static_cast<std::uint32_t>(block->bytes.size()), reinterpret_cast<std::uint8_t*>(block->bytes.data())};
  QuicLink::SendBlock* sent = block.release();
  if (const QUIC_STATUS status = api.StreamSend(stream, &sent->buffer, 1, QUIC_SEND_FLAG_NONE, sent); QUIC_FAILED(status))
  {
    // The connection is ending, and the messages end with it.
    const std::unique_ptr<QuicLink::SendBlock> failed(sent);
    const std::lock_guard lock(link.mutex);
    link.inFlightBytes -= failed->bytes.size();
  }
}

std::expected<std::uint64_t, SendError> QuicTransport::Send(Channel _channel, std::span<const std::byte> _message)
{
  if (_message.empty())
  {
    return std::unexpected(SendError::Empty);
  }
  if (_message.size() > MESSAGE_MAX_BYTES || (_channel == Channel::State && _message.size() > STATE_DATAGRAM_MAX_BYTES))
  {
    return std::unexpected(SendError::TooLarge);
  }
  QuicLink& link = *m_link;
  if (_channel == Channel::Reliable)
  {
    {
      const std::lock_guard lock(link.mutex);
      if (link.closed)
      {
        return std::unexpected(SendError::Closed);
      }
      const std::size_t framedBytes = _message.size() + FRAME_LENGTH_MAX_BYTES;
      if (link.waiting.size() + link.inFlightBytes + framedBytes > QUIC_RELIABLE_BACKLOG_BYTES)
      {
        return std::unexpected(SendError::Backlogged);
      }
      AppendFrame(link.waiting, _message);
      ++link.stats.messagesSent;
      link.stats.bytesSent += _message.size();
    }
    Flush();
    return 0;
  }
  auto block = std::make_unique<QuicLink::SendBlock>();
  {
    const std::lock_guard lock(link.mutex);
    if (link.closed)
    {
      return std::unexpected(SendError::Closed);
    }
    if (link.datagramsEnabled && _message.size() > link.datagramMaxSendBytes)
    {
      return std::unexpected(SendError::TooLarge);
    }
    block->datagram = link.nextDatagram++;
    ++link.stats.messagesSent;
    link.stats.bytesSent += _message.size();
    if (!link.datagramsEnabled)
    {
      // Before the peer takes datagrams, or once it stops, a state datagram is lost at once.
      link.outcomes.push_back({block->datagram, DatagramFate::Lost});
      ++link.stats.datagramsLost;
      return block->datagram;
    }
  }
  const std::uint64_t datagram = block->datagram;
  block->bytes.assign(_message.begin(), _message.end());
  block->buffer = QUIC_BUFFER{static_cast<std::uint32_t>(block->bytes.size()), reinterpret_cast<std::uint8_t*>(block->bytes.data())};
  QuicLink::SendBlock* sent = block.release();
  if (const QUIC_STATUS status = link.library->Api().DatagramSend(link.connection, &sent->buffer, 1, QUIC_SEND_FLAG_NONE, sent);
      QUIC_FAILED(status))
  {
    const std::unique_ptr<QuicLink::SendBlock> failed(sent);
    const std::lock_guard lock(link.mutex);
    link.outcomes.push_back({datagram, DatagramFate::Lost});
    ++link.stats.datagramsLost;
  }
  return datagram;
}

std::optional<ReceivedMessage> QuicTransport::Receive()
{
  Flush();
  QuicLink& link = *m_link;
  std::optional<ReceivedMessage> message;
  bool resume = false;
  HQUIC stream = nullptr;
  {
    const std::lock_guard lock(link.mutex);
    if (!link.inbound.empty())
    {
      message = std::move(link.inbound.front());
      link.inbound.pop_front();
      if (message->channel == Channel::Reliable)
      {
        link.inboundReliableBytes -= message->bytes.size();
      }
      ++link.stats.messagesReceived;
      link.stats.bytesReceived += message->bytes.size();
    }
    // A paused stream reads again once half its bound has been taken.
    resume = link.receivePaused && link.inboundReliableBytes <= QUIC_RELIABLE_BACKLOG_BYTES / 2;
    link.receivePaused = link.receivePaused && !resume;
    stream = link.stream;
  }
  if (resume)
  {
    static_cast<void>(link.library->Api().StreamReceiveSetEnabled(stream, TRUE));
  }
  return message;
}

std::vector<DatagramOutcome> QuicTransport::TakeDatagramOutcomes()
{
  const std::lock_guard lock(m_link->mutex);
  return std::exchange(m_link->outcomes, {});
}

void QuicTransport::Close()
{
  Flush();
  QuicLink& link = *m_link;
  const QUIC_API_TABLE& api = link.library->Api();
  HQUIC stream = nullptr;
  {
    const std::lock_guard lock(link.mutex);
    if (link.closed)
    {
      return;
    }
    link.MarkClosed(CloseReason::ClosedLocally);
    stream = link.stream;
  }
  // The stream's sending side ends after the bytes already sent, and the connection ends once they are acknowledged
  // (OnStreamEvent); an end with no stream yet ends the connection now, and the messages that waited for it with it.
  if (stream == nullptr || QUIC_FAILED(api.StreamShutdown(stream, QUIC_STREAM_SHUTDOWN_FLAG_GRACEFUL, 0)))
  {
    api.ConnectionShutdown(link.connection, QUIC_CONNECTION_SHUTDOWN_FLAG_NONE, 0);
  }
}

ConnectionState QuicTransport::State() const
{
  const std::lock_guard lock(m_link->mutex);
  return m_link->closed ? ConnectionState::Closed : ConnectionState::Open;
}

CloseReason QuicTransport::Reason() const
{
  const std::lock_guard lock(m_link->mutex);
  return m_link->reason;
}

TransportStats QuicTransport::Stats() const
{
  const std::lock_guard lock(m_link->mutex);
  TransportStats stats = m_link->stats;
  stats.reliableBacklogBytes = m_link->waiting.size() + m_link->inFlightBytes;
  return stats;
}

bool QuicTransport::Connected() const
{
  const std::lock_guard lock(m_link->mutex);
  return m_link->connected;
}

bool QuicTransport::CertificateRefused() const
{
  const std::lock_guard lock(m_link->mutex);
  return m_link->certificateRefused;
}

std::optional<QuicPathStats> QuicTransport::PathStats() const
{
  const QuicLink& link = *m_link;
  QUIC_STATISTICS_V2 statistics{};
  auto size = static_cast<std::uint32_t>(sizeof(statistics));
  if (link.connection == nullptr ||
      QUIC_FAILED(link.library->Api().GetParam(link.connection, QUIC_PARAM_CONN_STATISTICS_V2, &size, &statistics)))
  {
    return std::nullopt;
  }
  QuicPathStats stats;
  stats.roundTripMicroseconds = statistics.Rtt;
  stats.roundTripVarianceMicroseconds = statistics.RttVariance;
  stats.congestionWindowBytes = statistics.SendCongestionWindow;
  stats.pathMtuBytes = statistics.SendPathMtu;
  stats.packetsSent = statistics.SendTotalPackets;
  stats.packetsSuspectedLost = statistics.SendSuspectedLostPackets;
  stats.packetsSpuriouslyLost = statistics.SendSpuriousLostPackets;
  stats.packetsReceived = statistics.RecvTotalPackets;
  stats.udpBytesSent = statistics.SendTotalBytes;
  stats.udpBytesReceived = statistics.RecvTotalBytes;
  stats.streamBytesSent = statistics.SendTotalStreamBytes;
  stats.streamBytesReceived = statistics.RecvTotalStreamBytes;
  stats.congestionEvents = statistics.SendCongestionCount;
  const std::lock_guard lock(link.mutex);
  stats.datagramsEnabled = link.datagramsEnabled;
  stats.datagramMaxSendBytes = link.datagramMaxSendBytes;
  stats.datagramsRefused = link.datagramsRefused;
  stats.datagramsSent = link.datagramsSent;
  stats.datagramBytesSent = link.datagramBytesSent;
  return stats;
}

QuicEndpoint::~QuicEndpoint()
{
  try
  {
    // Closing the listener waits for its callbacks, so no connection arrives after it; then each one is closed.
    if (listener != nullptr)
    {
      library->Api().ListenerClose(listener);
    }
    arrived.clear();
    if (configuration != nullptr)
    {
      library->Api().ConfigurationClose(configuration);
    }
  }
  catch (...)
  {
    std::terminate();
  }
}

QUIC_STATUS QuicEndpoint::Arrived(HQUIC _connection)
{
  const QUIC_API_TABLE& api = library->Api();
  auto link = std::make_shared<QuicLink>();
  link->library = library;
  link->server = true;
  link->connection = _connection;
  api.SetCallbackHandler(_connection, reinterpret_cast<void*>(&OnConnectionEvent), link.get());
  // The connection is the end's from here, whether its configuration is taken or not: one that is not is ended, and
  // Take closes it with the others that never completed their handshake.
  const QUIC_STATUS status = api.ConnectionSetConfiguration(_connection, configuration);
  if (QUIC_FAILED(status))
  {
    {
      const std::lock_guard lock(link->mutex);
      link->MarkClosed(CloseReason::ClosedLocally);
    }
    api.ConnectionShutdown(_connection, QUIC_CONNECTION_SHUTDOWN_FLAG_NONE, 0);
  }
  const std::lock_guard lock(mutex);
  arrived.push_back(std::unique_ptr<QuicTransport>(new QuicTransport(std::move(link))));
  return QUIC_STATUS_SUCCESS;
}

std::unique_ptr<Transport> QuicEndpoint::Take()
{
  std::vector<std::unique_ptr<QuicTransport>> ended;
  std::unique_ptr<Transport> taken;
  {
    const std::lock_guard lock(mutex);
    for (auto entry = arrived.begin(); entry != arrived.end();)
    {
      const QuicLink& link = *(*entry)->m_link;
      bool connected = false;
      bool closed = false;
      {
        const std::lock_guard linkLock(link.mutex);
        connected = link.connected;
        closed = link.closed;
      }
      if (connected)
      {
        taken = std::move(*entry);
        arrived.erase(entry);
        break;
      }
      if (closed)
      {
        ended.push_back(std::move(*entry));
        entry = arrived.erase(entry);
        continue;
      }
      ++entry;
    }
  }
  // Closing a connection waits for MsQuic's thread, which may wait for the lock in Arrived, so it is done after it.
  ended.clear();
  return taken;
}

QuicListener::QuicListener(std::unique_ptr<QuicEndpoint> _endpoint) noexcept
  : m_endpoint(std::move(_endpoint))
{
}

QuicListener::~QuicListener() = default;

std::expected<std::unique_ptr<QuicListener>, std::string> QuicListener::Start(const Desc& _desc)
{
  if (_desc.certificate == nullptr)
  {
    return std::unexpected(std::string("a listener needs the server's certificate"));
  }
  std::expected<std::shared_ptr<QuicLibrary>, std::string> library = QuicLibrary::Acquire();
  if (!library)
  {
    return std::unexpected(std::move(library.error()));
  }
  // From here the endpoint owns each handle it opens, and its destructor closes them, on success or failure.
  auto endpoint = std::make_unique<QuicEndpoint>();
  endpoint->library = *library;
  const QUIC_API_TABLE& api = endpoint->library->Api();
  const Alpn alpn;
  const QUIC_SETTINGS settings = ServerSettings();
  if (const QUIC_STATUS status = api.ConfigurationOpen(endpoint->library->Registration(), alpn.Buffer(), 1, &settings,
                                                       static_cast<std::uint32_t>(sizeof(settings)), nullptr, &endpoint->configuration);
      QUIC_FAILED(status))
  {
    return std::unexpected(Failure("ConfigurationOpen", status));
  }
  QUIC_CREDENTIAL_CONFIG credential{};
  credential.Type = QUIC_CREDENTIAL_TYPE_CERTIFICATE_CONTEXT;
  credential.Flags = QUIC_CREDENTIAL_FLAG_NONE;
  // MsQuic's credential takes a mutable pointer and only reads the certificate through it.
  credential.CertificateContext = const_cast<void*>(_desc.certificate->Context());
  if (const QUIC_STATUS status = api.ConfigurationLoadCredential(endpoint->configuration, &credential); QUIC_FAILED(status))
  {
    return std::unexpected(Failure("ConfigurationLoadCredential", status));
  }
  if (const QUIC_STATUS status = api.ListenerOpen(endpoint->library->Registration(), &OnListenerEvent, endpoint.get(), &endpoint->listener);
      QUIC_FAILED(status))
  {
    return std::unexpected(Failure("ListenerOpen", status));
  }
  QUIC_ADDR address{};
  QuicAddrSetFamily(&address, QUIC_ADDRESS_FAMILY_INET);
  QuicAddrSetToLoopback(&address);
  QuicAddrSetPort(&address, _desc.port);
  if (const QUIC_STATUS status = api.ListenerStart(endpoint->listener, alpn.Buffer(), 1, &address); QUIC_FAILED(status))
  {
    return std::unexpected(Failure("ListenerStart", status));
  }
  QUIC_ADDR bound{};
  auto size = static_cast<std::uint32_t>(sizeof(bound));
  if (const QUIC_STATUS status = api.GetParam(endpoint->listener, QUIC_PARAM_LISTENER_LOCAL_ADDRESS, &size, &bound); QUIC_FAILED(status))
  {
    return std::unexpected(Failure("QUIC_PARAM_LISTENER_LOCAL_ADDRESS", status));
  }
  endpoint->port = QuicAddrGetPort(&bound);
  return std::unique_ptr<QuicListener>(new QuicListener(std::move(endpoint)));
}

std::uint16_t QuicListener::Port() const noexcept
{
  return m_endpoint->port;
}

std::unique_ptr<Transport> QuicListener::Accept()
{
  return m_endpoint->Take();
}

} // namespace Neuron
