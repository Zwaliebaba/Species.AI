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
  PresentationEvents
};

[[nodiscard]] constexpr Neuron::MessageTypeId TypeIdOf(GameMessageType _type) noexcept
{
  return static_cast<Neuron::MessageTypeId>(_type);
}

} // namespace Species
