#include "pch.h"

// The markers stay in Release (ADR-068), and pix3.h turns them on by itself only in Debug and profile builds, so the
// one file that includes it turns them on in both configurations (ADR-068 amendment). pix3.h comes last: with the
// markers on it includes strsafe.h, which renames the C string functions for everything that follows.
#define USE_PIX
#include "MarkerTimings.h"
#include "Profile.h"

#include <pix3.h>

namespace Neuron
{

ProfileScope::ProfileScope(const char* _name) noexcept
{
  PIXBeginEvent(PIX_COLOR_DEFAULT, _name);
  // The time inside the event, so that the event's own cost is not the work's (ADR-110).
  if (MarkerTimingOn())
  {
    m_timedName = _name;
    m_startNanoseconds = MarkerClockNanoseconds();
  }
}

ProfileScope::~ProfileScope()
{
  if (m_timedName != nullptr)
  {
    RecordMarkerTime(m_timedName, MarkerClockNanoseconds() - m_startNanoseconds);
  }
  PIXEndEvent();
}

} // namespace Neuron
