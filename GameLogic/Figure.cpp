#include "pch.h"

#include "DrawPurpose.h"
#include "Figure.h"

#include <algorithm>
#include <bit>
#include <cmath>

namespace Species
{

namespace
{

void WriteVec3(Neuron::BitWriter& _writer, const Neuron::Vec3& _value)
{
  _writer.WriteBits(std::bit_cast<std::uint64_t>(_value.x), 64);
  _writer.WriteBits(std::bit_cast<std::uint64_t>(_value.y), 64);
  _writer.WriteBits(std::bit_cast<std::uint64_t>(_value.z), 64);
}

// R-SIM-107: no simulation field holds NaN or an infinity, so a value that does is no state.
bool ReadDouble(Neuron::BitReader& _reader, double& _value)
{
  _value = std::bit_cast<double>(_reader.ReadBits(64));
  return std::isfinite(_value);
}

bool ReadVec3(Neuron::BitReader& _reader, Neuron::Vec3& _value)
{
  const bool x = ReadDouble(_reader, _value.x);
  const bool y = ReadDouble(_reader, _value.y);
  const bool z = ReadDouble(_reader, _value.z);
  return x && y && z;
}

Neuron::Vec3 DefaultFront(const TuningMvp& _tuning) noexcept
{
  const auto& front = _tuning.ref.sim.spawn.defaultFront;
  return {front.x, front.y, front.z};
}

} // namespace

void SetKind(Figure& _figure, FigureKind _kind, const TuningMvp& _tuning, const DrawScope& _draws)
{
  const auto& unit = _tuning.ref.unit;
  std::int64_t health = 0;
  double rate = 0.0;
  if (_kind == FigureKind::Worker)
  {
    health = unit.worker.health;
    _figure.speed = unit.worker.speed;
    rate = unit.worker.rate;
  }
  else
  {
    health = unit.leader.health;
    _figure.speed = unit.leader.speed;
    rate = unit.leader.rate;
  }
  _figure.kind = _kind;
  _figure.health = static_cast<std::int32_t>(std::min(health, unit.healthMax));
  const double tickPeriod = _tuning.ref.sim.tickPeriod;
  _figure.reloadIntervalTicks = static_cast<std::int32_t>(std::llround(rate / tickPeriod));
  Neuron::SimRng draw(*_draws.key, PurposeOf(DrawPurpose::ReloadTimer), _figure.id, _draws.tick, _draws.ledger);
  const double seconds = draw.NextUnit(rate);
  _figure.reloadTimerTicks = static_cast<std::int32_t>(std::ceil(seconds / tickPeriod));
}

Neuron::Vec3 SpawnFront(const Figure& _figure, const Neuron::Vec3& _velocity, const TuningMvp& _tuning, const DrawScope& _draws)
{
  const Neuron::Vec3 fallback = DefaultFront(_tuning);
  if (Neuron::LengthSquared(_velocity) > 0.0)
  {
    return Neuron::NormalizedOr(_velocity, fallback);
  }
  if (_figure.kind != FigureKind::Worker)
  {
    return fallback;
  }
  Neuron::SimRng draw(*_draws.key, PurposeOf(DrawPurpose::SpawnFront), _figure.id, _draws.tick, _draws.ledger);
  const double x = draw.NextSigned(1.0);
  const double z = draw.NextSigned(1.0);
  return Neuron::NormalizedOr({x, 0.0, z}, fallback);
}

HealthChange ApplyHealthChange(Figure& _figure, std::int32_t _amount, DamageKind _kind, const TuningMvp& _tuning)
{
  const auto& unit = _tuning.ref.unit;
  std::int32_t amount = _amount;
  // R-UNIT-116 step 3; the Shield, the Transport, Fury and the Visitor faction of steps 1, 2 and 4 are not built.
  if (_figure.kind == FigureKind::Worker && _figure.inFormation && _kind == DamageKind::Explosive)
  {
    amount = static_cast<std::int32_t>(static_cast<double>(amount) * unit.worker.formationExplosiveFactor);
  }
  HealthChange change{true, false};
  if (_figure.dead)
  {
    return change;
  }
  const std::int64_t result = std::int64_t{_figure.health} + amount;
  if (result <= 0)
  {
    _figure.health = static_cast<std::int32_t>(unit.corpse.fadeStart);
    _figure.dead = true;
    change.killed = true;
  }
  else if (result > unit.healthMax)
  {
    _figure.health = static_cast<std::int32_t>(unit.healthMax);
  }
  else
  {
    _figure.health = static_cast<std::int32_t>(result);
  }
  return change;
}

bool BaseAdvance(Figure& _figure, const TuningMvp& _tuning)
{
  if (!_figure.enabled)
  {
    return false;
  }
  if (_figure.destroyed)
  {
    return true;
  }
  if (_figure.dead)
  {
    // R-UNIT-034: the corpse fades by its rate on the ground or in the air and goes on the tick it reaches 0.
    const auto& corpse = _tuning.ref.unit.corpse;
    _figure.health -= static_cast<std::int32_t>(_figure.onGround ? corpse.fadeGroundPerTick : corpse.fadeAirPerTick);
    if (_figure.health <= 0)
    {
      return true;
    }
  }
  if (_figure.reloadTimerTicks > 0)
  {
    --_figure.reloadTimerTicks;
  }
  return false;
}

void WriteFigure(Neuron::BitWriter& _writer, const Figure& _figure)
{
  _writer.WriteBits(_figure.id, 64);
  _writer.WriteBounded(static_cast<std::uint64_t>(_figure.kind), FIGURE_KIND_MAX);
  WriteVec3(_writer, _figure.position);
  WriteVec3(_writer, _figure.velocity);
  WriteVec3(_writer, _figure.front);
  WriteVec3(_writer, _figure.roamCenter);
  _writer.WriteBits(std::bit_cast<std::uint64_t>(_figure.roamRange), 64);
  _writer.WriteBits(std::bit_cast<std::uint64_t>(_figure.speed), 64);
  _writer.WriteSigned(_figure.health, 32);
  _writer.WriteSigned(_figure.reloadIntervalTicks, 32);
  _writer.WriteSigned(_figure.reloadTimerTicks, 32);
  _writer.WriteBool(_figure.onGround);
  _writer.WriteBool(_figure.enabled);
  _writer.WriteBool(_figure.destroyed);
  _writer.WriteBool(_figure.dead);
  _writer.WriteBool(_figure.inFormation);
}

bool ReadFigure(Neuron::BitReader& _reader, Figure& _figure)
{
  _figure.id = _reader.ReadBits(64);
  _figure.kind = static_cast<FigureKind>(_reader.ReadBounded(FIGURE_KIND_MAX));
  bool finite = ReadVec3(_reader, _figure.position);
  finite = ReadVec3(_reader, _figure.velocity) && finite;
  finite = ReadVec3(_reader, _figure.front) && finite;
  finite = ReadVec3(_reader, _figure.roamCenter) && finite;
  finite = ReadDouble(_reader, _figure.roamRange) && finite;
  finite = ReadDouble(_reader, _figure.speed) && finite;
  _figure.health = static_cast<std::int32_t>(_reader.ReadSigned(32));
  _figure.reloadIntervalTicks = static_cast<std::int32_t>(_reader.ReadSigned(32));
  _figure.reloadTimerTicks = static_cast<std::int32_t>(_reader.ReadSigned(32));
  _figure.onGround = _reader.ReadBool();
  _figure.enabled = _reader.ReadBool();
  _figure.destroyed = _reader.ReadBool();
  _figure.dead = _reader.ReadBool();
  _figure.inFormation = _reader.ReadBool();
  return !_reader.Failed() && finite && _figure.id != 0;
}

} // namespace Species
