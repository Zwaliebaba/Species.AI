#include "pch.h"

#include "TickDraws.h"

namespace Species
{

std::size_t TickDraws::KeyHash::operator()(const std::pair<std::uint64_t, std::uint64_t>& _key) const noexcept
{
  // The subject's stable id spreads well on its own; the purpose is a small number.
  return static_cast<std::size_t>(_key.second * 0x9e3779b97f4a7c15ULL + _key.first);
}

void TickDraws::SetTick(std::uint64_t _tick)
{
  if (_tick != m_tick)
  {
    m_contexts.clear();
    m_tick = _tick;
  }
}

Neuron::SimRng& TickDraws::For(DrawPurpose _purpose, StableId _subject)
{
  const std::pair<std::uint64_t, std::uint64_t> key{PurposeOf(_purpose), _subject};
  const auto found = m_contexts.find(key);
  if (found != m_contexts.end())
  {
    return found->second;
  }
#if ENGINE_DEVELOPMENT
  Neuron::DrawLedger* ledger = &m_ledger;
#else
  Neuron::DrawLedger* ledger = nullptr;
#endif
  return m_contexts.try_emplace(key, m_key, key.first, _subject, m_tick, ledger).first->second;
}

} // namespace Species
