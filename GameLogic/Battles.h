#pragma once

#include "BitReader.h"
#include "BitWriter.h"
#include "RelationMessages.h"
#include "StableId.h"
#include "Vec3.h"

#include <array>
#include <cstdint>
#include <expected>
#include <optional>

namespace Species
{

class RegionSimulation;
struct Settlement;

// The first state version whose regions hold expeditions, battles, battle results and respites (M3 task T6, ADR-161).
inline constexpr std::uint32_t STATE_VERSION_WITH_BATTLES = 13;

// ADR-161: the team slot of the expedition of the settlement at a site, whose figures leave the settlement's team while
// they are out; the slots after the four sites' teams.
inline constexpr std::uint8_t EXPEDITION_TEAM_FIRST = 4;
[[nodiscard]] constexpr std::uint8_t ExpeditionTeamOf(std::uint8_t _site) noexcept
{
  return static_cast<std::uint8_t>(EXPEDITION_TEAM_FIRST + _site);
}

// MVP-042 and MVP-043: where an expedition stands; a value of the saved state that is never renumbered.
enum class ExpeditionState : std::uint8_t
{
  Held = 0,     // declared, held by the warning until mvp.conflict.warning_minutes have passed
  Released = 1, // released, its battle not yet started
  Fighting = 2  // its battle in progress
};

inline constexpr std::uint64_t EXPEDITION_STATE_MAX = 2;

/// MVP-041 and MVP-042 (O98, ADR-161): an attack of the settlement at site `attacker` on the settlement at site
/// `target`, a raid or the claim battle of `claim`, by up to three of its Leaders and the Workers that followed them when
/// it was declared, who form its expedition's team. Its stable id is the subject of the attack's grievance (MVP-051).
struct Expedition
{
  StableId id = 0;
  std::uint8_t attacker = 0;
  std::uint8_t target = 0;
  StableId claim = 0; // 0 for a raid
  std::uint64_t declaredTick = 0;
  std::uint32_t sent = 0; // its Workers when it was declared, for the rout (MVP-045)
  bool unprovoked = false;
  std::int64_t grievanceMicro = 0; // the weight of the grievance the attack gave its target (MVP-048)
  ExpeditionState state = ExpeditionState::Held;
  std::array<StableId, EXPEDITION_LEADERS_MAX> leaders{};

  bool operator==(const Expedition&) const = default;
};

/// MVP-043 to MVP-046 (ADR-161): a battle in progress between an expedition and the settlement at site `defender`, its
/// objective and battlefield, the defender at its start, what each side has lost, the Essence moved to the defender, and
/// how long the attacker has held the objective.
struct Battle
{
  StableId expedition = 0;
  std::uint8_t attacker = 0;
  std::uint8_t defender = 0;
  StableId claim = 0;
  std::uint64_t startTick = 0;
  Neuron::Vec3 objective;                   // the target's Wellspring, or the claimed area's resource site
  Neuron::Vec3 fieldCenter;                 // MVP-043: the battlefield, a disc that holds the target's territory or the claimed area
  double fieldRadius = 0.0;                 // and wld.view.settlement_margin about it
  std::uint32_t defenderStart = 0;          // its figures
  std::int64_t defenderStoresMilliRu = 0;   // its stores
  std::int64_t defenderStrengthMilliRu = 0; // its strength (MVP-038)
  std::uint32_t defenderLost = 0;
  std::uint32_t attackerLost = 0;
  std::uint32_t essenceMoved = 0; // the expedition's deaths whose Essence the defender's territory took
  std::int32_t heldTicks = 0;     // the ticks the attacker has held the objective alone

  bool operator==(const Battle&) const = default;
};

// MVP-045: how a battle ended; a value of the saved state that is never renumbered.
enum class BattleEnd : std::uint8_t
{
  ObjectiveHeld = 0, // the attacker held the objective for mvp.battle.objective_hold_s
  LossCap = 1,       // the defender's losses reached its loss cap
  Rout = 2,          // the attacker fell below mvp.battle.rout_share of the Workers it sent
  Withdrawn = 3,     // the attacker withdrew by command
  Time = 4           // pil.battle.max_minutes passed
};

inline constexpr std::uint64_t BATTLE_END_MAX = 4;

/// MVP-048 and PIL-046 (ADR-161): what a battle came to: its sides, its end and winner, the Workers each lost, the stores
/// and Essence moved, whether a claimed area moved, the grievance its attack created and the respite it started. Its
/// renown is M3 task T8's.
struct BattleResult
{
  StableId expedition = 0;
  std::uint8_t attacker = 0;
  std::uint8_t defender = 0;
  StableId claim = 0;
  BattleEnd end = BattleEnd::ObjectiveHeld;
  bool attackerWon = false;
  std::uint64_t startTick = 0;
  std::uint64_t endTick = 0;
  std::uint32_t attackerLost = 0;
  std::uint32_t defenderLost = 0;
  std::int64_t storesMovedMilliRu = 0;
  std::uint32_t essenceMoved = 0;
  bool areaMoved = false;
  std::int64_t grievanceMicro = 0;
  std::uint64_t respiteUntil = 0; // 0 for none

