#pragma once

#include <cstdint>

namespace Neuron
{

/// The presentation generator (R-SIM-607, VFX-011, ADR-036): every cosmetic random draw of the client, never the
/// simulation's (DEC-013). SplitMix64, one instance per thread; an effect seeds it from its event's identity, so the same
/// moment replayed on the same client draws the same particles.
class FxRng
{
public:
  FxRng() noexcept = default;
  explicit FxRng(std::uint64_t _seed) noexcept
    : m_state(_seed)
  {
  }

  void Seed(std::uint64_t _seed) noexcept
  {
    m_state = _seed;
  }

  // R-SIM-607's draw forms: a non-negative integer; uniform over [0, _range], both ends included; uniform over
  // [−_range ÷ 2, _range ÷ 2], both ends included.
  [[nodiscard]] std::uint32_t NextUint() noexcept;
  [[nodiscard]] double NextUnit(double _range) noexcept;
  [[nodiscard]] double NextSigned(double _range) noexcept;

private:
  [[nodiscard]] std::uint64_t Next() noexcept;

  std::uint64_t m_state = 0;
};

// ADR-036, VFX-011: a seed from an effect's identity: its tick, its emitter's handle and its sequence within the tick.
[[nodiscard]] std::uint64_t FxSeed(std::uint64_t _tick, std::uint64_t _emitter, std::uint64_t _sequence) noexcept;

} // namespace Neuron
