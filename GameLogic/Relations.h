#pragma once

#include "BitReader.h"
#include "BitWriter.h"

#include <cstdint>

namespace Species
{

class RegionSimulation;

// MVP-051: what a grievance arose from; a value of the saved state that is never renumbered.
enum class GrievanceCause : std::uint8_t
{
  Pressure = 0, // territory pressure while the two settlements are in contact (MVP-027)
  Claim = 1,    // a claim made on an area the holder holds (MVP-053)
  Attack = 2    // an attack declared on the holder (MVP-042)
};

inline constexpr std::uint64_t GRIEVANCE_CAUSE_MAX = 2;

// A weight unit in a grievance's integer millionths.
inline constexpr std::int64_t WEIGHT_MICRO = 1'000'000;

/// MVP-050 and MVP-052 (ADR-159): a grievance that the Cladan of the settlement at site `holder` holds against the
/// settlement at site `against`: its cause and what the cause names, its weight in integer millionths of a weight unit,
/// and whether its cause is removed, which makes it fade pil.conflict.settled_fade_factor times faster (PIL-122). A
/// region holds its grievances in the order of holder, against, cause and subject, one for each.
struct Grievance
{
  std::uint8_t holder = 0;
  std::uint8_t against = 0;
  GrievanceCause cause = GrievanceCause::Pressure;
  std::uint64_t subject = 0; // what the cause names: 0 for territory pressure, else the claim or the attack, never 0
  bool unprovoked = false;   // an attack with no cause on record (PIL-063)
  bool causeRemoved = false;
  std::int64_t weightMicro = 0;

  bool operator==(const Grievance&) const = default;
};

// MVP-050: the anger of the Cladan at site _holder against the settlement at site _against, the sum of the weights of
// its grievances against it, in millionths.
[[nodiscard]] std::int64_t AngerMicro(const RegionSimulation& _region, std::uint8_t _holder, std::uint8_t _against) noexcept;

// MVP-027: the settlements at sites _first and _second are in contact: they are valley neighbours and their territories
// meet, at their pair's halfway line or, once a claim has moved a half of their border area, at its edge (ADR-160).
[[nodiscard]] bool InContact(const RegionSimulation& _region, std::uint8_t _first, std::uint8_t _second);

// MVP-051: a claim or an attack gives the Cladan at site _holder a grievance of its cause's weight against the
// settlement at site _against, mvp.grievance.unprovoked_factor times heavier for an unprovoked attack; the same cause
// naming the same subject adds to the grievance it gave before. Territory pressure is the relations' own step.
void AddGrievance(RegionSimulation& _region, std::uint8_t _holder, std::uint8_t _against, GrievanceCause _cause, std::uint64_t _subject,
                  bool _unprovoked = false);

// PIL-122: the cause of every grievance of _cause that names _subject is removed: a claim withdrawn or settled, or an
// attack whose battle is over.
void RemoveCause(RegionSimulation& _region, GrievanceCause _cause, std::uint64_t _subject);

// MVP-052: a battle result removes mvp.grievance.battle_relief_won of the attacker's grievances against the target when
// the attacker won, and mvp.grievance.battle_relief_lost when it lost.
void RelieveAfterBattle(RegionSimulation& _region, std::uint8_t _attacker, std::uint8_t _target, bool _attackerWon);

// MVP-051 and MVP-052 (ADR-159): the relations' step, at each tick that starts a second of world time: each pair of
// valley neighbours in contact adds a second's territory pressure to each one's grievance against the other, and a
// pair no longer in contact removes its cause; then every grievance fades by a second's share, and one whose cause is
// removed ends once it weighs less than half a unit, which the relations panel shows as none.
void AdvanceRelations(RegionSimulation& _region, std::uint64_t _tick);

// The grievance canonically (ADR-081 item 7), and the values the writer wrote; false when they do not read whole.
void WriteGrievance(Neuron::BitWriter& _writer, const Grievance& _grievance);
[[nodiscard]] bool ReadGrievance(Neuron::BitReader& _reader, Grievance& _grievance);

} // namespace Species
