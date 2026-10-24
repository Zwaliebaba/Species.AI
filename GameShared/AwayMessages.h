#pragma once

#include "GameMessages.h"
#include "RelationMessages.h"
#include "StableId.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace Species
{

// The messages of away play (M4, ADR-185, ADR-187), written by hand in the form the generated codecs will take (ADR-062,
// ADR-093), as the settlement's and the relations' are (ADR-136, ADR-160).

// The values of away play that are both saved state (ADR-137, ADR-185, ADR-186) and on the wire (ADR-187): each is
// never renumbered.

// PIL-131: why a world froze.
enum class FreezeCause : std::uint8_t
{
  ServerStall = 0,   // the tick loop gave up 10 s or more (ADR-095)
  UncleanStop = 1,   // the server stopped without its final snapshot; the time since the snapshot it opened (MVP-075)
  ServerStopped = 2, // MVP-071 (ADR-188): the server was not running between its clean stop and its next start
  Sleep = 3          // MVP-071 (ADR-188): the computer slept; the time the tick loop gave up across it
};

inline constexpr std::uint64_t FREEZE_CAUSE_MAX = 3;

/// PIL-079 (MVP-073): the default orders a steward who sets none leaves the deputy, by which each deputy action is
/// permitted (PIL-026), and the steward's own declared attack, whose battle the deputy fights while the steward is away
/// but never declares again (O110).
enum class StandingOrder : std::uint8_t
{
  DefendTheSettlement = 0,
  KeepGathering = 1,
  FinishTheQueuedConstruction = 2,
  NeverAttack = 3,
  TheStewardsAttack = 4
};

// The bound a standing order is written with, which leaves room for the orders a later stage adds (PIL-083).
inline constexpr std::uint64_t STANDING_ORDER_MAX = 15;

/// MVP-073 (ADR-186): what a deputy action did.
enum class DeputyActionKind : std::uint8_t
{
  LeaderToObjective = 0 // sent a Leader's formation to its battle's objective; the detail is the other side's site
};

inline constexpr std::uint64_t DEPUTY_ACTION_KIND_MAX = 15;

/// MVP-072 (ADR-185): the client's word that its steward gave input, sent on the reliable channel when the input router
/// sees input and no report has gone for PRESENCE_REPORT_SECONDS. It is read, not queued: the server notes the input's
/// tick for the steward's presence and answers it as it answers a request, and it changes no simulation state itself.
struct PresenceReportCommand
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::PresenceReport);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 3; // the 3-byte header

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

// How often a client that keeps receiving input reports it: well inside mvp.presence.idle_minutes, so that a steward who
// plays is never away, and seldom enough to cost nothing on the wire.
inline constexpr double PRESENCE_REPORT_SECONDS = 15.0;

/// MVP-074 (ADR-187): a request for the away report of the settlement of `team`, which the server answers after the next
/// tick, as it answers a status request (MVP-098).
struct ReadAwayReportCommand
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::ReadAwayReport);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 4; // the 3-byte header and the team

  std::uint8_t team = 0;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

// MVP-074 (ADR-187): the most lines of each kind an away report carries; the deputy's actions beyond its bound are
// counted, as the absence counts those beyond its own.
inline constexpr std::uint64_t AWAY_BATTLES_MAX = 64;
inline constexpr std::uint64_t AWAY_GRIEVANCES_MAX = 256;
inline constexpr std::uint64_t AWAY_CLAIMS_MAX = 64;
inline constexpr std::uint64_t AWAY_FREEZES_MAX = 64;
inline constexpr std::uint64_t AWAY_ACTIONS_MAX = 1'024;
// The structures a settlement completes, by kind: the dwelling, the storehouse and the turret (MVP-034).
inline constexpr std::size_t AWAY_STRUCTURE_KINDS = 3;

