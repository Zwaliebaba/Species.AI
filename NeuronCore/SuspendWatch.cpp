#include "pch.h"

#include <powerbase.h>
#include <powrprof.h>

#include "SuspendWatch.h"

#include <memory>

#pragma comment(lib, "powrprof.lib")

namespace Neuron
{

namespace
{

// DEVICE_NOTIFY_CALLBACK's routine: the system's suspend and resume events for this process.
ULONG CALLBACK OnPowerEvent(PVOID _context, ULONG _type, PVOID /*_setting*/)
{
  static_cast<SuspendWatch*>(_context)->Notify(_type);
  return ERROR_SUCCESS;
}

} // namespace

// The system reads the routine and its context from these while the registration lasts.
struct SuspendWatch::Subscription
{
  DEVICE_NOTIFY_SUBSCRIBE_PARAMETERS parameters{};
};

SuspendWatch::SuspendWatch()
  : m_subscription(std::make_unique<Subscription>())
{
  m_subscription->parameters.Callback = &OnPowerEvent;
  m_subscription->parameters.Context = this;
  HPOWERNOTIFY registration = nullptr;
  if (PowerRegisterSuspendResumeNotification(DEVICE_NOTIFY_CALLBACK, &m_subscription->parameters, &registration) == ERROR_SUCCESS)
  {
    m_registration = registration;
  }
}

SuspendWatch::~SuspendWatch()
{
  if (m_registration != nullptr)
  {
    static_cast<void>(PowerUnregisterSuspendResumeNotification(static_cast<HPOWERNOTIFY>(m_registration)));
  }
}

void SuspendWatch::Notify(std::uint32_t _event) noexcept
{
  if (_event == PBT_APMSUSPEND)
  {
    m_suspensions.fetch_add(1, std::memory_order_acq_rel);
  }
}

} // namespace Neuron
