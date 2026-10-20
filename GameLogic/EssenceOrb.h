#pragma once

#include "BitReader.h"
#include "BitWriter.h"
#include "StableId.h"
#include "Vec3.h"

#include <cstdint>

namespace Species
{

class RegionSimulation;

// R-UNIT-310: the states of an Essence orb that the MVP builds. M1 has no taker and no territory, so every orb follows
// the reference's timed states and ascends (MVP-023). The values are state, never renumbered.
enum class OrbState : std::uint8_t
{
  Birth,
  Floating,
  Ascent
};

/// An Essence orb of the region's orb pool (R-UNIT-301, R-UNIT-306): a simulated object that is not an actor, never in
/// the actor grid.
struct EssenceOrb
{
  StableId id = 0;
  StableId owner = 0; // R-UNIT-303: the actor that released it
  std::uint8_t team = 0;
  OrbState state = OrbState::Birth;
  Neuron::Vec3 position;
  Neuron::Vec3 velocity;
  Neuron::Vec3 hover; // R-UNIT-320: the wobble velocity
  Neuron::Vec3 rates; // R-UNIT-320: the wobble amplitude of each axis
  double phase = 0.0;
  double groundHeight = 0.0; // R-UNIT-322: the floor reference, the terrain height at its creation
  std::int32_t timerTicks = 0;
  bool push = true; // R-UNIT-321: whether the structure push runs
};

// R-UNIT-303, R-UNIT-307: an orb of _team released by _owner at _position raised by the orb lift, with _velocity,
// and its start step's draws.
void SpawnOrb(RegionSimulation& _region, const Neuron::Vec3& _position, const Neuron::Vec3& _velocity, std::uint8_t _team, StableId _owner);

// R-UNIT-311 to R-UNIT-322: one advance of an orb; true when it asks to be removed, at the end of its ascent.
[[nodiscard]] bool AdvanceOrb(RegionSimulation& _region, EssenceOrb& _orb);

void WriteOrb(Neuron::BitWriter& _writer, const EssenceOrb& _orb);
[[nodiscard]] bool ReadOrb(Neuron::BitReader& _reader, EssenceOrb& _orb);

} // namespace Species
