#pragma once

#include "GameMessages.h"
#include "ReplicatedState.h"
#include "StableId.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace Species
{

// The relations' commands (M3 task T5, ADR-160), written by hand in the form the generated codecs will take (ADR-062,
// ADR-093), as the settlement's are (ADR-136). A command names its settlement by its team, which its sender must
// command (ADR-087 item 2), and a field out of its bounds fails the decoding, so the queue drops the command as
// malformed.

// The valley's values that are both saved state (ADR-159 to ADR-161) and on the wire (ADR-165): each is never
// renumbered.

// MVP-051: what a grievance arose from.
enum class GrievanceCause : std::uint8_t
{
  Pressure = 0, // territory pressure while the two settlements are in contact (MVP-027)
  Claim = 1,    // a claim made on an area the holder holds (MVP-053)
  Attack = 2    // an attack declared on the holder (MVP-042)
};

inline constexpr std::uint64_t GRIEVANCE_CAUSE_MAX = 2;

// MVP-053: where a claim stands.
enum class ClaimState : std::uint8_t
{
  Open = 0,     // on an area another settlement holds: settled by a claim battle or the claimant's withdrawal
  Waiting = 1,  // on the central resource site while nobody holds it: held after mvp.claim.uncontested_minutes
  Contested = 2 // on the central resource site while nobody holds it, which another settlement also claims
};

inline constexpr std::uint64_t CLAIM_STATE_MAX = 2;

// MVP-042 and MVP-043: where an expedition stands.
enum class ExpeditionState : std::uint8_t
{
  Held = 0,     // declared, held by the warning until mvp.conflict.warning_minutes have passed
  Released = 1, // released, its battle not yet started
  Fighting = 2  // its battle in progress
};

inline constexpr std::uint64_t EXPEDITION_STATE_MAX = 2;

// MVP-045: how a battle ended.
enum class BattleEnd : std::uint8_t
{
  ObjectiveHeld = 0, // the attacker held the objective for mvp.battle.objective_hold_s
  LossCap = 1,       // the defender's losses reached its loss cap
  Rout = 2,          // the attacker fell below mvp.battle.rout_share of the Workers it sent
  Withdrawn = 3,     // the attacker withdrew by command
  Time = 4           // pil.battle.max_minutes passed
};

inline constexpr std::uint64_t BATTLE_END_MAX = 4;

// MVP-053 and WLD-086: the areas of a region that a claim names. Each half of a valley border area is its pair's index
// in VALLEY_PAIRS times two plus its side, the half nearer that side's site; the valley's central resource site follows.
inline constexpr std::uint8_t CLAIM_AREA_CENTRAL = 8;
inline constexpr std::uint64_t CLAIM_AREA_MAX = CLAIM_AREA_CENTRAL;

// MVP-053: why a claim or a withdrawal is refused, checked in this order; the reason of its command's outcome, a wire
// value that is never renumbered.
enum class ClaimRefusal : std::uint8_t
{
  NoSettlement = 1,   // the team is no settlement's
  NotClaimable = 2,   // a half of a valley border area that is not its valley neighbour's pair, or the claimant's own
  NotHeld = 3,        // a half that its neighbour does not hold
  AlreadyHeld = 4,    // an area the claimant holds
  AlreadyClaimed = 5, // an area the claimant already claims
  NoSuchClaim = 6,    // a withdrawal that names no claim of the claimant's
  CoolingDown = 7     // an area whose last claim ended less than mvp.claim.cooldown_minutes ago (O102, DEC-1008)
};

inline constexpr std::uint8_t CLAIM_REFUSAL_MAX = static_cast<std::uint8_t>(ClaimRefusal::CoolingDown);

// MVP-054: why a tribute is refused, checked in this order; the reason of its command's outcome, a wire value that is
// never renumbered.
enum class TributeRefusal : std::uint8_t
{
  NoSettlement = 1, // the team is no settlement's
  NoGrievance = 2,  // the named settlement's Cladan holds no such grievance against the sender
  NoAmount = 3,     // an amount of nothing
  CannotAfford = 4  // the stores do not hold the amount
};

inline constexpr std::uint8_t TRIBUTE_REFUSAL_MAX = static_cast<std::uint8_t>(TributeRefusal::CannotAfford);

// MVP-041 and MVP-042 (ADR-161): why a declared attack or its withdrawal is refused, checked in this order; the reason
// of its command's outcome, a wire value that is never renumbered.
enum class AttackRefusal : std::uint8_t
{
  NoSettlement = 1, // the team is no settlement's
  Underway = 2,     // the settlement already has an expedition out; for a withdrawal, it has none
  NoTarget = 3,     // a target that is no settlement or the attacker's own, or a claim that is not the attacker's open one
  UnderRespite = 4, // the target is under a respite (MVP-047)
  NoLeader = 5,     // no live Leader of the settlement among those named
  TooLarge = 6,     // the Leaders' Workers are more than mvp.expedition.size_max
  CannotAfford = 7, // the stores do not hold the attack's cost
  Quiet = 8         // the run is in its last mvp.run.quiet_minutes (MVP-060, ADR-163)
};

inline constexpr std::uint8_t ATTACK_REFUSAL_MAX = static_cast<std::uint8_t>(AttackRefusal::Quiet);
// O98: an expedition's Leaders, at most.
inline constexpr std::size_t EXPEDITION_LEADERS_MAX = 3;

// ADR-161: the team slot of the expedition of the settlement at a site, whose figures leave the settlement's team while
// they are out; the slots after the four sites' teams. In GameShared from M3 task T12, so that the client knows its
// expedition's figures (ADR-167).
inline constexpr std::uint8_t EXPEDITION_TEAM_FIRST = 4;
[[nodiscard]] constexpr std::uint8_t ExpeditionTeamOf(std::uint8_t _site) noexcept
{
  return static_cast<std::uint8_t>(EXPEDITION_TEAM_FIRST + _site);
}

/// MVP-053: the settlement claims an area: a half of a valley border area that its valley neighbour holds, or the
/// valley's central resource site. A claim refused when it executes has no effect, and its outcome's reason is the
/// ClaimRefusal.
struct MakeClaimCommand
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::MakeClaim);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 5; // the 3-byte header, the team and a 4-bit area

  std::uint8_t team = 0;
  std::uint8_t area = 0;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

