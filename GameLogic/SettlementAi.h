#pragma once

#include "AiController.h"
#include "BitReader.h"
#include "BitWriter.h"
#include "Settlement.h"
#include "StableId.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace Species
{

class RegionSimulation;
struct ScriptedSteward;
struct TuningMvp;

// The first state version whose regions hold their AI settlements' decisions (M3 task T7, ADR-162).
inline constexpr std::uint32_t STATE_VERSION_WITH_SETTLEMENT_AI = 14;

/// MVP-041 and MVP-055 (ADR-162): an attack an AI settlement is mustering: its target and the claim it settles (0 for a
/// raid), the Workers it means to send and the fewest it may, when it began, how many Leaders it wants and those it has.
struct Muster
{
  std::uint8_t target = 0;
  StableId claim = 0;
  std::uint32_t size = 0; // the Workers it can field (MVP-055)
  std::uint32_t need = 0; // mvp.ai.attack_min_share of the target's Workers it sees, at least 1
  std::uint64_t startTick = 0;
  std::uint8_t leadersWanted = 0;
  std::vector<StableId> leaders; // in the order adopted, at most EXPEDITION_LEADERS_MAX

  bool operator==(const Muster&) const = default;
};

/// MVP-055 to MVP-058 (ADR-162): the decisions of the AI settlement of team `team`, which its scripted steward sends
/// as the settlement's AI commander: its decision countdown, the muster of its next attack, and the reference bot's
/// battle core (ADR-107) for its settlement's side while it defends and for its expedition while its battle is in
/// progress. Region state from version 14.
struct SettlementAi
{
  std::uint8_t team = 0;
  std::int32_t decisionTicks = 1; // to its next decision, every mvp.ai.decision_ticks
  std::optional<Muster> muster;
  bool defending = false;
  AiController defense; // its id is the scripted steward's, its team the settlement's
  bool attacking = false;
  AiController attack; // its id is the scripted steward's, its team the expedition's

  bool operator==(const SettlementAi& _other) const;
};

// The decisions of the AI settlement that _steward runs, whose expedition is team _expeditionTeam: its battle cores sent as
// the steward, its first decision at its first step.
[[nodiscard]] SettlementAi MakeSettlementAi(const ScriptedSteward& _steward, std::uint8_t _expeditionTeam);

// MVP-057: the anger at which a Cladan of _temperament may attack, in units of grievance weight; none for no temperament.
[[nodiscard]] std::optional<double> AngerThresholdOf(const TuningMvp& _tuning, Temperament _temperament) noexcept;

// MVP-055: the living Workers of the settlement at _target, its expedition's among them, that the settlement at
// _viewer sees (MVP-016).
[[nodiscard]] std::uint32_t SeenWorkersOf(const RegionSimulation& _region, std::uint8_t _viewer, std::uint8_t _target);
// MVP-055 (ADR-162): the Workers the settlement can field: its living Workers beyond mvp.protection.worker_floor, at
// most mvp.expedition.size_max.
[[nodiscard]] std::uint32_t FieldableWorkers(const RegionSimulation& _region, const Settlement& _settlement);

// At the end of the region's tick, after the scripted steward's own look, for the AI settlement it runs (ADR-162):
// - each tick, the battle core of its settlement's side while it defends a battle, and of its expedition while that
//   expedition's battle is in progress, walking to the battle's objective (MVP-049, MVP-058);
// - every mvp.ai.decision_ticks, its decision: its Leaders outside a muster and a defense demoted; a claim by its
//   temperament (MVP-057, DEC-1006); then its muster's next step, or a new muster against the settlement it is angriest
//   with, or whose area it claims, while it can field enough of an expedition (MVP-055).
// Each command is sent as the steward and executes at the next tick (MVP-094).
void StepSettlementAi(RegionSimulation& _region, ScriptedSteward& _steward, SettlementAi& _ai);

// The AI settlement's decisions canonically (ADR-081 item 7), and the values the writer wrote; false when they do not
// read whole.
void WriteSettlementAi(Neuron::BitWriter& _writer, const SettlementAi& _ai);
[[nodiscard]] bool ReadSettlementAi(Neuron::BitReader& _reader, SettlementAi& _ai);

} // namespace Species
