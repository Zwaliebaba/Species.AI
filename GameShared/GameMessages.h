#pragma once

#include "MessageCodec.h"

namespace Species
{

// The type ids of the game's messages (ADR-096), from the first id the engine leaves to the game. An id is never reused
// for another message, and a removed message keeps its id unused.
enum class GameMessageType : Neuron::MessageTypeId
{
  RetiredTickReport = Neuron::GAME_MESSAGE_TYPE_FIRST, // M0's tick report, which replication replaced at M1 (ADR-111)
  WorldTerrain,
  CommandReport, // ADR-106: the outcome of a command, to its sender
  // The battle commands of a region (M1 task T7, ADR-106), each in the reference's player command catalogue (R-SIM-450).
  Select,
  PromoteWorker,
  DemoteLeader,
  OrderUnit,
  ToggleUnit,
  SelectWorkers,
  OrderWorkers,
  ControlState,
  // The AI's own actions, which only an AI sender may send (DEC-122, R-BOT-015).
  AiOrderWorker,
  AiOrderLeader,
  AiLeaderFields,
  // A development command of the M1 sandbox (M1 plan §6, ADR-109).
  SandboxRestart,
  // Replication (M1 task T12, ADR-111): the state datagrams and the droppable presentation events of a tick.
  StateUpdate,
  PresentationEvents,
  // The server's tick statistics for the development overlay, Debug builds only (M1 task T15b, ADR-120).
  ServerStats,
  // The settlement's commands (M2 task T8, ADR-136), and the request for its status and the status itself (MVP-098).
  PlaceStructure,
  RebuildTurret,
  SetPriorities,
  ReadSettlementStatus,
  SettlementStatus,
  // A guaranteed event for the HUD's notifications, on the reliable channel (M2 task T11, ADR-139).
  SettlementNotice,
  // The relations' commands (M3 task T5, ADR-160).
  MakeClaim,
  WithdrawClaim,
  SendTribute,
  // The attacks (M3 task T6, ADR-161).
  DeclareAttack,
  WithdrawAttack,
  // The valley's status and the battle results (M3 task T10, ADR-165).
  ReadValleyStatus,
  ValleyStatus,
  BattleResultNotice,
  // The run's epilogue (M3 task T14, ADR-169).
  ReadEpilogue,
  Epilogue,
  // The steward's growth hold (O106, ADR-178).
  HoldGrowth,
  // Away play (M4): the client's report of its steward's input (ADR-181).
  PresenceReport
};

[[nodiscard]] constexpr Neuron::MessageTypeId TypeIdOf(GameMessageType _type) noexcept
{
  return static_cast<Neuron::MessageTypeId>(_type);
}

} // namespace Species
