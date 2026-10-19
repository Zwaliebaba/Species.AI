#include "pch.h"

// The markers stay in Release (ADR-068), and pix3.h turns them on by itself only in Debug and profile builds, so the
// one file that includes it turns them on in both configurations (ADR-068 amendment). pix3.h comes last: with the
// markers on it includes strsafe.h, which renames the C string functions for everything that follows.
#define USE_PIX
#include "Profile.h"

#include <pix3.h>

namespace Neuron
{

ProfileScope::ProfileScope(const char* _name) noexcept
{
  PIXBeginEvent(PIX_COLOR_DEFAULT, _name);
}

ProfileScope::~ProfileScope()
{
  PIXEndEvent();
}

} // namespace Neuron
