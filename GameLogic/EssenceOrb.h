#pragma once

#include "BitReader.h"
#include "BitWriter.h"
#include "OrbState.h"
#include "StableId.h"
#include "Vec3.h"

#include <cstdint>

namespace Species
{

class RegionSimulation;

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
  // From state version 9 (M2 task T6):
  std::uint8_t settlement = 0; // MVP-023: the site of the settlement it drifts to or waits at
};

// R-UNIT-303, R-UNIT-307: an orb of _team released by _owner at _position raised by the orb lift, with _velocity,
// and its start step's draws.
void SpawnOrb(RegionSimulation& _region, const Neuron::Vec3& _position, const Neuron::Vec3& _velocity, std::uint8_t _team, StableId _owner);

// R-UNIT-311 to R-UNIT-322: one advance of an orb; true when it asks to be removed, at the end of its ascent or when it
// becomes a Worker. MVP-023 and MVP-025 (ADR-134): a floating orb inside a settlement's territory drifts to its
// Wellspring at mvp.essence.drift_speed; on reaching it, within the Wellspring's radius, it becomes a Worker at the
// Wellspring's door while the settlement is below its population limit, waits there while fewer than
// mvp.essence.wait_max of its orbs wait, and otherwise ascends; a waiting orb becomes a Worker as soon as there is room.
[[nodiscard]] bool AdvanceOrb(RegionSimulation& _region, EssenceOrb& _orb);

// An orb canonically (ADR-081 item 7), and the values the writer wrote; an orb of a state version before 9 has no
// settlement and none of the states of M2.
void WriteOrb(Neuron::BitWriter& _writer, const EssenceOrb& _orb);
[[nodiscard]] bool ReadOrb(Neuron::BitReader& _reader, EssenceOrb& _orb, std::uint32_t _stateVersion);

} // namespace Species
