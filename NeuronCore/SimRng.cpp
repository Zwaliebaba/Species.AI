#include "pch.h"

#include "Check.h"
#include "SimRng.h"

namespace Neuron
{

void DrawLedger::BeginTick(std::uint64_t _tick)
{
  m_tick = _tick;
  m_claimed.clear();
}

bool DrawLedger::Claim(std::uint64_t _purpose, std::uint64_t _subject, std::uint64_t _tick)
{
  if (_tick != m_tick)
  {
    BeginTick(_tick);
  }
  return m_claimed.emplace(_purpose, _subject).second;
}

SimRng::SimRng(const SipHashKey& _key, std::uint64_t _purpose, std::uint64_t _subject, std::uint64_t _tick, DrawLedger* _ledger)
  : m_key(_key),
    m_purpose(_purpose),
    m_subject(_subject),
    m_tick(_tick)
{
  ENGINE_ASSERT(_subject != 0);
#if ENGINE_DEVELOPMENT
  if (_ledger != nullptr)
  {
    ENGINE_ASSERT(_ledger->Claim(_purpose, _subject, _tick));
  }
#else
  static_cast<void>(_ledger);
#endif
}

std::uint64_t SimRng::NextValue() noexcept
{
  return SipHash24Words(m_key, {SIMULATION_DRAW_DOMAIN, m_purpose, m_subject, m_tick, m_ordinal++});
}

std::uint32_t SimRng::NextUint() noexcept
{
  return static_cast<std::uint32_t>(NextValue() >> 32);
}

double SimRng::NextUnit(double _range) noexcept
{
  return static_cast<double>(NextValue() >> 11) * 0x1.0p-53 * _range;
}

double SimRng::NextSigned(double _range) noexcept
{
  return (NextUnit(1.0) - 0.5) * _range;
}

std::uint32_t SimRng::NextBelow(std::uint64_t _count) noexcept
{
  ENGINE_ASSERT(_count > 0 && _count <= (std::uint64_t{1} << 32));
  return static_cast<std::uint32_t>(((NextValue() >> 32) * _count) >> 32);
}

} // namespace Neuron
