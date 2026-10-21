#pragma once

#include "PlatformClock.h"
#include "ReplicatedState.h"
#include "StableId.h"
#include "StateReplica.h"
#include "Vec3.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace Species
{

// ADR-084 item 4: when the next state is late, the replica holds its latest state for up to 250 ms, and then the client
// shows that it waits for the server.
inline constexpr Neuron::Nanoseconds REPLICA_HOLD{250'000'000};

/// Where the client's display time stands between the replica's two latest ticks (ADR-084 item 4): the latest tick's
/// state is drawn one tick period after it arrived, the previous one's as it arrives, and the display time never passes
/// the latest state, so nothing is extrapolated. The integrated host's jitter allowance is 0; M4 measures a remote
/// client's.
class ReplicaTiming
{
public:
  explicit ReplicaTiming(Neuron::Nanoseconds _tickPeriod) noexcept
    : m_tickPeriod(_tickPeriod)
  {
  }

  // The replica's latest tick became _tick at _now.
  void Arrived(std::uint64_t _tick, Neuron::Nanoseconds _now) noexcept;
  // The interpolation fraction at _now, from 0 at the previous tick's state to 1 at the latest's, where it holds.
  [[nodiscard]] double Fraction(Neuron::Nanoseconds _now) const noexcept;
  // The next state is later than the hold allows.
  [[nodiscard]] bool Waiting(Neuron::Nanoseconds _now) const noexcept;
  // The display time at _now in ticks: the previous tick plus the fraction of the way to the latest.
  [[nodiscard]] std::optional<double> DisplayTick(Neuron::Nanoseconds _now, std::optional<std::uint64_t> _previousTick) const noexcept;

  [[nodiscard]] Neuron::Nanoseconds TickPeriod() const noexcept
  {
    return m_tickPeriod;
  }

private:
  Neuron::Nanoseconds m_tickPeriod;
  std::optional<std::uint64_t> m_latestTick;
  Neuron::Nanoseconds m_arrived{0};
};

/// A figure of the replica as a frame draws it: its latest replicated state, and its position, facing and health between
/// its two latest states at the frame's fraction, with its movement over the latest tick (ADR-121's walk bob). A figure
/// that entered at the latest tick is drawn from its first state, with no movement.
struct InterpolatedFigure
{
  StableId id = 0;
  ReplicatedFigure state;
  Neuron::Vec3 position;
  double yawRadians = 0.0;
  double health = 0.0;     // a dead figure's is its corpse fade (R-UNIT-034)
  Neuron::Vec3 tickMotion; // the latest state's position less the previous one's, wu
};

[[nodiscard]] std::vector<InterpolatedFigure> InterpolateFigures(const StateReplica& _replica, double _fraction);

/// An Essence orb of the replica as a frame draws it (RND-082): its latest state, and its position between its two
/// latest states at the frame's fraction.
struct InterpolatedOrb
{
  StableId id = 0;
  ReplicatedOrb state;
  Neuron::Vec3 position;
};

[[nodiscard]] std::vector<InterpolatedOrb> InterpolateOrbs(const StateReplica& _replica, double _fraction);

} // namespace Species
