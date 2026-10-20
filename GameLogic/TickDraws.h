#pragma once

#include "Development.h"
#include "DrawPurpose.h"
#include "SimRng.h"
#include "StableId.h"

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <utility>

namespace Species
{

/// The draw contexts of one owner's tick (ADR-082 item 3, ADR-103): one context per purpose and subject, made at its
/// first use in the tick and continued by every later use, so a rule that draws for the same figure and purpose twice
/// in a tick (two orders, two pushes) takes the next values rather than the same ones. A new tick forgets the contexts
/// of the last. The owner's step uses it from one thread.
class TickDraws
{
public:
  explicit TickDraws(const Neuron::SipHashKey& _key)
    : m_key(_key)
  {
  }

  // The tick whose contexts For gives from now on; a tick other than the current one forgets every context.
  void SetTick(std::uint64_t _tick);
  [[nodiscard]] std::uint64_t Tick() const noexcept
  {
    return m_tick;
  }

  // The context of _purpose for _subject in this tick. The map is never iterated, so its order reaches no outcome.
  [[nodiscard]] Neuron::SimRng& For(DrawPurpose _purpose, StableId _subject);

private:
  struct KeyHash
  {
    [[nodiscard]] std::size_t operator()(const std::pair<std::uint64_t, std::uint64_t>& _key) const noexcept;
  };

  Neuron::SipHashKey m_key;
  std::uint64_t m_tick = 0;
  std::unordered_map<std::pair<std::uint64_t, std::uint64_t>, Neuron::SimRng, KeyHash> m_contexts;
#if ENGINE_DEVELOPMENT
  Neuron::DrawLedger m_ledger;
#endif
};

} // namespace Species
