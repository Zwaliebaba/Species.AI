#pragma once

#include "BitReader.h"
#include "BitWriter.h"
#include "StableId.h"
#include "Vec3.h"

#include <cstdint>

namespace Species
{

class RegionSimulation;

/// A laser bolt of the region's laser-bolt pool (R-CRATE-440, R-SIM-125): it moves for its life's moves and is removed on
/// the advance after, ricochets off the terrain once, and hits one enemy at most.
struct LaserBolt
{
  StableId id = 0;
  std::uint8_t team = 0;
  Neuron::Vec3 position;
  Neuron::Vec3 velocity;
  std::int32_t movesLeft = 0; // R-CRATE-440: the moves still to make
  bool ricocheted = false;    // R-CRATE-441 step 4: its first ground contact is behind it
  bool harmless = false;      // R-CRATE-444: it has hit
};

// R-CRATE-400: the kinds of the region's effect pool that the MVP builds. Muzzle flashes, particles and the other
// presentation of the weapons are effect events, not pool objects (ADR-105). The values are state, never renumbered.
enum class EffectKind : std::uint8_t
{
  Grenade,
  ShockwaveRing
};

/// An effect of the region's effect pool (R-CRATE-400): a grenade, a thrown weapon with a fuse (R-CRATE-410,
/// R-CRATE-415), or the shockwave ring of an explosion (R-CRATE-470).
struct Effect
{
  StableId id = 0;
  EffectKind kind = EffectKind::Grenade;
  std::uint8_t team = 0;
  Neuron::Vec3 position;
  Neuron::Vec3 velocity;
  double force = 0.0;          // R-CRATE-410: a thrown weapon's bounce force
  double power = 0.0;          // R-CRATE-415: a grenade's power
  std::int32_t fuseTicks = 0;  // R-CRATE-415: a grenade's fuse
  double sizeSeconds = 0.0;    // R-CRATE-470: a ring's size s
  std::int32_t activeTick = 0; // R-CRATE-470: the ring's advances so far, its active tick k
};

// R-CRATE-405, fire a laser: a bolt of _team from _position with _velocity, for the laser level's life (DEC-115).
void FireLaser(RegionSimulation& _region, std::uint8_t _team, const Neuron::Vec3& _position, const Neuron::Vec3& _velocity);

// R-CRATE-405, throw a weapon: a grenade of _team from _position towards _target, with the throw force of the distance
// capped at the match's grenade level, along the front lifted by the front lift.
void ThrowGrenade(RegionSimulation& _region, std::uint8_t _team, const Neuron::Vec3& _position, const Neuron::Vec3& _target);

// R-CRATE-405, create a shockwave ring: a ring of size _sizeSeconds of _team at _position (R-CRATE-470).
void CreateShockwaveRing(RegionSimulation& _region, const Neuron::Vec3& _position, double _sizeSeconds, std::uint8_t _team);

// R-CRATE-406, R-SIM-388: the explosion of range _range and damage _damage of _team at _position: every actor within
// the actor reach takes its share and is knocked back, the exploding team's kills are counted, and its shockwave ring is
// created. The MVP has no structure, so step 6 is not built. Returns the Workers of other teams it killed.
std::uint32_t Explode(RegionSimulation& _region, const Neuron::Vec3& _position, double _range, double _damage, std::uint8_t _team);

// R-CRATE-441 to R-CRATE-444: one advance of a bolt; true when it asks to be removed.
[[nodiscard]] bool AdvanceLaserBolt(RegionSimulation& _region, LaserBolt& _bolt);
// R-CRATE-411, R-CRATE-415, R-CRATE-470, R-CRATE-471: one advance of an effect; true when it asks to be removed.
[[nodiscard]] bool AdvanceEffect(RegionSimulation& _region, Effect& _effect);

// The objects canonically (ADR-081 item 7), and the values the writers wrote; false when they do not read whole.
void WriteLaserBolt(Neuron::BitWriter& _writer, const LaserBolt& _bolt);
[[nodiscard]] bool ReadLaserBolt(Neuron::BitReader& _reader, LaserBolt& _bolt);
void WriteEffect(Neuron::BitWriter& _writer, const Effect& _effect);
[[nodiscard]] bool ReadEffect(Neuron::BitReader& _reader, Effect& _effect);

} // namespace Species
