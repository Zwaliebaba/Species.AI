#include "pch.h"

#include <bcrypt.h>

#include "RunId.h"

#include <format>
#include <stdexcept>

#pragma comment(lib, "bcrypt.lib")

namespace Neuron
{

std::uint64_t DrawRunId()
{
  std::uint64_t runId = 0;
  const NTSTATUS status =
    BCryptGenRandom(nullptr, reinterpret_cast<PUCHAR>(&runId), static_cast<ULONG>(sizeof(runId)), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
  if (status < 0)
  {
    throw std::runtime_error(std::format("BCryptGenRandom failed with status {:#010x}", static_cast<std::uint32_t>(status)));
  }
  return runId;
}

std::string FormatRunId(std::uint64_t _runId)
{
  return std::format("{:016x}", _runId);
}

} // namespace Neuron
