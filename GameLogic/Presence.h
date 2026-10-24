#pragma once

#include "BitReader.h"
#include "BitWriter.h"
#include "StableId.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace Species
{

class RegionSimulation;
class WorldSimulation;

// A region of state version 19 and later holds its absences (M4 task T3, ADR-181).
inline constexpr std::uint32_t STATE_VERSION_WITH_ABSENCES = 19;

/// PIL-079 (MVP-073): the default orders a steward who sets none leaves the deputy, by which each deputy action is
/// permitted (PIL-026). A state value that is never renumbered.
enum class StandingOrder : std::uint8_t
{
  DefendTheSettlement = 0,
  KeepGathering = 1,
  FinishTheQueuedConstruction = 2,
  NeverAttack = 3
};

// The bound a standing order is written with, which leaves room for the orders a later stage adds (PIL-083).
inline constexpr std::uint64_t STANDING_ORDER_MAX = 15;

/// MVP-073 (ADR-182): what a deputy action did. A state value that is never renumbered.
enum class DeputyActionKind : std::uint8_t
{
  LeaderToObjective = 0 // sent a Leader's formation to its battle's objective; the detail is the battle's place
};

inline constexpr std::uint64_t DEPUTY_ACTION_KIND_MAX = 15;

/// One action of the deputy (MVP-073, PIL-026): its tick, what it did, the standing order that permitted it, the Leader
/// it ordered by stable id, and a detail its kind names.
struct DeputyAction
{
  std::uint64_t tick = 0;
  DeputyActionKind kind = DeputyActionKind::LeaderToObjective;
  StandingOrder order = StandingOrder::DefendTheSettlement;
  StableId leader = 0;
  std::uint32_t detail = 0;

  friend constexpr bool operator==(const DeputyAction& _a, const DeputyAction& _b) noexcept = default;
};

// The most deputy actions an absence keeps; the ones beyond are counted, not kept (MVP-074's report lists what it keeps).
inline constexpr std::uint64_t DEPUTY_ACTIONS_MAX = 4'096;

/// MVP-072 to MVP-074 (ADR-181): one absence of the steward of the settlement of team `team`, from the tick its steward
/// went away to the tick they came back, none while it lasts; the settlement's Workers and stores at either end, and the
/// deputy's actions in it in the order they were taken. A region keeps each stewarded settlement's open absence and its
/// latest closed one, so that its away report can be asked for again (O109).
struct Absence
{
  std::uint8_t team = 0;
  std::uint64_t fromTick = 0;
  std::optional<std::uint64_t> toTick;
  std::uint32_t workersFrom = 0;
  std::int64_t storesFromMilliRu = 0;
  std::uint32_t workersTo = 0;
  std::int64_t storesToMilliRu = 0;
  std::vector<DeputyAction> actions;
  std::uint32_t actionsDropped = 0; // beyond DEPUTY_ACTIONS_MAX

  friend bool operator==(const Absence& _a, const Absence& _b) = default;
};

// The most absences a region keeps: an open and a closed one for each of its settlements.
inline constexpr std::uint64_t ABSENCES_MAX = 8;

// The absence canonically (ADR-081 item 7); false when it does not read whole.
void WriteAbsence(Neuron::BitWriter& _writer, const Absence& _absence);
[[nodiscard]] bool ReadAbsence(Neuron::BitReader& _reader, Absence& _absence);

// MVP-072: the open absence of the settlement of _team, while its steward is away; null while they are present.
[[nodiscard]] const Absence* OpenAbsence(const RegionSimulation& _region, std::uint8_t _team) noexcept;
[[nodiscard]] Absence* OpenAbsence(RegionSimulation& _region, std::uint8_t _team) noexcept;
// O109: the latest closed absence of the settlement of _team; null before its steward has come back once.
[[nodiscard]] const Absence* LatestAbsence(const RegionSimulation& _region, std::uint8_t _team) noexcept;

// MVP-072 (ADR-181): the steward is away from now on, or present again, for every settlement a steward runs in the
// world. Going away opens each settlement's absence at the world's next tick with its Workers and stores; coming back
// closes it there with them, and it replaces the settlement's previous closed absence. Called between ticks, as a
// connection is (ADR-095), by the world server and by the journal's replay. False when nothing changed.
bool SetStewardPresence(WorldSimulation& _world, bool _away);
// Whether the steward is away from a settlement of the world: any stewarded settlement has an open absence.
[[nodiscard]] bool StewardAway(const WorldSimulation& _world) noexcept;

// MVP-073 (ADR-182): records a deputy action in the open absence of _team's settlement; nothing while it has none.
void RecordDeputyAction(RegionSimulation& _region, std::uint8_t _team, const DeputyAction& _action);

} // namespace Species
