#pragma once

#include <cstdint>

namespace Neuron
{

// Why a platform service failed (platform layer §2.1).
enum class PlatformError : std::uint8_t
{
  Unsupported,
  AccessDenied,
  NotFound,
  Other
};

} // namespace Neuron
