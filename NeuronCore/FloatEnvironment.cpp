#include "pch.h"

#include <immintrin.h>

#include "FloatEnvironment.h"

namespace Neuron
{

bool IsDefaultFloatEnvironment() noexcept
{
  return (_mm_getcsr() & ~MXCSR_STATUS_FLAGS) == DEFAULT_MXCSR;
}

void ApplyDefaultFloatEnvironment() noexcept
{
  _mm_setcsr(DEFAULT_MXCSR);
}

} // namespace Neuron