/// MVP-053 and PIL-122: the settlement withdraws its claim, named by its stable id, which settles it with no battle.
struct WithdrawClaimCommand
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::WithdrawClaim);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 12; // the header, the team and a 64-bit id

  std::uint8_t team = 0;
  StableId claim = 0;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

/// MVP-041 and MVP-042 (O98, ADR-161): the settlement declares an attack with up to three of its Leaders and the Workers
/// following them, as one expedition: a raid on the settlement at site `target`, or, when `claim` names one of its
/// claims, the claim battle that settles it. A Leader id of 0 names none. A declaration refused when it executes has no
/// effect, and its outcome's reason is the AttackRefusal.
struct DeclareAttackCommand
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::DeclareAttack);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 37; // the header, the team, three 64-bit Leaders, a 2-bit site and a 64-bit claim

  std::uint8_t team = 0;
  std::array<StableId, EXPEDITION_LEADERS_MAX> leaders{};
  std::uint8_t target = 0;
  StableId claim = 0;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

/// MVP-045 and PIL-087 (ADR-161): the settlement withdraws its expedition: before its battle the attack ends with no
/// battle; in its battle, the battle ends as a rout.
struct WithdrawAttackCommand
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::WithdrawAttack);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 4; // the header and the team

  std::uint8_t team = 0;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

/// MVP-054: the settlement sends tribute from its stores to the settlement at site `to`, whose Cladan holds the
/// grievance against it that the cause and the subject name, to ease that grievance. A tribute refused when it executes
/// has no effect, and its outcome's reason is the TributeRefusal.
struct SendTributeCommand
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::SendTribute);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 21; // the header, the team, a 2-bit site, a 2-bit cause and two 64-bit fields

  std::uint8_t team = 0;
  std::uint8_t to = 0;
  std::uint8_t cause = 0; // GrievanceCause's value
  std::uint64_t subject = 0;
  std::int64_t amountMilliRu = 0;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

