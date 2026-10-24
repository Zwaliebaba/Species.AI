#pragma once

#include "Sha256.h"
#include "Transport.h"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace Neuron
{

class ServerCertificate;

// ADR-088 item 2: the application protocol (ALPN) of a client's session.
inline constexpr std::string_view QUIC_ALPN_PLAY = "species-play";
// ADR-188: the interface a server listens on and a client connects to while all play is on one computer (O110).
inline constexpr std::string_view QUIC_LOOPBACK_HOST = "127.0.0.1";
// ADR-088 item 3: the client's keep-alive, one tick period, and each end's idle timeout. QUIC takes the smaller of the two
// ends' timeouts (RFC 9000 §10.1), so a connection whose peer vanished closes after the client's at both ends.
inline constexpr std::uint32_t QUIC_CLIENT_KEEP_ALIVE_MS = 100;
inline constexpr std::uint64_t QUIC_CLIENT_IDLE_TIMEOUT_MS = 15'000;
inline constexpr std::uint64_t QUIC_SERVER_IDLE_TIMEOUT_MS = 30'000;
// The bound of each end's reliable backlog, sent and not yet acknowledged, and of the reliable bytes received and not
// yet taken, as InProcessTransport's (ADR-093).
inline constexpr std::size_t QUIC_RELIABLE_BACKLOG_BYTES = std::size_t{4} * 1024 * 1024;
// How long an end that was closed waits, when it ends, for its reliable messages to be acknowledged.
inline constexpr std::uint32_t QUIC_CLOSE_WAIT_MS = 1'000;
// The application error code a connection is closed with when its peer broke the stream's framing (ADR-088 item 5).
inline constexpr std::uint64_t QUIC_PROTOCOL_ERROR_CODE = 1;

/// ADR-088 item 7: the figures of one connection that MsQuic measures, besides the transport's own (TransportStats).
struct QuicPathStats
{
  std::uint32_t roundTripMicroseconds = 0;
  std::uint32_t roundTripVarianceMicroseconds = 0;
  std::uint32_t congestionWindowBytes = 0;
  std::uint16_t pathMtuBytes = 0;
  std::uint64_t packetsSent = 0;
  std::uint64_t packetsSuspectedLost = 0;
  std::uint64_t packetsSpuriouslyLost = 0;
  std::uint64_t packetsReceived = 0;
  std::uint64_t udpBytesSent = 0; // the UDP payloads, every QUIC header and frame included
  std::uint64_t udpBytesReceived = 0;
  std::uint64_t streamBytesSent = 0; // the stream frames' payloads: the reliable channel's framed messages
  std::uint64_t streamBytesReceived = 0;
  std::uint32_t congestionEvents = 0;
  std::uint64_t datagramsSent = 0;        // the state datagrams MsQuic put in packets, at its SENT report
  std::uint64_t datagramBytesSent = 0;    // and their bytes, the message's header included
  bool datagramsEnabled = false;          // the peer takes QUIC datagrams (ADR-089)
  std::uint16_t datagramMaxSendBytes = 0; // the largest datagram MsQuic will send now (ADR-088 item 3)
  std::uint64_t datagramsRefused = 0;     // received empty or above STATE_DATAGRAM_MAX_BYTES, and dropped
};

struct QuicLink;
struct QuicEndpoint;

/// The remote transport (ADR-088 items 1 to 4, ADR-188): one QUIC connection through MsQuic (ADR-089), which owns the UDP
/// socket. The reliable channel is one bidirectional stream that the client opens when the handshake completes, each
/// message framed by its length (StreamFraming.h); the state channel is QUIC DATAGRAM frames, each reported acknowledged
/// or lost once. TLS 1.3 secures it with the server's self-signed certificate, which the client pins by its fingerprint.
/// Messages sent before the stream exists wait in the reliable backlog and leave at the next call on the end. A server end
/// has no stream until its client's first message opens it, so one closed before then ends the connection at once and
/// what waited goes with it; every client's first message is its join (ADR-189).
class QuicTransport final : public Transport
{
public:
  struct ConnectDesc
  {
    std::uint16_t port = 0;
    Sha256Digest serverFingerprint{}; // the server's certificate, which the client refuses to connect without
  };

  // Starts a connection to the server on the loopback interface. It returns at once: the connection then opens, or
  // closes with its reason, on MsQuic's threads. Fails when MsQuic cannot be loaded or the connection cannot start.
  [[nodiscard]] static std::expected<std::unique_ptr<QuicTransport>, std::string> Connect(const ConnectDesc& _desc);

  QuicTransport(const QuicTransport&) = delete;
  QuicTransport& operator=(const QuicTransport&) = delete;
  ~QuicTransport() override;

  [[nodiscard]] std::expected<std::uint64_t, SendError> Send(Channel _channel, std::span<const std::byte> _message) override;
  [[nodiscard]] std::optional<ReceivedMessage> Receive() override;
  [[nodiscard]] std::vector<DatagramOutcome> TakeDatagramOutcomes() override;
  void Close() override;
  [[nodiscard]] ConnectionState State() const override;
  [[nodiscard]] CloseReason Reason() const override;
  [[nodiscard]] TransportStats Stats() const override;

  // Whether the handshake has completed: the server is the one pinned, and the connection is secured.
  [[nodiscard]] bool Connected() const;
  // Whether the client closed the connection because the server's certificate is not the one it pins (ADR-088 item 4).
  [[nodiscard]] bool CertificateRefused() const;
  // MsQuic's figures of the connection, or nothing when it cannot give them.
  [[nodiscard]] std::optional<QuicPathStats> PathStats() const;

private:
  friend struct QuicEndpoint;

  explicit QuicTransport(std::shared_ptr<QuicLink> _link) noexcept;
  // Sends the reliable messages that waited for the stream, or for a backlog that drained.
  void Flush();

  std::shared_ptr<QuicLink> m_link;
};

/// A server's listener on MsQuic (ADR-088, ADR-188): it accepts each client's connection on the loopback interface,
/// secured by the server's certificate, once its handshake has completed.
class QuicListener final : public TransportListener
{
public:
  struct Desc
  {
    std::uint16_t port = 0;                         // 0 for one the system chooses
    const ServerCertificate* certificate = nullptr; // held by the caller for as long as the listener
  };

  [[nodiscard]] static std::expected<std::unique_ptr<QuicListener>, std::string> Start(const Desc& _desc);

  QuicListener(const QuicListener&) = delete;
  QuicListener& operator=(const QuicListener&) = delete;
  ~QuicListener() override;

  // The port it listens on.
  [[nodiscard]] std::uint16_t Port() const noexcept;
  // The next connection whose handshake has completed, in the order they arrived, or nullptr.
  [[nodiscard]] std::unique_ptr<Transport> Accept() override;

private:
  explicit QuicListener(std::unique_ptr<QuicEndpoint> _endpoint) noexcept;

  std::unique_ptr<QuicEndpoint> m_endpoint;
};

} // namespace Neuron
