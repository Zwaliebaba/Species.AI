#include "pch.h"

#include <bcrypt.h>

#include "SecureRandom.h"

#include <array>
#include <cstring>
#include <format>
#include <stdexcept>

#pragma comment(lib, "bcrypt.lib")

namespace Neuron
{

void FillSecureRandom(std::span<std::byte> _bytes)
{
  const NTSTATUS status =
    BCryptGenRandom(nullptr, reinterpret_cast<PUCHAR>(_bytes.data()), static_cast<ULONG>(_bytes.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
  if (status < 0)
  {
    throw std::runtime_error(std::format("BCryptGenRandom failed with status {:#010x}", static_cast<std::uint32_t>(status)));
  }
}

SipHashKey DrawSecretKey()
{
  std::array<std::byte, 16> bytes{};
  FillSecureRandom(bytes);
  SipHashKey key;
  std::memcpy(&key.low, bytes.data(), sizeof(key.low));
  std::memcpy(&key.high, bytes.data() + sizeof(key.low), sizeof(key.high));
  return key;
}

} // namespace Neuron
