#include "pch.h"

#include "Check.h"
#include "CommandQueue.h"

#include <algorithm>

namespace Neuron
{

std::uint64_t SenderOrderValue(const SipHashKey& _key, std::uint64_t _tick, SenderId _sender) noexcept
{
  return SipHash24Words(_key, {SENDER_ORDER_DOMAIN, _tick, _sender});
}

bool CommandQueue::Submit(SenderId _sender, std::uint64_t _sequence, std::span<const std::byte> _message)
{
  if (_message.size() > MESSAGE_MAX_BYTES)
  {
    Reject(_sender, _sequence, CommandRejection::Oversized);
    return false;
  }
  BitReader reader(_message);
  const MessageHeader header = ReadMessageHeader(reader);
  if (reader.Failed())
  {
    Reject(_sender, _sequence, CommandRejection::Malformed);
    return false;
  }
  const auto found = m_entries.find(header.type);
  if (found == m_entries.end())
  {
    Reject(_sender, _sequence, CommandRejection::UnknownType);
    return false;
  }
  const Entry& entry = found->second;
  if (header.version != entry.version)
  {
    Reject(_sender, _sequence, CommandRejection::WrongVersion);
    return false;
  }
  if (_message.size() > entry.maxBytes)
  {
    Reject(_sender, _sequence, CommandRejection::Oversized);
    return false;
  }
  if (const std::optional<CommandRejection> rejection = entry.check(_sender, reader))
  {
    Reject(_sender, _sequence, *rejection);
    return false;
  }
  // Within one tick a sender's sequence number executes once (ADR-087 item 6).
  if (!m_pendingSequences.emplace(_sender, _sequence).second)
  {
    Reject(_sender, _sequence, CommandRejection::Duplicate);
    return false;
  }
  m_pending.push_back(Queued{_sender, _sequence, header.type, std::vector<std::byte>(_message.begin(), _message.end())});
  return true;
}

void CommandQueue::Execute(std::uint64_t _tick)
{
  std::vector<Queued> batch;
  batch.swap(m_pending);
  m_pendingSequences.clear();
  m_nextTick = _tick + 1;

  // The senders in the keyed order of this tick; the order ends with the sender's id, so it is total (ADR-081 item 2).
  // A stable sort keeps each sender's commands in the order they arrived.
  std::map<SenderId, std::uint64_t> orderValues;
  for (const Queued& queued : batch)
  {
    orderValues.try_emplace(queued.sender, SenderOrderValue(m_key, _tick, queued.sender));
  }
  std::ranges::stable_sort(
    batch, [&orderValues](const Queued& _left, const Queued& _right)
    { return std::pair{orderValues.at(_left.sender), _left.sender} < std::pair{orderValues.at(_right.sender), _right.sender}; });

  for (const Queued& queued : batch)
  {
    BitReader reader(queued.message);
    const MessageHeader header = ReadMessageHeader(reader);
    ENGINE_ASSERT(header.type == queued.type);
    const CommandEffect effect = m_entries.at(queued.type).execute(queued.sender, reader, _tick);
    m_outcomes.push_back(CommandOutcome{queued.sender, queued.sequence, _tick, std::nullopt, effect});
  }
}

std::vector<CommandOutcome> CommandQueue::TakeOutcomes()
{
  std::vector<CommandOutcome> outcomes;
  outcomes.swap(m_outcomes);
  return outcomes;
}

void CommandQueue::Reject(SenderId _sender, std::uint64_t _sequence, CommandRejection _reason)
{
  switch (_reason)
  {
  case CommandRejection::UnknownType:
    ++m_rejections.unknownType;
    break;
  case CommandRejection::WrongVersion:
    ++m_rejections.wrongVersion;
    break;
  case CommandRejection::Oversized:
    ++m_rejections.oversized;
    break;
  case CommandRejection::Malformed:
    ++m_rejections.malformed;
    break;
  case CommandRejection::NotPermitted:
    ++m_rejections.notPermitted;
    break;
  case CommandRejection::Duplicate:
    ++m_rejections.duplicate;
    break;
  }
  m_outcomes.push_back(CommandOutcome{_sender, _sequence, m_nextTick, _reason, CommandEffect::NoEffect});
}

} // namespace Neuron