// A region's local sites, 0 to 3 (WLD-040), the settlements a valley status lists at most, and the areas a claim names.
inline constexpr std::uint64_t VALLEY_SITE_MAX = 3;
inline constexpr std::size_t VALLEY_SETTLEMENTS_MAX = VALLEY_SITE_MAX + 1;
inline constexpr std::size_t VALLEY_AREAS = CLAIM_AREA_MAX + 1;
// The grievances and claims a valley status lists at most: the claims a region holds at most, and far more grievances
// than one settlement's relations hold (ADR-165).
inline constexpr std::uint64_t VALLEY_GRIEVANCES_MAX = 255;
inline constexpr std::uint64_t VALLEY_CLAIMS_MAX = 36;

/// MVP-098, MVP-102 (ADR-165): a request for the valley's status as the settlement of the team, which its sender must
/// command, sees it. The server answers it outside the queue, as it answers a settlement's status (ADR-136): a permitted
/// request is followed by the status of the tick after which it is taken.
struct ReadValleyStatusCommand
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::ReadValleyStatus);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 4; // the header and the team

  std::uint8_t team = 0;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

/// MVP-050: a grievance the Cladan at site `holder` holds against the settlement at site `against`, with its cause, what
/// the cause names (0 for territory pressure), whether the attack was unprovoked, whether its cause is removed, and its
/// weight in millionths of a weight unit.
struct GrievanceLine
{
  std::uint8_t holder = 0;
  std::uint8_t against = 0;
  GrievanceCause cause = GrievanceCause::Pressure;
  std::uint64_t subject = 0;
  bool unprovoked = false;
  bool causeRemoved = false;
  std::int64_t weightMicro = 0;

  friend bool operator==(const GrievanceLine&, const GrievanceLine&) = default;
};

/// MVP-053: a claim, by its stable id, its claimant's site, its area, the tick it was made and its state.
struct ClaimLine
{
  StableId id = 0;
  std::uint8_t claimant = 0;
  std::uint8_t area = 0;
  std::uint64_t madeTick = 0;
  ClaimState state = ClaimState::Open;

  friend bool operator==(const ClaimLine&, const ClaimLine&) = default;
};

/// MVP-053 (O102): an area's holder by site, none while nobody holds it, and the first tick a claim may be made on it
/// again, 0 when no claim on it has ended.
struct AreaLine
{
  std::optional<std::uint8_t> holder;
  std::uint64_t claimableTick = 0;

  friend bool operator==(const AreaLine&, const AreaLine&) = default;
};

/// MVP-041 and MVP-042: an expedition by its stable id, its attacker's and its target's sites, the claim its battle
/// settles (0 for a raid), the tick it was declared, its state, whether the attack was unprovoked, and its Workers when
/// it was declared.
struct ExpeditionLine
{
  StableId id = 0;
  std::uint8_t attacker = 0;
  std::uint8_t target = 0;
  StableId claim = 0;
  std::uint64_t declaredTick = 0;
  ExpeditionState state = ExpeditionState::Held;
  bool unprovoked = false;
  std::uint32_t sent = 0;

  friend bool operator==(const ExpeditionLine&, const ExpeditionLine&) = default;
};

/// MVP-043 to MVP-046: a battle in progress, by its expedition, its sides, its claim and the tick it started; for a battle
/// the steward's settlement is a side of (`side`), also its objective, its battlefield (WLD-244) and the figures each side
/// has lost against the defender's loss cap, which another settlement's steward sees as none.
struct BattleLine
{
  StableId expedition = 0;
  std::uint8_t attacker = 0;
  std::uint8_t defender = 0;
  StableId claim = 0;
  std::uint64_t startTick = 0;
  bool side = false;
  QuantizedPosition objective;
  QuantizedPosition fieldCenter;
  std::uint32_t fieldRadius = 0; // in 1/16 wu, as a settlement's radius (ADR-139)
  std::uint32_t attackerLost = 0;
  std::uint32_t defenderLost = 0;
  std::uint32_t lossCap = 0;

