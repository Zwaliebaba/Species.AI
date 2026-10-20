#include "pch.h"

#include "Sha256.h"

#include <bit>
#include <format>

namespace Neuron
{

namespace
{

constexpr std::array<std::uint32_t, 8> INITIAL_STATE{0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                                                     0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

constexpr std::array<std::uint32_t, 64> ROUND_CONSTANTS{
  0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be,
  0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa,
  0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85,
  0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
  0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f,
  0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

std::uint32_t BigEndianWord(const std::byte* _bytes) noexcept
{
  return (static_cast<std::uint32_t>(_bytes[0]) << 24) | (static_cast<std::uint32_t>(_bytes[1]) << 16) |
         (static_cast<std::uint32_t>(_bytes[2]) << 8) | static_cast<std::uint32_t>(_bytes[3]);
}

} // namespace

Sha256::Sha256() noexcept
  : m_state(INITIAL_STATE)
{
}

void Sha256::Compress(const std::byte* _block) noexcept
{
  std::array<std::uint32_t, 64> schedule{};
  for (std::size_t t = 0; t < 16; ++t)
  {
    schedule[t] = BigEndianWord(_block + 4 * t);
  }
  for (std::size_t t = 16; t < 64; ++t)
  {
    const std::uint32_t s0 = std::rotr(schedule[t - 15], 7) ^ std::rotr(schedule[t - 15], 18) ^ (schedule[t - 15] >> 3);
    const std::uint32_t s1 = std::rotr(schedule[t - 2], 17) ^ std::rotr(schedule[t - 2], 19) ^ (schedule[t - 2] >> 10);
    schedule[t] = schedule[t - 16] + s0 + schedule[t - 7] + s1;
  }
  std::uint32_t a = m_state[0];
  std::uint32_t b = m_state[1];
  std::uint32_t c = m_state[2];
  std::uint32_t d = m_state[3];
  std::uint32_t e = m_state[4];
  std::uint32_t f = m_state[5];
  std::uint32_t g = m_state[6];
  std::uint32_t h = m_state[7];
  for (std::size_t t = 0; t < 64; ++t)
  {
    const std::uint32_t sum1 = std::rotr(e, 6) ^ std::rotr(e, 11) ^ std::rotr(e, 25);
    const std::uint32_t choose = (e & f) ^ (~e & g);
    const std::uint32_t first = h + sum1 + choose + ROUND_CONSTANTS[t] + schedule[t];
    const std::uint32_t sum0 = std::rotr(a, 2) ^ std::rotr(a, 13) ^ std::rotr(a, 22);
    const std::uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
    const std::uint32_t second = sum0 + majority;
    h = g;
    g = f;
    f = e;
    e = d + first;
    d = c;
    c = b;
    b = a;
    a = first + second;
  }
  m_state[0] += a;
  m_state[1] += b;
  m_state[2] += c;
  m_state[3] += d;
  m_state[4] += e;
  m_state[5] += f;
  m_state[6] += g;
  m_state[7] += h;
}

void Sha256::Update(std::span<const std::byte> _bytes) noexcept
{
  m_totalBytes += _bytes.size();
  for (const std::byte value : _bytes)
  {
    m_block[m_blockBytes++] = value;
    if (m_blockBytes == m_block.size())
    {
      Compress(m_block.data());
      m_blockBytes = 0;
    }
  }
}

Sha256Digest Sha256::Finish() noexcept
{
  // FIPS 180-4 §5.1.1: a one bit, zeros to 56 bytes of the block, then the message length in bits, big-endian.
  const std::uint64_t bits = m_totalBytes * 8;
  m_block[m_blockBytes++] = std::byte{0x80};
  if (m_blockBytes > 56)
  {
    while (m_blockBytes < 64)
    {
      m_block[m_blockBytes++] = std::byte{0};
    }
    Compress(m_block.data());
    m_blockBytes = 0;
  }
  while (m_blockBytes < 56)
  {
    m_block[m_blockBytes++] = std::byte{0};
  }
  for (std::size_t n = 0; n < 8; ++n)
  {
    m_block[56 + n] = static_cast<std::byte>(bits >> (56 - 8 * n));
  }
  Compress(m_block.data());
  Sha256Digest digest{};
  for (std::size_t word = 0; word < 8; ++word)
  {
    for (std::size_t n = 0; n < 4; ++n)
    {
      digest[4 * word + n] = static_cast<std::byte>(m_state[word] >> (24 - 8 * n));
    }
  }
  *this = Sha256();
  return digest;
}

Sha256Digest Sha256Of(std::span<const std::byte> _bytes) noexcept
{
  Sha256 hasher;
  hasher.Update(_bytes);
  return hasher.Finish();
}

std::string ShortHex(const Sha256Digest& _digest)
{
  std::string text;
  for (std::size_t n = 0; n < 8; ++n)
  {
    text += std::format("{:02x}", static_cast<unsigned>(_digest[n]));
  }
  return text;
}

} // namespace Neuron
