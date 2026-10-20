#include "pch.h"

#include <float.h>

#include "FloatEnvironment.h"

namespace Neuron
{

namespace
{

// The parts of the control word that make up the environment, and their default values. The denormal-operand
// exception, an x86 extension of IEEE 754, is left out: _controlfp_s does not set its mask, and the CRT maps only the
// five IEEE exceptions on ARM64.
constexpr unsigned int EXCEPTION_MASKS = _MCW_EM & ~_EM_DENORMAL;
constexpr unsigned int ENVIRONMENT_MASK = EXCEPTION_MASKS | _MCW_RC | _MCW_DN;
constexpr unsigned int DEFAULT_ENVIRONMENT = EXCEPTION_MASKS | _RC_NEAR | _DN_SAVE;

} // namespace

bool IsDefaultFloatEnvironment() noexcept
{
  unsigned int control = 0;
  return _controlfp_s(&control, 0, 0) == 0 && (control & ENVIRONMENT_MASK) == DEFAULT_ENVIRONMENT;
}

void ApplyDefaultFloatEnvironment() noexcept
{
  unsigned int control = 0;
  _controlfp_s(&control, DEFAULT_ENVIRONMENT, ENVIRONMENT_MASK);
  _clearfp();
}

} // namespace Neuron
