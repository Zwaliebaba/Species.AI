#pragma once

#include "AwayMessages.h"
#include "BitReader.h"
#include "BitWriter.h"
#include "RelationMessages.h"
#include "StableId.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace Species
{

class RegionSimulation;
class WorldSimulation;

// A region of state version 19 and later holds its absences (M4 task T3, ADR-185), and of version 21 and later each
// absence's grievances (M4 task T5, ADR-187).
inline constexpr std::uint32_t STATE_VERSION_WITH_ABSENCES = 19;
inline constexpr std::uint32_t STATE_VERSION_WITH_ABSENCE_GRIEVANCES = 21;

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

/// MVP-074 (ADR-187): a grievance recorded while the absence lasted, held by the Cladan at site `holder` against the
/// settlement at site `against`, one of them the absent steward's: its tick and its cause.
struct AbsenceGrievance
{
  std::uint64_t tick = 0;
  std::uint8_t holder = 0;
  std::uint8_t against = 0;
  GrievanceCause cause = GrievanceCause::Pressure;

  friend constexpr bool operator==(const AbsenceGrievance& _a, const AbsenceGrievance& _b) noexcept = default;
};

// The most grievances an absence keeps; the ones beyond are counted, not kept.
inline constexpr std::uint64_t ABSENCE_GRIEVANCES_MAX = 256;

/// MVP-072 to MVP-074 (ADR-185): one absence of the steward of the settlement of team `team`, from the tick its steward
/// went away to the tick they came back, none while it lasts; the settlement's Workers and stores at either end, the
/// deputy's actions in it in the order they were taken, and the grievances recorded for or against the settlement in it
/// (ADR-187). A region keeps each stewarded settlement's open absence and its
/// latest closed one, so that its away report can be asked for again (O110).
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
  std::vector<AbsenceGrievance> grievances;
  std::uint32_t grievancesDropped = 0; // beyond ABSENCE_GRIEVANCES_MAX

  friend bool operator==(const Absence& _a, const Absence& _b) = default;
};

// The most absences a region keeps: an open and a closed one for each of its settlements.
inline constexpr std::uint64_t ABSENCES_MAX = 8;

// The absence canonically (ADR-081 item 7), and as a region of _stateVersion wrote it; false when it does not read whole.
void WriteAbsence(Neuron::BitWriter& _writer, const Absence& _absence);
[[nodiscard]] bool ReadAbsence(Neuron::BitReader& _reader, Absence& _absence, std::uint32_t _stateVersion);

// MVP-072: the open absence of the settlement of _team, while its steward is away; null while they are present.
[[nodiscard]] const Absence* OpenAbsence(const RegionSimulation& _region, std::uint8_t _team) noexcept;
[[nodiscard]] Absence* OpenAbsence(RegionSimulation& _region, std::uint8_t _team) noexcept;
// O110: the latest closed absence of the settlement of _team; null before its steward has come back once.
[[nodiscard]] const Absence* LatestAbsence(const RegionSimulation& _region, std::uint8_t _team) noexcept;

// MVP-072 (ADR-185): the steward is away from now on, or present again, for every settlement a steward runs in the
// world. Going away opens each settlement's absence at the world's next tick with its Workers and stores; coming back
// closes it there with them, and it replaces the settlement's previous closed absence. Called between ticks, as a
// connection is (ADR-095), by the world server and by the journal's replay. False when nothing changed.
bool SetStewardPresence(WorldSimulation& _world, bool _away);
// Whether the steward is away from a settlement of the world: any stewarded settlement has an open absence.
[[nodiscard]] bool StewardAway(const WorldSimulation& _world) noexcept;

// MVP-073 (ADR-186): records a deputy action in the open absence of _team's settlement; nothing while it has none.
void RecordDeputyAction(RegionSimulation& _region, std::uint8_t _team, const DeputyAction& _action);
// MVP-074 (ADR-187): records a new grievance in the open absence of each of its two settlements that has one.
void RecordAbsenceGrievance(RegionSimulation& _region, const AbsenceGrievance& _grievance);

} // namespace Species
