#pragma once

#include "BitReader.h"
#include "BitWriter.h"
#include "FigureKind.h"
#include "SimRng.h"
#include "StableId.h"
#include "TuningMvp.h"
#include "Vec3.h"

#include <cstdint>

namespace Species
{

// R-UNIT-025: the damage kinds of a health change; their order carries no meaning.
enum class DamageKind : std::uint8_t
{
  Unresistable,
  Laser,
  Explosive,
  TurretRound,
  Fire
};

/// One Worker or Leader of a region (R-SIM-101, R-SIM-105): the simulation state that the figure rules read and write.
/// Its team, pool and slot are its container's (R-SIM-131); its stable id stands for the reference's serial (ADR-083
/// item 5). A dead figure's health is its corpse fade (R-UNIT-026).
struct Figure
{
  StableId id = 0;
  FigureKind kind = FigureKind::Worker;
  Neuron::Vec3 position;
  Neuron::Vec3 velocity;
  Neuron::Vec3 front;
  Neuron::Vec3 roamCenter; // R-UNIT-020: the spawn point, the position the spawn requested
  double roamRange = 0.0;
  double speed = 0.0;                   // R-UNIT-011: wu/s, from the stat table
  std::int32_t health = 0;              // R-UNIT-012: never above ref.unit.health_max
  std::int32_t reloadIntervalTicks = 0; // R-UNIT-011: the rate in ticks
  std::int32_t reloadTimerTicks = 0;    // R-UNIT-015: the figure fires only at 0
  bool onGround = false;
  bool enabled = true;
  bool destroyed = false; // R-SIM-113: removed at its next advance, with no corpse
  bool dead = false;
  bool inFormation = false; // R-UNIT-116: a Worker in a Leader's formation
};

// The rules' draws for one figure in one tick (ADR-082 item 3): its simulation key, the tick and, in development builds,
// the owner's ledger of the tick's contexts.
struct DrawScope
{
  const Neuron::SipHashKey* key = nullptr;
  std::uint64_t tick = 0;
  Neuron::DrawLedger* ledger = nullptr;
};

// R-UNIT-011 and R-UNIT-015 (R-SIM-109): sets the kind, the stats of the stat table, the reload interval and the
// initial reload timer, from one draw of purpose ReloadTimer.
void SetKind(Figure& _figure, FigureKind _kind, const TuningMvp& _tuning, const DrawScope& _draws);

// R-SIM-226 step 6, R-UNIT-019, R-UNIT-059: the front of a figure spawned with _velocity.
[[nodiscard]] Neuron::Vec3 SpawnFront(const Figure& _figure, const Neuron::Vec3& _velocity, const TuningMvp& _tuning,
                                      const DrawScope& _draws);

struct HealthChange
{
  bool registered = false; // R-UNIT-026: the caller knocks back only a registered hit
  bool killed = false;     // the change made the figure dead
};

// R-UNIT-026, R-UNIT-027 and R-UNIT-116 for the Worker and the Leader: an integer change of health of one damage
// kind. A kill makes the figure dead with its health at the corpse fade start; the Essence orb of R-UNIT-038 and the
// presentation events are the caller's, from the result.
HealthChange ApplyHealthChange(Figure& _figure, std::int32_t _amount, DamageKind _kind, const TuningMvp& _tuning);

// R-SIM-111 steps 1 to 3 and the corpse fade of R-UNIT-034: true when the figure asks to be removed.
[[nodiscard]] bool BaseAdvance(Figure& _figure, const TuningMvp& _tuning);

// The figure canonically (ADR-081 item 7): every field in declaration order, binary64 as its 64 bits.
void WriteFigure(Neuron::BitWriter& _writer, const Figure& _figure);
// The values WriteFigure wrote; false when they do not read whole or name no kind.
[[nodiscard]] bool ReadFigure(Neuron::BitReader& _reader, Figure& _figure);

} // namespace Species