  bool operator==(const BattleResult&) const = default;
};

/// MVP-047 (ADR-161): the settlement at `site` may not be attacked before tick `until`.
struct Respite
{
  std::uint8_t site = 0;
  std::uint64_t until = 0;

  bool operator==(const Respite&) const = default;
};

// ADR-161: the team is a settlement's or its expedition's: such teams are at peace with each other but in a battle.
[[nodiscard]] bool IsSettlementSide(const RegionSimulation& _region, std::uint8_t _team) noexcept;
// The settlement whose team, or whose expedition's team, _team is, or none.
[[nodiscard]] const Settlement* SettlementOfSide(const RegionSimulation& _region, std::uint8_t _team) noexcept;
// The expedition of the settlement at _site, or none; and the battle of an expedition, or none.
[[nodiscard]] const Expedition* ExpeditionOf(const RegionSimulation& _region, std::uint8_t _site) noexcept;
[[nodiscard]] const Battle* BattleOf(const RegionSimulation& _region, StableId _expedition) noexcept;
// MVP-047: the settlement at _site is under a respite at the region's next tick.
[[nodiscard]] bool UnderRespite(const RegionSimulation& _region, std::uint8_t _site) noexcept;

// MVP-046: the most figures the defender of the battle may lose: mvp.protection.loss_cap_worker_share of those it had at
// the start, and never so many that fewer than mvp.protection.worker_floor remain.
[[nodiscard]] std::uint32_t LossCapOf(const RegionSimulation& _region, const Battle& _battle) noexcept;

// MVP-041 and MVP-042 (ADR-161): the settlement at _attacker declares an attack with the Leaders named and the Workers
// following them on the settlement at _target, a raid, or the claim battle of its claim _claim. The attack pays
// mvp.conflict.attack_cost_ru, pil.conflict.unprovoked_cost_factor times as much with no cause on record (PIL-063); gives
// the target's Cladan the attack's grievance against the attacker (MVP-051); moves the Leaders and their Workers to the
// expedition's team, commanded by the settlement's commander; and walks them in formation to the edge of the target's
// territory, where the warning holds them (MVP-042). Returns the expedition's id.
[[nodiscard]] std::expected<StableId, AttackRefusal> DeclareAttack(RegionSimulation& _region, std::uint8_t _attacker,
                                                                   const std::array<StableId, EXPEDITION_LEADERS_MAX>& _leaders,
                                                                   std::uint8_t _target, StableId _claim);
// MVP-045 and PIL-087 (ADR-161): the settlement at _attacker withdraws its expedition: before its battle the attack
// ends, its grievance's cause removed and no respite started; in its battle, the battle ends Withdrawn.
[[nodiscard]] std::optional<AttackRefusal> WithdrawAttack(RegionSimulation& _region, std::uint8_t _attacker);

// MVP-060 (ADR-163): at the run's end, every expedition whose battle has not started ends as a withdrawal before its
// battle does, with no battle, no result and no respite; battles in progress finish.
void EndUnfoughtExpeditions(RegionSimulation& _region);

// MVP-042 to MVP-048 (ADR-161): the attacks' step at the end of the tick _tick: a held expedition released by its warning
// walks to its objective; a released one whose figures enter the target's territory, or the claimed area, starts its
// battle while fewer than wld.scale.battles_per_region_max are in progress, in the order of declaration (MVP-015);
// each battle counts its objective's hold and ends at the first of its ends; an expedition with no figure left ends.
void AdvanceBattles(RegionSimulation& _region, std::uint64_t _tick);

// MVP-044 and MVP-046 (ADR-161): the damage a figure of _team at _position takes from _amount (negative): none for a
// defender whose losses have reached its battle's loss cap, mvp.defense.damage_taken_factor of it for a defender inside
// its own territory.
[[nodiscard]] std::int32_t BattleDamage(const RegionSimulation& _region, std::uint8_t _team, const Neuron::Vec3& _position,
                                        std::int32_t _amount) noexcept;
// MVP-044, MVP-046 and PIL-045 (ADR-161): a figure of _team died at _position: its battle counts the loss, and an
// expedition figure's Essence that the defender's territory takes counts towards the battle's bound, beyond which it
// ascends. True when the death leaves an orb.
[[nodiscard]] bool BattleDeath(RegionSimulation& _region, std::uint8_t _team, const Neuron::Vec3& _position);

// The records canonically (ADR-081 item 7), and the values the writer wrote; false when they do not read whole.
void WriteExpedition(Neuron::BitWriter& _writer, const Expedition& _expedition);
[[nodiscard]] bool ReadExpedition(Neuron::BitReader& _reader, Expedition& _expedition);
void WriteBattle(Neuron::BitWriter& _writer, const Battle& _battle);
[[nodiscard]] bool ReadBattle(Neuron::BitReader& _reader, Battle& _battle);
void WriteBattleResult(Neuron::BitWriter& _writer, const BattleResult& _result);
[[nodiscard]] bool ReadBattleResult(Neuron::BitReader& _reader, BattleResult& _result);
void WriteRespite(Neuron::BitWriter& _writer, const Respite& _respite);
[[nodiscard]] bool ReadRespite(Neuron::BitReader& _reader, Respite& _respite);

} // namespace Species
