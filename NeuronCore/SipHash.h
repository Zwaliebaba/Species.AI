#pragma once

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <span>

namespace Neuron
{

/// A 128-bit secret key: its 16 bytes read as two little-endian 64-bit words, the first eight bytes in `low`.
struct SipHashKey
{
  std::uint64_t low = 0;
  std::uint64_t high = 0;
};

// SipHash-2-4 (Aumasson and Bernstein, 2012): a keyed function whose outputs do not reveal its key (ADR-082 item 2).
// Plain C++ and integer arithmetic only, so every build and every CPU computes the same value.
[[nodiscard]] std::uint64_t SipHash24(const SipHashKey& _key, std::span<const std::byte> _message) noexcept;

// The message of the keyed values of ADR-082: each word as its eight little-endian bytes, in order.
[[nodiscard]] std::uint64_t SipHash24Words(const SipHashKey& _key, std::initializer_list<std::uint64_t> _words) noexcept;

} // namespace Neuron