/// A battle of the absence that the settlement was a side of (MVP-048): the other side, whether the settlement defended
/// and won, how the battle ended and when, the settlement's Workers lost, the stores it gained (above 0) or lost (below),
/// and whether a claimed area moved.
struct AwayBattleLine
{
  std::uint8_t other = 0;
  bool defended = false;
  bool won = false;
  BattleEnd end = BattleEnd::ObjectiveHeld;
  std::uint64_t endTick = 0;
  std::uint32_t workersLost = 0;
  std::int64_t storesMilliRu = 0;
  bool areaMoved = false;

  friend constexpr bool operator==(const AwayBattleLine& _a, const AwayBattleLine& _b) noexcept = default;
};

/// A grievance recorded in the absence, held by the Cladan at site `holder` against the settlement at site `against`
/// (MVP-051).
struct AwayGrievanceLine
{
  std::uint64_t tick = 0;
  std::uint8_t holder = 0;
  std::uint8_t against = 0;
  GrievanceCause cause = GrievanceCause::Pressure;

  friend constexpr bool operator==(const AwayGrievanceLine& _a, const AwayGrievanceLine& _b) noexcept = default;
};

/// An area that moved in the absence (MVP-053): a half of a valley border area from its holder to a claimant, or the
/// central resource site to the settlement that held it, from nobody.
struct AwayClaimLine
{
  std::uint64_t tick = 0;
  std::uint8_t area = 0;
  std::optional<std::uint8_t> from;
  std::uint8_t to = 0;

  friend constexpr bool operator==(const AwayClaimLine& _a, const AwayClaimLine& _b) noexcept = default;
};

/// A freeze of the world in the absence (PIL-131): its world time, its cause and how long it lasted, 0 when unknown.
struct AwayFreezeLine
{
  std::uint64_t tick = 0;
  FreezeCause cause = FreezeCause::ServerStall;
  std::uint64_t seconds = 0;

  friend constexpr bool operator==(const AwayFreezeLine& _a, const AwayFreezeLine& _b) noexcept = default;
};

/// A deputy action (MVP-073, PIL-026): its tick, what it did and the standing order that permitted it, the Leader it
/// ordered, and the other side of the battle.
struct AwayActionLine
{
  std::uint64_t tick = 0;
  DeputyActionKind kind = DeputyActionKind::LeaderToObjective;
  StandingOrder order = StandingOrder::DefendTheSettlement;
  StableId leader = 0;
  std::uint8_t other = 0;

  friend constexpr bool operator==(const AwayActionLine& _a, const AwayActionLine& _b) noexcept = default;
};

/// MVP-074 and PIL-073 (ADR-187): the away report of the settlement of `team` at `tick`: its steward's latest absence,
/// from the tick they went away to the tick they came back, with its summary and every deputy action with its standing
/// order. `absence` is false when its steward was never away, and the report holds nothing else. `onReturn` marks the
/// report the server sends unasked at a return whose absence has news (O110).
struct AwayReport
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::AwayReport);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = Neuron::MESSAGE_MAX_BYTES;

  std::uint64_t tick = 0;
  std::uint8_t team = 0;
  bool absence = false;
  bool onReturn = false;
  std::uint64_t fromTick = 0;
  std::uint64_t toTick = 0;
  std::uint32_t workersFrom = 0;
  std::uint32_t workersTo = 0;
  std::int64_t storesFromMilliRu = 0;
  std::int64_t storesToMilliRu = 0;
  std::array<std::uint32_t, AWAY_STRUCTURE_KINDS> structuresCompleted{}; // dwellings, storehouses, turrets
  std::vector<AwayBattleLine> battles;
  std::vector<AwayGrievanceLine> grievances;
  std::vector<AwayClaimLine> claims;
  std::vector<AwayFreezeLine> freezes;
  std::vector<AwayActionLine> actions;
  std::uint32_t grievancesTotal = 0; // those beyond the lines included
  std::uint32_t actionsTotal = 0;

  // O110: the absence has news: a battle, a deputy action, a freeze, a grievance, a claim moved or a structure finished.
  [[nodiscard]] bool HasNews() const noexcept;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);

  friend bool operator==(const AwayReport& _a, const AwayReport& _b) = default;
};

} // namespace Species