  friend bool operator==(const BattleLine&, const BattleLine&) = default;
};

/// MVP-047: a settlement under a respite until a tick.
struct RespiteLine
{
  std::uint8_t site = 0;
  std::uint64_t until = 0;

  friend bool operator==(const RespiteLine&, const RespiteLine&) = default;
};

/// MVP-061 and MVP-062: a settlement's renown by source, as the tuning derives it from what it did (ADR-163).
struct RenownLine
{
  std::uint8_t site = 0;
  std::uint32_t defense = 0;
  std::uint32_t claims = 0;
  std::uint32_t easing = 0;
  std::uint32_t structures = 0;

  friend bool operator==(const RenownLine&, const RenownLine&) = default;
};

/// MVP-098 and MVP-102 (ADR-165): the valley as the settlement of the team sees it after a tick, for the relations panel,
/// the warning, the battles, the renown standing and the time left: the run's end, its quiet minutes and whether it has
/// ended (MVP-060, MVP-063); each settlement's renown (MVP-062); every area's holder and when it is claimable again; every
/// claim; the grievances its Cladan holds and those held against it; its expedition and those declared on it; every
/// battle in progress, with the figures of those it is a side of; and every respite. Each list is in the region's order.
struct ValleyStatus
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::ValleyStatus);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t GRIEVANCE_BITS = 2 + 2 + 2 + 64 + 1 + 1 + 64;
  static constexpr std::size_t CLAIM_BITS = 64 + 2 + 4 + 64 + 2;
  static constexpr std::size_t AREA_BITS = 3 + 64;
  static constexpr std::size_t EXPEDITION_BITS = 64 + 2 + 2 + 64 + 64 + 2 + 1 + 32;
  static constexpr std::size_t POSITION_BITS = POSITION_HORIZONTAL_BITS * 2 + POSITION_VERTICAL_BITS;
  static constexpr std::size_t BATTLE_BITS = 64 + 2 + 2 + 64 + 64 + 1 + POSITION_BITS * 2 + SETTLEMENT_RADIUS_BITS + std::size_t{32} * 3;
  static constexpr std::size_t RESPITE_BITS = 2 + 64;
  static constexpr std::size_t RENOWN_BITS = 2 + 32 * 4;
  // The tick, the team, the run, the areas, and the six lists' counts.
  static constexpr std::size_t FIXED_BITS = 64 + 8 + 64 + 64 + 1 + AREA_BITS * VALLEY_AREAS + 3 + 8 + 6 + 3 + 3 + 3;
  static constexpr std::size_t MAX_BYTES =
    3 + (FIXED_BITS + VALLEY_SETTLEMENTS_MAX * (RENOWN_BITS + EXPEDITION_BITS + BATTLE_BITS + RESPITE_BITS) +
         VALLEY_GRIEVANCES_MAX * GRIEVANCE_BITS + VALLEY_CLAIMS_MAX * CLAIM_BITS + 7) /
          8;

  std::uint64_t tick = 0;
  std::uint8_t team = 0;
  std::uint64_t runEndTick = 0;
  std::uint64_t quietTick = 0; // the first tick of the run's quiet minutes
  bool runEnded = false;
  std::vector<RenownLine> renown;
  std::array<AreaLine, VALLEY_AREAS> areas{};
  std::vector<ClaimLine> claims;
  std::vector<GrievanceLine> grievances;
  std::vector<ExpeditionLine> expeditions;
  std::vector<BattleLine> battles;
  std::vector<RespiteLine> respites;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

