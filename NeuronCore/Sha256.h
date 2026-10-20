#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

namespace Neuron
{

// A SHA-256 digest (FIPS 180-4): the content identity of terrain and, later, of packs (ADR-060, TER-252).
using Sha256Digest = std::array<std::byte, 32>;

/// SHA-256 over bytes given in any number of parts. Plain C++ and integer arithmetic only, so every build and every
/// CPU computes the same digest (R14: no library).
class Sha256
{
public:
  Sha256() noexcept;

  void Update(std::span<const std::byte> _bytes) noexcept;
  // The digest of every byte given; the hasher then starts again, empty.
  [[nodiscard]] Sha256Digest Finish() noexcept;

private:
  void Compress(const std::byte* _block) noexcept;

  std::array<std::uint32_t, 8> m_state;
  std::array<std::byte, 64> m_block{};
  std::size_t m_blockBytes = 0;
  std::uint64_t m_totalBytes = 0;
};

[[nodiscard]] Sha256Digest Sha256Of(std::span<const std::byte> _bytes) noexcept;

// TER-252, ADR-060: logs show the first 16 hexadecimal digits; every comparison uses all 32 bytes.
[[nodiscard]] std::string ShortHex(const Sha256Digest& _digest);

} // namespace Neuron
