#include "pch.h"

#include "FxRng.h"

namespace Neuron
{

namespace
{

constexpr std::uint64_t GOLDEN_GAMMA = 0x9E3779B97F4A7C15ull;
constexpr double UNIT_SCALE = 1.0 / 9007199254740991.0; // 1 ÷ (2⁵³ − 1): the top 53 bits reach 1 exactly

// SplitMix64's output function.
std::uint64_t Mix(std::uint64_t _z) noexcept
{
  _z = (_z ^ (_z >> 30)) * 0xBF58476D1CE4E5B9ull;
  _z = (_z ^ (_z >> 27)) * 0x94D049BB133111EBull;
  return _z ^ (_z >> 31);
}

} // namespace

std::uint64_t FxRng::Next() noexcept
{
  m_state += GOLDEN_GAMMA;
  return Mix(m_state);
}

std::uint32_t FxRng::NextUint() noexcept
{
  return static_cast<std::uint32_t>(Next() >> 32);
}

double FxRng::NextUnit(double _range) noexcept
{
  return static_cast<double>(Next() >> 11) * UNIT_SCALE * _range;
}

double FxRng::NextSigned(double _range) noexcept
{
  return (NextUnit(1.0) - 0.5) * _range;
}

std::uint64_t FxSeed(std::uint64_t _tick, std::uint64_t _emitter, std::uint64_t _sequence) noexcept
{
  return Mix(Mix(Mix(_tick + GOLDEN_GAMMA) ^ _emitter) ^ _sequence);
}

} // namespace Neuron
