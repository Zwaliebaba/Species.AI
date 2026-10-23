#include "pch.h"

#include "AudioEngine.h"
#include "AudioMath.h"
#include "SilentAudioDevice.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <thread>
#include <utility>

namespace Neuron
{

namespace
{

constexpr Nanoseconds REBUILD_RETRY{1'000'000'000}; // a lost device is tried again once a second (AEN-260)
constexpr std::int64_t NANOSECONDS_PER_SECOND = 1'000'000'000;

[[nodiscard]] double ToDecibels(double _gain) noexcept
{
  return _gain > 0.0 ? 20.0 * std::log10(_gain) : -std::numeric_limits<double>::infinity();
}

} // namespace

AudioEngine::AudioEngine(Desc _desc)
  : m_settings(_desc.settings),
    m_events(std::move(_desc.events)),
    m_device(_desc.device != nullptr ? std::move(_desc.device) : std::make_unique<SilentAudioDevice>()),
    m_clock(_desc.clock),
    m_commands(_desc.settings.commandCapacity),
    m_instances(_desc.settings.instanceCapacity),
    m_lastMember(m_events.size(), std::numeric_limits<std::uint32_t>::max()),
    m_masterStep(std::min(_desc.settings.masterVolumeStep, _desc.settings.volumeMax)),
    m_rng(_desc.settings.rngSeed)
{
  // AEN-300: every table is made here, so an update allocates nothing.
  m_order.reserve(m_instances.size());
  m_fading.reserve(m_instances.size());
  m_real.reserve(m_instances.size());
  m_perEvent.resize(m_events.size());
  if (_desc.ownThread)
  {
    m_thread = PlatformThread::Start("audio",
                                     [this](const std::stop_token& _stop)
                                     {
                                       Nanoseconds next = m_clock->Now();
                                       while (!_stop.stop_requested())
                                       {
                                         Update();
                                         next += m_settings.updatePeriod;
                                         const Nanoseconds now = m_clock->Now();
                                         if (next > now)
                                         {
                                           std::this_thread::sleep_for(next - now);
                                         }
                                         else
                                         {
                                           next = now; // AEN-121: a late update runs once, with no catching up
                                         }
                                       }
                                       // AEN-106: the device goes on the thread that may have rebuilt it, before that
                                       // thread's COM initialization ends.
                                       m_device.reset();
                                     });
  }
}

AudioEngine::~AudioEngine()
{
  // AEN-106: the update stops first, then the voices go with the device.
  m_thread.RequestStop();
  if (m_thread.Joinable())
  {
    m_thread.Join();
  }
}

bool AudioEngine::Push(const Command& _command)
{
  if (m_commands.Push(_command))
  {
    return true;
  }
  m_commandsDropped.fetch_add(1, std::memory_order_relaxed);
  return false;
}

bool AudioEngine::Trigger(SoundEventId _event, const Vec3& _position)
{
  Command command;
  command.kind = CommandKind::Trigger;
  command.issued = m_clock->Now();
  command.event = _event;
  command.position = _position;
  return Push(command);
}

bool AudioEngine::SetListener(const AudioListener& _listener)
{
  Command command;
  command.kind = CommandKind::Listener;
  command.issued = m_clock->Now();
  command.position = _listener.position;
  command.front = _listener.front;
  command.up = _listener.up;
  return Push(command);
}

bool AudioEngine::SetMasterVolume(unsigned _step)
{
  Command command;
  command.kind = CommandKind::MasterVolume;
  command.issued = m_clock->Now();
  command.step = _step;
  return Push(command);
}

std::uint32_t AudioEngine::DrawMember(SoundEventId _event)
{
  // AEN-180 (DEC-568): every member weighs 1 at the MVP; the one drawn last is excluded when there are two or more, and
  // the first eligible member whose cumulative weight reaches u = NextUnit(W) is taken.
  const auto count = static_cast<std::uint32_t>(m_events[_event].members.size());
  std::uint32_t& last = m_lastMember[_event];
  if (count <= 1)
  {
    last = 0;
    return 0;
  }
  const bool excluded = last < count;
  const double drawn = m_rng.NextUnit(static_cast<double>(excluded ? count - 1 : count));
  std::uint32_t member = 0;
  double cumulative = 0.0;
  for (; member < count; ++member)
  {
    if (excluded && member == last)
    {
      continue;
    }
    cumulative += 1.0;
    if (cumulative >= drawn)
    {
      break;
    }
  }
  last = std::min(member, count - 1);
  return last;
}

void AudioEngine::Apply(const Command& _command)
{
  switch (_command.kind)
  {
  case CommandKind::Listener:
    // AEN-209: a front or up of no length, or the two parallel, keeps the previous orientation.
    m_listener.position = _command.position;
    if (LengthSquared(_command.front) > NORMALIZE_MIN_LENGTH_SQUARED && LengthSquared(_command.up) > NORMALIZE_MIN_LENGTH_SQUARED &&
        std::abs(Dot(NormalizedOr(_command.front, Vec3{}), NormalizedOr(_command.up, Vec3{}))) < 1.0 - 1e-9)
    {
      m_listener.front = _command.front;
      m_listener.up = _command.up;
    }
    break;
  case CommandKind::MasterVolume:
    m_masterStep = std::min(_command.step, m_settings.volumeMax);
    break;
  case CommandKind::Trigger:
  {
    // AEN-111: an unknown event, a silent one and a full table each drop the trigger, counted.
    if (_command.event >= m_events.size() || m_events[_command.event].members.empty())
    {
      ++m_statistics.triggersDropped;
      break;
    }
    const auto free = std::ranges::find(m_instances, InstanceState::Free, &Instance::state);
    if (free == m_instances.end())
    {
      ++m_statistics.triggersDropped;
      break;
    }
    *free = Instance{};
    free->state = InstanceState::Pending;
    free->event = _command.event;
    free->member = DrawMember(_command.event);
    free->position = _command.position;
    free->start = _command.issued + m_settings.scheduleLatency; // AEN-024
    break;
  }
  }
}

std::size_t AudioEngine::FramesPlayed(const Instance& _instance, const SoundAsset& _sound, Nanoseconds _now) noexcept
{
  return static_cast<std::size_t>(std::max<std::int64_t>((_now - _instance.origin).count(), 0) *
                                  static_cast<std::int64_t>(_sound.sampleRate) / NANOSECONDS_PER_SECOND);
}

void AudioEngine::Retire(Instance& _instance)
{
  if (_instance.voice)
  {
    m_device->Stop(*_instance.voice);
  }
  _instance = Instance{};
}

void AudioEngine::Demote(Instance& _instance, Nanoseconds _now)
{
  // AEN-145: the voice fades over the steal fade before it returns to its pool, and the instance goes virtual.
  if (_instance.voice)
  {
    m_device->Set(*_instance.voice, 0.0f, CenteredPan());
    m_fading.push_back({*_instance.voice, _now + m_settings.stealFade});
    _instance.voice.reset();
  }
  _instance.state = InstanceState::Virtual;
  _instance.stateSince = _now;
}

bool AudioEngine::TryStart(Instance& _instance, Nanoseconds _now)
{
  const SoundEventDef& event = m_events[_instance.event];
  const SoundAsset& sound = *event.members[_instance.member];
  // AEN-146: a one-shot never real starts at its first frame; one stolen kept its play position (ADR-023) and resumes
  // where it would be.
  const std::size_t startFrame = _instance.everReal ? FramesPlayed(_instance, sound, _now) : 0;
  if (startFrame >= sound.Frames())
  {
    return false;
  }
  const std::array<float, 2> pan =
    event.positioned ? StereoPan(_instance.position, m_listener.position, m_listener.front, m_listener.up) : CenteredPan();
  const auto amplitude = static_cast<float>(DecibelsToAmplitude(_instance.audibleDb, m_settings.floorDb));
  const std::optional<AudioVoice> voice = m_device->Start(sound, startFrame, event.bus, amplitude, pan);
  if (!voice)
  {
    return false;
  }
  _instance.voice = voice;
  _instance.state = InstanceState::Real;
  _instance.stateSince = _now;
  if (!_instance.everReal)
  {
    _instance.origin = _now;
    _instance.everReal = true;
  }
  return true;
}

void AudioEngine::Evaluate(Nanoseconds _now)
{
  const double premasterDb = VolumeStepDb(m_masterStep, m_settings.volumeMax, m_settings.masterStepDb, m_settings.floorDb);
  m_order.clear();
  for (std::uint32_t slot = 0; slot < m_instances.size(); ++slot)
  {
    Instance& instance = m_instances[slot];
    if (instance.state == InstanceState::Free)
    {
      continue;
    }
    const SoundEventDef& event = m_events[instance.event];
    // AEN-123 step 4: the triggers due by the end of this update start, virtual; an announcer start ducks the world bus.
    if (instance.state == InstanceState::Pending)
    {
      if (instance.start > _now + m_settings.updatePeriod)
      {
        continue;
      }
      instance.state = InstanceState::Virtual;
      instance.stateSince = _now;
      if (event.bus == AudioBus::Announcer)
      {
        m_duckUntil = std::max(m_duckUntil, instance.start + m_settings.duckTime);
      }
    }
    // A one-shot ends at its sample's end (AEN-175, AEN-179); one never real has not advanced (AEN-146).
    const bool ended =
      instance.everReal && FramesPlayed(instance, *event.members[instance.member], _now) >= event.members[instance.member]->Frames();
    if (ended || (instance.voice && m_device->Ended(*instance.voice)))
    {
      Retire(instance);
      continue;
    }
    // AEN-140, AEN-201: the audibility, the level after the distance gain; inaudible under the floor.
    instance.gain = event.positioned ? DistanceGain(Length(instance.position - m_listener.position), event.minDistance,
                                                    m_settings.maxDistance, m_settings.maxDistanceFadeFraction)
                                     : 1.0;
    instance.audibleDb = event.levelDb + ToDecibels(instance.gain);
    instance.audible = instance.audibleDb + m_settings.busLevelsDb[static_cast<std::size_t>(event.bus)] + premasterDb > m_settings.floorDb;
    m_order.push_back(slot);
  }
  // AEN-141: each further copy of an event, loudest first (ties: the earlier start, then the lower slot), scores the
  // copy decay lower per louder copy.
  std::ranges::sort(m_order,
                    [&](std::uint32_t _a, std::uint32_t _b)
                    {
                      const Instance& a = m_instances[_a];
                      const Instance& b = m_instances[_b];
                      if (a.event != b.event)
                      {
                        return a.event < b.event;
                      }
                      if (a.audibleDb != b.audibleDb)
                      {
                        return a.audibleDb > b.audibleDb;
                      }
                      return a.start != b.start ? a.start < b.start : _a < _b;
                    });
  const double decayDb = ToDecibels(m_settings.copyDecay);
  std::uint32_t louder = 0;
  for (std::size_t index = 0; index < m_order.size(); ++index)
  {
    Instance& instance = m_instances[m_order[index]];
    louder = index > 0 && m_instances[m_order[index - 1]].event == instance.event ? louder + 1 : 0;
    instance.scoreDb = instance.audibleDb + static_cast<double>(louder) * decayDb;
  }
  // AEN-144: the bus class, then the score, then the earlier start, then the lower slot.
  std::ranges::sort(m_order,
                    [&](std::uint32_t _a, std::uint32_t _b)
                    {
                      const Instance& a = m_instances[_a];
                      const Instance& b = m_instances[_b];
                      const auto busA = m_events[a.event].bus;
                      const auto busB = m_events[b.event].bus;
                      if (busA != busB)
                      {
                        return busA < busB;
                      }
                      if (a.scoreDb != b.scoreDb)
                      {
                        return a.scoreDb > b.scoreDb;
                      }
                      return a.start != b.start ? a.start < b.start : _a < _b;
                    });
  for (std::uint32_t place = 0; place < m_order.size(); ++place)
  {
    m_instances[m_order[place]].rank = place;
  }
}

void AudioEngine::Allocate(Nanoseconds _now)
{
  std::vector<std::uint32_t>& perEvent = m_perEvent;
  std::ranges::fill(perEvent, 0U);
  std::array<std::uint32_t, AUDIO_BUS_COUNT> perBus{};
  m_real.clear();
  // Keep (AEN-144): a real instance stays real unless it goes inaudible.
  for (const std::uint32_t slot : m_order)
  {
    Instance& instance = m_instances[slot];
    if (instance.state != InstanceState::Real)
    {
      continue;
    }
    if (!instance.audible)
    {
      Demote(instance, _now);
      continue;
    }
    ++perEvent[instance.event];
    ++perBus[static_cast<std::size_t>(m_events[instance.event].bus)];
    m_real.push_back(slot);
  }
  // Fill, in rank order, within the event's cap, the bus's cap and the budget; else steal by the margin (AEN-145).
  for (const std::uint32_t slot : m_order)
  {
    Instance& candidate = m_instances[slot];
    if (candidate.state != InstanceState::Virtual || !candidate.audible || m_device->Lost())
    {
      continue;
    }
    const SoundEventDef& event = m_events[candidate.event];
    const auto bus = static_cast<std::size_t>(event.bus);
    const bool eventFull = perEvent[candidate.event] >= event.voiceCap;
    const bool busFull = perBus[bus] >= m_settings.busCaps[bus];
    const bool budgetFull = m_real.size() >= m_settings.voiceBudget;
    if (!eventFull && !busFull && !budgetFull)
    {
      if (TryStart(candidate, _now))
      {
        ++perEvent[candidate.event];
        ++perBus[bus];
        m_real.push_back(slot);
      }
      continue;
    }
    // The victim: the lowest-ranked real instance in the first full limit.
    std::optional<std::size_t> victimAt;
    for (std::size_t at = 0; at < m_real.size(); ++at)
    {
      const Instance& other = m_instances[m_real[at]];
      const auto otherBus = m_events[other.event].bus;
      const bool inLimit = eventFull ? other.event == candidate.event : busFull ? otherBus == event.bus : otherBus >= event.bus;
      if (inLimit && (!victimAt || other.rank > m_instances[m_real[*victimAt]].rank))
      {
        victimAt = at;
      }
    }
    if (!victimAt)
    {
      continue;
    }
    Instance& victim = m_instances[m_real[*victimAt]];
    const AudioBus victimBus = m_events[victim.event].bus;
    if (victimBus == AudioBus::Ui || (event.bus == AudioBus::World && victimBus == AudioBus::Ambience) ||
        candidate.scoreDb < victim.scoreDb + m_settings.stealMarginDb || _now - candidate.stateSince < m_settings.stateHold ||
        _now - victim.stateSince < m_settings.stateHold)
    {
      continue;
    }
    --perEvent[victim.event];
    --perBus[static_cast<std::size_t>(victimBus)];
    Demote(victim, _now);
    m_real.erase(m_real.begin() + static_cast<std::ptrdiff_t>(*victimAt));
    ++m_statistics.steals;
    if (TryStart(candidate, _now))
    {
      ++perEvent[candidate.event];
      ++perBus[bus];
      m_real.push_back(slot);
    }
  }
  // AEN-146: a one-shot still virtual at its start plus its patience is retired, unheard if it never became real.
  for (Instance& instance : m_instances)
  {
    if (instance.state == InstanceState::Virtual && _now >= instance.start + m_settings.patience)
    {
      if (!instance.everReal)
      {
        ++m_statistics.retiredUnheard;
      }
      Retire(instance);
    }
  }
  m_statistics.real = static_cast<std::uint32_t>(m_real.size());
  m_statistics.realPerBus = perBus;
}

void AudioEngine::Update()
{
  const Nanoseconds now = m_clock->Now();
  // AEN-259: a lost device plays every instance virtual, and is rebuilt once it can be.
  if (m_device->Lost())
  {
    for (Instance& instance : m_instances)
    {
      if (instance.state == InstanceState::Real)
      {
        instance.voice.reset(); // the device's voices are gone with it
        instance.state = InstanceState::Virtual;
        instance.stateSince = now;
      }
    }
    m_fading.clear();
    if (now >= m_nextRebuild)
    {
      m_nextRebuild = now + REBUILD_RETRY;
      static_cast<void>(m_device->Rebuild());
    }
  }
  while (const std::optional<Command> command = m_commands.Pop())
  {
    Apply(*command);
  }
  Evaluate(now);
  Allocate(now);
  // AEN-214: each real voice's volume and pan, recalculated every update (AEN-210).
  for (const std::uint32_t slot : m_real)
  {
    const Instance& instance = m_instances[slot];
    if (!instance.voice)
    {
      continue;
    }
    const SoundEventDef& event = m_events[instance.event];
    m_device->Set(*instance.voice, static_cast<float>(DecibelsToAmplitude(instance.audibleDb, m_settings.floorDb)),
                  event.positioned ? StereoPan(instance.position, m_listener.position, m_listener.front, m_listener.up) : CenteredPan());
  }
  // A faded victim's voice returns to its pool.
  std::erase_if(m_fading,
                [&](const Fading& _fading)
                {
                  if (now < _fading.until)
                  {
                    return false;
                  }
                  m_device->Stop(_fading.voice);
                  return true;
                });
  // AEN-215, AEN-216: the bus gains, the world bus ducked under the announcer, and the master volume's pre-master gain.
  for (std::size_t bus = 0; bus < AUDIO_BUS_COUNT; ++bus)
  {
    const double duck = static_cast<AudioBus>(bus) == AudioBus::World && now < m_duckUntil ? m_settings.duckWorldDb : 0.0;
    m_device->SetBusGain(static_cast<AudioBus>(bus),
                         static_cast<float>(DecibelsToAmplitude(m_settings.busLevelsDb[bus] + duck, m_settings.floorDb)));
  }
  m_device->SetPreMasterGain(static_cast<float>(DecibelsToAmplitude(
    VolumeStepDb(m_masterStep, m_settings.volumeMax, m_settings.masterStepDb, m_settings.floorDb), m_settings.floorDb)));
  m_device->Commit();
  ++m_statistics.updates;
  Publish();
}

void AudioEngine::Publish()
{
  const auto live = static_cast<std::uint32_t>(
    std::ranges::count_if(m_instances, [](const Instance& _instance)
                          { return _instance.state == InstanceState::Virtual || _instance.state == InstanceState::Real; }));
  m_publishedLive.store(live, std::memory_order_relaxed);
  m_publishedReal.store(m_statistics.real, std::memory_order_relaxed);
  for (std::size_t bus = 0; bus < AUDIO_BUS_COUNT; ++bus)
  {
    m_publishedRealPerBus[bus].store(m_statistics.realPerBus[bus], std::memory_order_relaxed);
  }
  m_publishedTriggersDropped.store(m_statistics.triggersDropped, std::memory_order_relaxed);
  m_publishedRetiredUnheard.store(m_statistics.retiredUnheard, std::memory_order_relaxed);
  m_publishedSteals.store(m_statistics.steals, std::memory_order_relaxed);
  m_publishedDeviceLost.store(m_device->Lost(), std::memory_order_relaxed);
  m_publishedUpdates.store(m_statistics.updates, std::memory_order_release);
}

AudioStatistics AudioEngine::Statistics() const noexcept
{
  AudioStatistics statistics;
  statistics.updates = m_publishedUpdates.load(std::memory_order_acquire);
  statistics.live = m_publishedLive.load(std::memory_order_relaxed);
  statistics.real = m_publishedReal.load(std::memory_order_relaxed);
  for (std::size_t bus = 0; bus < AUDIO_BUS_COUNT; ++bus)
  {
    statistics.realPerBus[bus] = m_publishedRealPerBus[bus].load(std::memory_order_relaxed);
  }
  statistics.commandsDropped = m_commandsDropped.load(std::memory_order_relaxed);
  statistics.triggersDropped = m_publishedTriggersDropped.load(std::memory_order_relaxed);
  statistics.retiredUnheard = m_publishedRetiredUnheard.load(std::memory_order_relaxed);
  statistics.steals = m_publishedSteals.load(std::memory_order_relaxed);
  statistics.deviceLost = m_publishedDeviceLost.load(std::memory_order_relaxed);
  return statistics;
}

} // namespace Neuron
