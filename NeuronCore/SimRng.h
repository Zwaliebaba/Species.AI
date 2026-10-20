#pragma once

#include "SipHash.h"

#include <cstdint>
#include <set>
#include <tuple>

namespace Neuron
{

// ADR-082 item 2: the domain of the simulation's draws; world generation is domain 2 and the sender order domain 3.
inline constexpr std::uint64_t SIMULATION_DRAW_DOMAIN = 1;

/// The draws one owner has made contexts for in its current tick (ADR-082 item 3): a second context for the same
/// purpose, subject and tick is a defect, reported by a development check. Development builds only; the owner keeps one
/// and its step uses it from one thread.
class DrawLedger
{
public:
  void BeginTick(std::uint64_t _tick);
  // False when this purpose, subject and tick already had a context.
  [[nodiscard]] bool Claim(std::uint64_t _purpose, std::uint64_t _subject, std::uint64_t _tick);

private:
  std::uint64_t m_tick = 0;
  std::set<std::tuple<std::uint64_t, std::uint64_t>> m_claimed;
};

/// The context a simulation draw comes from (ADR-082 item 3): one subject, one purpose and one tick. Each value is
/// SipHash-2-4 of the world's simulation key over the domain, the purpose, the subject's stable id, the tick and the
/// ordinal, the number of earlier draws of this context, so no draw depends on any other subject's or purpose's draws.
/// A context is never shared between subjects or threads. One draw per statement, never in a constructor, an argument
/// list or a loop condition (R-SIM-621, R-SIM-626); no raw value is ever replicated or shown.
class SimRng
{
public:
  // _purpose is the game's enumerator of the draw site family; _subject a stable id, never 0. _ledger, in development
  // builds, is the owner's ledger of the tick's contexts.
  SimRng(const SipHashKey& _key, std::uint64_t _purpose, std::uint64_t _subject, std::uint64_t _tick, DrawLedger* _ledger = nullptr);

  // The next 64-bit value.
  [[nodiscard]] std::uint64_t NextValue() noexcept;
  // The top 32 bits of the next value.
  [[nodiscard]] std::uint32_t NextUint() noexcept;
  // In [0, _range): (value >> 11) × 2⁻⁵³ × _range.
  [[nodiscard]] double NextUnit(double _range) noexcept;
  // In [-_range / 2, _range / 2): (NextUnit(1) - 0.5) × _range.
  [[nodiscard]] double NextSigned(double _range) noexcept;
  // In [0, _count) for a count of at most 2³²: ((value >> 32) × _count) >> 32.
  [[nodiscard]] std::uint32_t NextBelow(std::uint64_t _count) noexcept;

  [[nodiscard]] std::uint64_t Ordinal() const noexcept
  {
    return m_ordinal;
  }

private:
  SipHashKey m_key;
  std::uint64_t m_purpose;
  std::uint64_t m_subject;
  std::uint64_t m_tick;
  std::uint64_t m_ordinal = 0;
};

} // namespace Neuron
