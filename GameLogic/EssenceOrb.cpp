#include "pch.h"

#include "DrawPurpose.h"
#include "EssenceOrb.h"
#include "RegionSimulation.h"
#include "StateValues.h"

#include <algorithm>
#include <cmath>

namespace Species
{

namespace
{

// R-UNIT-321: an orb inside a structure's hit volume is pushed out; once nothing holds it and it is slow, the push stops
// for good. The MVP has no structure until M2, so only a test's shape pushes.
void StructurePush(RegionSimulation& _region, EssenceOrb& _orb)
{
  const auto& orb = _region.Tuning().ref.unit.essenceOrb;
  const ObstacleGrid& obstacles = _region.Obstacles();
  bool hit = false;
  for (const std::uint32_t index : obstacles.StructuresAt(_orb.position.x, _orb.position.z))
  {
    const ObstacleShape& shape = obstacles.Shape(index);
    if (!SphereHits(shape, _orb.position, orb.pushRadius))
    {
      continue;
    }
    hit = true;
    _orb.velocity = _orb.velocity + (_orb.position - shape.position) * orb.pushGain;
    _orb.velocity.y = 0.0;
    if (Neuron::Length(_orb.velocity) > orb.pushSpeedMax)
    {
      _orb.velocity = Neuron::WithLength(_orb.velocity, orb.pushSpeedMax);
    }
  }
  if (!hit && Neuron::Length(_orb.velocity) < orb.pushStopSpeed)
  {
    _orb.push = false;
  }
}

} // namespace

void SpawnOrb(RegionSimulation& _region, const Neuron::Vec3& _position, const Neuron::Vec3& _velocity, std::uint8_t _team, StableId _owner)
{
  const TuningMvp& tuning = _region.Tuning();
  EssenceOrb orb;
  orb.id = _region.IssueId();
  orb.owner = _owner;
  orb.team = _team;
  orb.position = _position + Neuron::Vec3{0.0, tuning.ref.sim.spawn.orbLift, 0.0};
  orb.velocity = _velocity;
  // R-UNIT-307: the start step's four draws, the phase and then the x, y and z axis rates.
  orb.state = OrbState::Birth;
  orb.timerTicks = static_cast<std::int32_t>(tuning.ref.econ.orb.birthTicks);
  Neuron::SimRng& draw = _region.Draws().For(DrawPurpose::OrbStart, orb.id);
  orb.phase = draw.NextUnit(tuning.ref.unit.essenceOrb.wobblePhaseRange);
  orb.rates.x = draw.NextUnit(tuning.ref.econ.orb.wobbleRateRange);
  orb.rates.y = draw.NextUnit(tuning.ref.econ.orb.wobbleRateRange);
  orb.rates.z = draw.NextUnit(tuning.ref.econ.orb.wobbleRateRange);
  orb.groundHeight = _region.Terrain()->field.Height(orb.position.x, orb.position.z);
  orb.push = true;
  static_cast<void>(_region.Orbs().Take(orb));
}

bool AdvanceOrb(RegionSimulation& _region, EssenceOrb& _orb)
{
  const TuningMvp& tuning = _region.Tuning();
  const auto& orb = tuning.ref.unit.essenceOrb;
  const auto& econ = tuning.ref.econ.orb;
  const double dt = tuning.ref.sim.tickPeriod;
  Neuron::SimRng& draw = _region.Draws().For(DrawPurpose::OrbStep, _orb.id);
  // R-UNIT-319 step 1: drag. Step 2: the timer and the wobble, in every state the MVP builds.
  _orb.velocity = _orb.velocity * orb.dragPerTick;
  --_orb.timerTicks;
  _orb.phase += orb.phaseStep;
  _orb.rates.x = std::clamp(_orb.rates.x + draw.NextSigned(econ.wobbleOffsetRange), orb.wobbleRateClamp.min, orb.wobbleRateClamp.max);
  _orb.rates.y = std::clamp(_orb.rates.y + draw.NextSigned(econ.wobbleOffsetRange), orb.wobbleRateClamp.min, orb.wobbleRateClamp.max);
  _orb.rates.z = std::clamp(_orb.rates.z + draw.NextSigned(econ.wobbleOffsetRange), orb.wobbleRateClamp.min, orb.wobbleRateClamp.max);
  _orb.hover = _orb.rates * std::sin(_orb.phase);
  // Step 3: the state step (R-UNIT-311, R-UNIT-312, R-UNIT-315).
  bool remove = false;
  switch (_orb.state)
  {
  case OrbState::Birth:
  {
    _orb.hover.y = std::max(_orb.hover.y, draw.NextUnit(orb.birthHoverMax));
    const double height = _orb.position.y - _orb.groundHeight;
    if (height > econ.birthMaxHeightWu)
    {
      _orb.hover.y = -(orb.birthPullBase + draw.NextUnit(orb.birthPullRange)) * std::min(height / orb.birthPullScaleHeight, 1.0);
    }
    else if (_orb.timerTicks <= 0)
    {
      // R-ECON-068: the floating time, (120 + U[−30, 30)) ÷ 3 s, as ticks of 1 + ⌊seconds ÷ tick period⌋.
      const double seconds = econ.floatingTicks.centerSeconds + draw.NextSigned(econ.floatingTicks.spreadSeconds);
      _orb.state = OrbState::Floating;
      _orb.timerTicks = TicksOf(1, seconds, dt);
    }
    break;
  }
  case OrbState::Floating:
    if (_orb.timerTicks <= 0)
    {
      _orb.state = OrbState::Ascent;
      _orb.timerTicks = static_cast<std::int32_t>(econ.ascentTicks);
    }
    break;
  case OrbState::Ascent:
    _orb.hover.y = std::max(_orb.hover.y, orb.ascentRiseMin + draw.NextUnit(orb.ascentRiseRange));
    remove = _orb.timerTicks <= 0;
    break;
  }
  // Step 4: the structure push; step 5: the integration, the map clamp and the floor (R-UNIT-322).
  if (_orb.push && _orb.state != OrbState::Ascent)
  {
    StructurePush(_region, _orb);
  }
  _orb.position = _orb.position + (_orb.velocity + _orb.hover) * dt;
  const MovementContext movement = _region.Movement();
  _orb.position.x = std::clamp(_orb.position.x, movement.minX, movement.maxX);
  _orb.position.z = std::clamp(_orb.position.z, movement.minZ, movement.maxZ);
  if (_orb.state != OrbState::Ascent)
  {
    _orb.position.y = std::max(_orb.position.y, _orb.groundHeight + orb.floorHeight);
  }
  return remove;
}

void WriteOrb(Neuron::BitWriter& _writer, const EssenceOrb& _orb)
{
  _writer.WriteBits(_orb.id, 64);
  _writer.WriteBits(_orb.owner, 64);
  _writer.WriteBits(_orb.team, 8);
  _writer.WriteBounded(static_cast<std::uint64_t>(_orb.state), ORB_STATE_MAX);
  WriteVec3(_writer, _orb.position);
  WriteVec3(_writer, _orb.velocity);
  WriteVec3(_writer, _orb.hover);
  WriteVec3(_writer, _orb.rates);
  WriteDouble(_writer, _orb.phase);
  WriteDouble(_writer, _orb.groundHeight);
  _writer.WriteSigned(_orb.timerTicks, 32);
  _writer.WriteBool(_orb.push);
}

bool ReadOrb(Neuron::BitReader& _reader, EssenceOrb& _orb)
{
  _orb.id = _reader.ReadBits(64);
  _orb.owner = _reader.ReadBits(64);
  _orb.team = static_cast<std::uint8_t>(_reader.ReadBits(8));
  _orb.state = static_cast<OrbState>(_reader.ReadBounded(ORB_STATE_MAX));
  bool finite = ReadVec3(_reader, _orb.position);
  finite = ReadVec3(_reader, _orb.velocity) && finite;
  finite = ReadVec3(_reader, _orb.hover) && finite;
  finite = ReadVec3(_reader, _orb.rates) && finite;
  finite = ReadDouble(_reader, _orb.phase) && finite;
  finite = ReadDouble(_reader, _orb.groundHeight) && finite;
  _orb.timerTicks = static_cast<std::int32_t>(_reader.ReadSigned(32));
  _orb.push = _reader.ReadBool();
  return !_reader.Failed() && finite && _orb.id != 0;
}

} // namespace Species
