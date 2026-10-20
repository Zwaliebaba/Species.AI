#include "pch.h"

#include "SipHash.h"

#include <bit>

namespace Neuron
{

namespace
{

struct SipState
{
  std::uint64_t v0;
  std::uint64_t v1;
  std::uint64_t v2;
  std::uint64_t v3;

  explicit SipState(const SipHashKey& _key) noexcept
    : v0(_key.low ^ 0x736f6d6570736575),
      v1(_key.high ^ 0x646f72616e646f6d),
      v2(_key.low ^ 0x6c7967656e657261),
      v3(_key.high ^ 0x7465646279746573)
  {
  }

  void Round() noexcept
  {
    v0 += v1;
    v1 = std::rotl(v1, 13);
    v1 ^= v0;
    v0 = std::rotl(v0, 32);
    v2 += v3;
    v3 = std::rotl(v3, 16);
    v3 ^= v2;
    v0 += v3;
    v3 = std::rotl(v3, 21);
    v3 ^= v0;
    v2 += v1;
    v1 = std::rotl(v1, 17);
    v1 ^= v2;
    v2 = std::rotl(v2, 32);
  }

  // Two compression rounds per 8-byte block (the "2" of SipHash-2-4).
  void Absorb(std::uint64_t _block) noexcept
  {
    v3 ^= _block;
    Round();
    Round();
    v0 ^= _block;
  }

  // Four finalisation rounds (the "4").
  [[nodiscard]] std::uint64_t Finish() noexcept
  {
    v2 ^= 0xff;
    Round();
    Round();
    Round();
    Round();
    return v0 ^ v1 ^ v2 ^ v3;
  }
};

std::uint64_t ReadLittleEndian(std::span<const std::byte> _bytes) noexcept
{
  std::uint64_t value = 0;
  for (std::size_t i = 0; i < _bytes.size(); ++i)
  {
    value |= std::to_integer<std::uint64_t>(_bytes[i]) << (8 * i);
  }
  return value;
}

} // namespace

std::uint64_t SipHash24(const SipHashKey& _key, std::span<const std::byte> _message) noexcept
{
  SipState state(_key);
  const std::size_t whole = _message.size() - _message.size() % 8;
  for (std::size_t offset = 0; offset < whole; offset += 8)
  {
    state.Absorb(ReadLittleEndian(_message.subspan(offset, 8)));
  }
  // The last block: the remaining bytes, and the message length modulo 256 in the top byte.
  const std::uint64_t last = ReadLittleEndian(_message.subspan(whole)) | (static_cast<std::uint64_t>(_message.size() & 0xff) << 56);
  state.Absorb(last);
  return state.Finish();
}

std::uint64_t SipHash24Words(const SipHashKey& _key, std::initializer_list<std::uint64_t> _words) noexcept
{
  SipState state(_key);
  for (const std::uint64_t word : _words)
  {
    state.Absorb(word);
  }
  // Whole words leave no remainder: the last block carries the byte length alone.
  state.Absorb(static_cast<std::uint64_t>((_words.size() * 8) & 0xff) << 56);
  return state.Finish();
}

} // namespace Neuron
