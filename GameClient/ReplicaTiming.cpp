#include "pch.h"

#include "ReplicaTiming.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <variant>

namespace Species
{

void ReplicaTiming::Arrived(std::uint64_t _tick, Neuron::Nanoseconds _now) noexcept
{
  if (!m_latestTick || _tick > *m_latestTick)
  {
    m_latestTick = _tick;
    m_arrived = _now;
  }
}

double ReplicaTiming::Fraction(Neuron::Nanoseconds _now) const noexcept
{
  if (!m_latestTick || m_tickPeriod.count() <= 0)
  {
    return 1.0;
  }
  const double elapsed = static_cast<double>((_now - m_arrived).count());
  return std::clamp(elapsed / static_cast<double>(m_tickPeriod.count()), 0.0, 1.0);
}

bool ReplicaTiming::Waiting(Neuron::Nanoseconds _now) const noexcept
{
  return m_latestTick && _now - m_arrived > m_tickPeriod + REPLICA_HOLD;
}

std::optional<double> ReplicaTiming::DisplayTick(Neuron::Nanoseconds _now, std::optional<std::uint64_t> _previousTick) const noexcept
{
  if (!m_latestTick)
  {
    return std::nullopt;
  }
  const auto previous = static_cast<double>(_previousTick.value_or(*m_latestTick));
  return previous + (static_cast<double>(*m_latestTick) - previous) * Fraction(_now);
}

std::vector<InterpolatedFigure> InterpolateFigures(const StateReplica& _replica, double _fraction)
{
  const double f = std::clamp(_fraction, 0.0, 1.0);
  std::vector<InterpolatedFigure> figures;
  const StateReplica::View& previous = _replica.Previous();
  auto before = previous.begin();
  for (const auto& [id, state] : _replica.Latest())
  {
    const auto* figure = std::get_if<ReplicatedFigure>(&state);
    if (figure == nullptr)
    {
      continue;
    }
    InterpolatedFigure& drawn = figures.emplace_back(
      InterpolatedFigure{id, *figure, PositionOf(figure->position), YawRadians(figure->yaw), static_cast<double>(figure->health), {}});
    // Both views are in id order, so one walk finds each figure's previous state.
    while (before != previous.end() && before->first < id)
    {
      ++before;
    }
    const auto* was = before != previous.end() && before->first == id ? std::get_if<ReplicatedFigure>(&before->second) : nullptr;
    if (was == nullptr)
    {
      continue;
    }
    const Neuron::Vec3 from = PositionOf(was->position);
    drawn.tickMotion = drawn.position - from;
    drawn.position = {from.x + (drawn.position.x - from.x) * f, from.y + (drawn.position.y - from.y) * f,
                      from.z + (drawn.position.z - from.z) * f};
    drawn.health = static_cast<double>(was->health) + (drawn.health - static_cast<double>(was->health)) * f;
    // The shorter way round.
    const double start = YawRadians(was->yaw);
    double turn = drawn.yawRadians - start;
    if (turn > std::numbers::pi)
    {
      turn -= 2.0 * std::numbers::pi;
    }
    else if (turn < -std::numbers::pi)
    {
      turn += 2.0 * std::numbers::pi;
    }
    drawn.yawRadians = start + turn * f;
  }
  return figures;
}

std::vector<InterpolatedOrb> InterpolateOrbs(const StateReplica& _replica, double _fraction)
{
  const double f = std::clamp(_fraction, 0.0, 1.0);
  std::vector<InterpolatedOrb> orbs;
  const StateReplica::View& previous = _replica.Previous();
  auto before = previous.begin();
  for (const auto& [id, state] : _replica.Latest())
  {
    const auto* orb = std::get_if<ReplicatedOrb>(&state);
    if (orb == nullptr)
    {
      continue;
    }
    InterpolatedOrb& drawn = orbs.emplace_back(InterpolatedOrb{id, *orb, PositionOf(orb->position)});
    while (before != previous.end() && before->first < id)
    {
      ++before;
    }
    const auto* was = before != previous.end() && before->first == id ? std::get_if<ReplicatedOrb>(&before->second) : nullptr;
    if (was != nullptr)
    {
      const Neuron::Vec3 from = PositionOf(was->position);
      drawn.position = from + (drawn.position - from) * f;
    }
  }
  return orbs;
}

std::vector<InterpolatedStructure> InterpolateStructures(const StateReplica& _replica, double _fraction)
{
  const double f = std::clamp(_fraction, 0.0, 1.0);
  std::vector<InterpolatedStructure> structures;
  const StateReplica::View& previous = _replica.Previous();
  auto before = previous.begin();
  for (const auto& [id, state] : _replica.Latest())
  {
    const auto* structure = std::get_if<ReplicatedStructure>(&state);
    if (structure == nullptr)
    {
      continue;
    }
    InterpolatedStructure& drawn = structures.emplace_back(InterpolatedStructure{
      id, *structure, YawRadians(structure->aimYaw), static_cast<double>(structure->aimPitch) * 2.0 * std::numbers::pi / YAW_STEPS});
    while (before != previous.end() && before->first < id)
    {
      ++before;
    }
    const auto* was = before != previous.end() && before->first == id ? std::get_if<ReplicatedStructure>(&before->second) : nullptr;
    if (was != nullptr)
    {
      // The shorter way round, as a figure's yaw (ADR-111).
      const double from = YawRadians(was->aimYaw);
      const double turn = std::remainder(drawn.aimYawRadians - from, 2.0 * std::numbers::pi);
      drawn.aimYawRadians = from + turn * f;
      const double pitchFrom = static_cast<double>(was->aimPitch) * 2.0 * std::numbers::pi / YAW_STEPS;
      drawn.aimPitchRadians = pitchFrom + (drawn.aimPitchRadians - pitchFrom) * f;
    }
  }
  return structures;
}

std::vector<InterpolatedEffect> InterpolateEffects(const StateReplica& _replica, double _fraction)
{
  const double f = std::clamp(_fraction, 0.0, 1.0);
  std::vector<InterpolatedEffect> effects;
  const StateReplica::View& previous = _replica.Previous();
  auto before = previous.begin();
  for (const auto& [id, state] : _replica.Latest())
  {
    const auto* effect = std::get_if<ReplicatedEffect>(&state);
    if (effect == nullptr)
    {
      continue;
    }
    InterpolatedEffect& drawn = effects.emplace_back(InterpolatedEffect{id, *effect, PositionOf(effect->position), {}, true});
    while (before != previous.end() && before->first < id)
    {
      ++before;
    }
    const auto* was = before != previous.end() && before->first == id ? std::get_if<ReplicatedEffect>(&before->second) : nullptr;
    if (was != nullptr)
    {
      const Neuron::Vec3 from = PositionOf(was->position);
      drawn.tickMotion = drawn.position - from;
      drawn.position = from + drawn.tickMotion * f;
      drawn.entered = false;
    }
  }
  return effects;
}

} // namespace Species
