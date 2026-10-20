#pragma once

#include "MessageCodec.h"
#include "SipHash.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <utility>
#include <vector>

namespace Neuron
{

// Who sent a command: the stable id (ADR-083 item 5) of a steward, a settlement's deputy, an AI settlement, the world
// host or a server API caller. Every source takes the same path (ADR-087 item 1).
using SenderId = std::uint64_t;

// ADR-082 item 5: the domain of the keyed value that orders the senders of a tick.
inline constexpr std::uint64_t SENDER_ORDER_DOMAIN = 3;
// The most commands an owner's queue holds in its saved state (ADR-096): a bound for the reader, far above a tick's.
inline constexpr std::uint64_t COMMAND_QUEUE_STATE_MAX = 65'535;

// The sender's place in the keyed order of a tick (ADR-087 item 4): a SipHash-2-4 value of the simulation key over the
// domain, the tick and the sender, which no client can compute and which changes every tick.
[[nodiscard]] std::uint64_t SenderOrderValue(const SipHashKey& _key, std::uint64_t _tick, SenderId _sender) noexcept;

// Why an arriving command was dropped (ADR-087 item 2); each is counted and reported to its sender.
enum class CommandRejection : std::uint8_t
{
  UnknownType,  // no handler for its type
  WrongVersion, // a version its handler does not read
  Oversized,    // larger than its type allows
  Malformed,    // a field out of bounds, a missing field, bad padding or bad UTF-8
  NotPermitted, // the sender has no right over what it names, or the phase does not allow it
  Duplicate     // a sequence number its sender already used in this tick
};

// What an executed command did (ADR-087 item 3): a command whose target has changed since it was issued does nothing.
enum class CommandEffect : std::uint8_t
{
  Applied,
  NoEffect
};

/// One command's outcome, reported to its sender (ADR-087 item 4).
struct CommandOutcome
{
  SenderId sender = 0;
  std::uint64_t sequence = 0;
  std::uint64_t tick = 0;                    // the tick that executed it, or the next tick when it was refused
  std::optional<CommandRejection> rejection; // empty when it executed
  CommandEffect effect = CommandEffect::NoEffect;
};

struct CommandRejectionCounts
{
  std::uint64_t unknownType = 0;
  std::uint64_t wrongVersion = 0;
  std::uint64_t oversized = 0;
  std::uint64_t malformed = 0;
  std::uint64_t notPermitted = 0;
  std::uint64_t duplicate = 0;

  [[nodiscard]] std::uint64_t Total() const noexcept
  {
    return unknownType + wrongVersion + oversized + malformed + notPermitted + duplicate;
  }
};

/// The command queue of one state owner (ADR-087 item 1): the only way a command enters the simulation. A command is
/// checked when it arrives (ADR-087 item 2), executes in the first tick that starts after it arrived, and is checked
/// again by its handler against that tick's state (item 3). The senders of a tick execute in their keyed order, each
/// sender's commands in the order they arrived (item 4). A command submitted while a tick executes, by an AI settlement
/// or the deputy, executes in the next tick like any other. Used from the owner's thread only.
class CommandQueue
{
public:
  // _nextTick is the tick that the first queued command will execute in.
  CommandQueue(const SipHashKey& _simulationKey, std::uint64_t _nextTick)
    : m_key(_simulationKey),
      m_nextTick(_nextTick)
  {
  }

  // _permitted is the arrival check of the sender's right and the phase; an empty one permits every sender.
  // _execute applies the command at the start of its tick and says whether it had an effect.
  template <MessageCodec T>
  void Register(std::function<bool(SenderId, const T&)> _permitted,
                std::function<CommandEffect(SenderId, const T&, std::uint64_t)> _execute);

  // A command arrives: true when it is queued for the next tick; otherwise its rejection is counted and reported.
  bool Submit(SenderId _sender, std::uint64_t _sequence, std::span<const std::byte> _message);

  // The start of tick _tick: executes every command queued before it, in the keyed order of the tick.
  void Execute(std::uint64_t _tick);

  [[nodiscard]] std::size_t Pending() const noexcept
  {
    return m_pending.size();
  }
  // The queued commands, in arrival order, as owner state (ADR-083 items 4 and 9): what a cut saves and the state hash
  // covers. A tick's commands from the deputy or the AI are queued in it for the next.
  void WriteState(BitWriter& _writer) const;
  // Queues the commands that WriteState wrote, checked again on arrival; false when the state does not read whole.
  [[nodiscard]] bool ReadState(BitReader& _reader);
  [[nodiscard]] std::vector<CommandOutcome> TakeOutcomes();
  [[nodiscard]] const CommandRejectionCounts& Rejections() const noexcept
  {
    return m_rejections;
  }

private:
  struct Entry
  {
    std::uint8_t version = 0;
    std::size_t maxBytes = 0;
    std::function<std::optional<CommandRejection>(SenderId, BitReader&)> check;
    std::function<CommandEffect(SenderId, BitReader&, std::uint64_t)> execute;
  };

  struct Queued
  {
    SenderId sender = 0;
    std::uint64_t sequence = 0;
    MessageTypeId type = 0;
    std::vector<std::byte> message;
  };

  void Reject(SenderId _sender, std::uint64_t _sequence, CommandRejection _reason);

  SipHashKey m_key;
  std::map<MessageTypeId, Entry> m_entries;
  std::vector<Queued> m_pending;
  std::set<std::pair<SenderId, std::uint64_t>> m_pendingSequences;
  std::vector<CommandOutcome> m_outcomes;
  CommandRejectionCounts m_rejections;
  std::uint64_t m_nextTick = 0;
};

template <MessageCodec T>
void CommandQueue::Register(std::function<bool(SenderId, const T&)> _permitted,
                            std::function<CommandEffect(SenderId, const T&, std::uint64_t)> _execute)
{
  // The bytes are kept and decoded again when the command executes, so a decoded field never refers to a buffer
  // that has gone; the second decoding of bytes that decoded once cannot fail.
  Entry entry;
  entry.version = T::VERSION;
  entry.maxBytes = T::MAX_BYTES;
  entry.check = [permitted = std::move(_permitted)](SenderId _sender, BitReader& _reader) -> std::optional<CommandRejection>
  {
    T command{};
    command.Decode(_reader);
    if (!_reader.Finish())
    {
      return CommandRejection::Malformed;
    }
    if (permitted && !permitted(_sender, command))
    {
      return CommandRejection::NotPermitted;
    }
    return std::nullopt;
  };
  entry.execute = [execute = std::move(_execute)](SenderId _sender, BitReader& _reader, std::uint64_t _tick)
  {
    T command{};
    command.Decode(_reader);
    return execute(_sender, command, _tick);
  };
  m_entries[T::TYPE] = std::move(entry);
}

} // namespace Neuron
