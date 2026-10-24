#pragma once

#include "GameMessages.h"
#include "Sha256.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace Species
{

// ADR-088 item 6, ADR-190: the version of the messages a client and a server exchange, raised by every change to one
// of them. JoinCommand and JoinAnswer never change their layout, so that any two builds read each other's join.
inline constexpr std::uint16_t GAME_PROTOCOL_VERSION = 1;

// ADR-190: the programs of one build, whose files' stamp is the build identity both of them send and check until
// ADR-057's generated identity exists.
inline constexpr std::array<std::wstring_view, 2> BUILD_PROGRAMS{L"Species.exe", L"SpeciesServer.exe"};

/// ADR-088 item 6 (ADR-190): what a client and a server compare when the client joins: the messages' version, the build
/// and the simulation-relevant content (ADR-060, AST-100).
struct JoinIdentity
{
  std::uint16_t protocol = GAME_PROTOCOL_VERSION;
  Neuron::Sha256Digest build{};
  Neuron::Sha256Digest content{};

  friend constexpr bool operator==(const JoinIdentity& _a, const JoinIdentity& _b) noexcept = default;
};

/// Why a server refused a client's join. Values are on the wire and never renumbered.
enum class JoinRefusal : std::uint8_t
{
  None = 0,     // joined
  Protocol = 1, // the client speaks another version of the messages
  Build = 2,    // the client is of another build
  Content = 3   // the client mounts other simulation-relevant content
};

inline constexpr std::uint64_t JOIN_REFUSAL_MAX = 7;

// The refusal as a log and the client's message name it: "joined", "another protocol", "another build", "other content".
[[nodiscard]] std::string_view JoinRefusalName(JoinRefusal _refusal) noexcept;

// ADR-190: this program's own identity: its build's stamp, the files of BUILD_PROGRAMS beside it, and _content, the
// simulation content identity of the content it mounted.
[[nodiscard]] JoinIdentity OwnJoinIdentity(const Neuron::Sha256Digest& _content);

// The first difference that refuses a client of _client on a server of _server: the protocol, then the build, then the
// content; None when they agree.
[[nodiscard]] JoinRefusal JoinRefusalOf(const JoinIdentity& _server, const JoinIdentity& _client) noexcept;

/// ADR-190: a client's first message on its connection, before which the server takes nothing else from it.
struct JoinCommand
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::Join);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 69; // the 3-byte header, the 16-bit protocol and the two 256-bit digests

  JoinIdentity identity;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

/// ADR-190: the server's answer to a join, its first message on the connection: the refusal, None when the client
/// joined, and the server's own identity, which a refused client shows. The server closes a refused connection.
struct JoinAnswer
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::JoinAnswer);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 70; // the 3-byte header, the refusal and the server's identity

  JoinRefusal refusal = JoinRefusal::None;
  JoinIdentity server;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

} // namespace Species
