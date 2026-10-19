#pragma once

#include <cstdint>

namespace Neuron
{

// The default floating-point environment of every thread (PLT-087): round to nearest with ties to even, flush-to-zero
// and denormals-are-zero off, every exception masked. As an SSE control word that is MXCSR 0x1F80, without the six
// sticky status flags, which arithmetic sets and which are not part of the environment.
inline constexpr std::uint32_t DEFAULT_MXCSR = 0x1F80;
inline constexpr std::uint32_t MXCSR_STATUS_FLAGS = 0x3F;

[[nodiscard]] bool IsDefaultFloatEnvironment() noexcept;
void ApplyDefaultFloatEnvironment() noexcept;

} // namespace Neuron
