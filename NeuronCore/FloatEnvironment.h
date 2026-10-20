#pragma once

namespace Neuron
{

// The default floating-point environment of every thread (PLT-087): round to nearest with ties to even, flush-to-zero
// and denormals-are-zero off, every exception masked. It is read and set through the CRT's control word, which MSVC
// maps to MXCSR on x64 and to FPCR on ARM64; the sticky status flags, which arithmetic sets, are not part of it.
[[nodiscard]] bool IsDefaultFloatEnvironment() noexcept;
void ApplyDefaultFloatEnvironment() noexcept;

} // namespace Neuron