/// MVP-048 and PIL-046 (ADR-165): a battle's result, on the reliable channel to every steward once the battle has ended,
/// after the notice of its end: its expedition, its sides, the claimed area if any, its end and winner, its ticks and
/// whether a claimed area moved, which are the public line every steward sees; and, for a steward whose settlement was a
/// side (`side`), the Workers each side lost, the stores and Essence moved, the grievance the attack created, the respite
/// it started and the renown it gave, which another steward receives as none.
struct BattleResultNotice
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::BattleResultNotice);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 3 + (64 + 2 + 2 + 4 + 3 + 1 + 64 + 64 + 1 + 1 + 32 * 2 + 64 + 32 + 64 + 64 + 32 + 7) / 8;

  StableId expedition = 0;
  std::uint8_t attacker = 0;
  std::uint8_t defender = 0;
  std::optional<std::uint8_t> area; // the claimed area of a claim battle; none for a raid
  BattleEnd end = BattleEnd::ObjectiveHeld;
  bool attackerWon = false;
  std::uint64_t startTick = 0;
  std::uint64_t endTick = 0;
  bool areaMoved = false;
  bool side = false;
  std::uint32_t attackerLost = 0;
  std::uint32_t defenderLost = 0;
  std::int64_t storesMovedMilliRu = 0;
  std::uint32_t essenceMoved = 0;
  std::int64_t grievanceMicro = 0;
  std::uint64_t respiteUntil = 0; // 0 for none
  std::uint32_t renown = 0;       // the renown it gave its winner (MVP-061)

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

/// MVP-063 (ADR-169): the steward asks for the epilogue of the settlement of a team it commands; the world server answers
/// after the next tick with the Epilogue, whether or not the run has ended, and while it has.
struct ReadEpilogueCommand
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::ReadEpilogue);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 4; // the 3-byte header and the team

  std::uint8_t team = 0;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

// MVP-063 (ADR-169): a settlement's strength over the run, at most this many samples evenly taken from the server's.
inline constexpr std::uint64_t EPILOGUE_SAMPLES_MAX = 48;

/// MVP-063 and PIL-125 (ADR-169): a settlement's history over the run: its renown by source, the battles it won and lost
/// on either side, its claims made and how each ended, the structures it completed, the grievances its Cladan took, its
/// most Workers and its Workers at the last record, and its strength over the run.
struct EpilogueLine
{
  std::uint8_t site = 0;
  RenownLine renown;
  std::uint32_t battlesWon = 0;
  std::uint32_t battlesLost = 0;
  std::uint32_t claimsMade = 0;
  std::uint32_t claimsWon = 0;
  std::uint32_t claimsLost = 0;
  std::uint32_t claimsWithdrawn = 0;
  std::uint32_t structures = 0;
  std::uint32_t grievances = 0;
  std::uint32_t workersMost = 0;
  std::uint32_t workersLast = 0;
  std::vector<std::int64_t> strengthMilliRu; // evenly over the server's samples, the first and the last among them

  friend bool operator==(const EpilogueLine&, const EpilogueLine&) = default;
};

/// MVP-063 (ADR-169): the epilogue of the settlement of the team: the tick, the run's end and whether it has ended; every
/// settlement's history in the order of the standing (MVP-062); and the steward's statistics: its settlement's growth
/// steps, the largest gap between two in ticks, and the tick its territory first reached a halfway line, 0 for never.
struct Epilogue
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::Epilogue);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES =
    3 + (64 + 8 + 64 + 1 + 3 + VALLEY_SETTLEMENTS_MAX * (2 + 4 * 32 + 10 * 32 + 6 + EPILOGUE_SAMPLES_MAX * 64) + 32 + 64 + 64 + 7) / 8;

  std::uint64_t tick = 0;
  std::uint8_t team = 0;
  std::uint64_t runEndTick = 0;
  bool runEnded = false;
  std::vector<EpilogueLine> settlements;
  std::uint32_t growthSteps = 0;
  std::uint64_t largestGapTicks = 0;
  std::uint64_t firstHalfwayTick = 0;

  friend bool operator==(const Epilogue&, const Epilogue&) = default;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

} // namespace Species
