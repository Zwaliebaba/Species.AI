#pragma once

#include "MessageCodec.h"

namespace Species
{

// The type ids of the game's messages (ADR-096), from the first id the engine leaves to the game. An id is never reused
// for another message, and a removed message keeps its id unused.
enum class GameMessageType : Neuron::MessageTypeId
{
  TickReport = Neuron::GAME_MESSAGE_TYPE_FIRST,
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
  AiLeaderFields
};

[[nodiscard]] constexpr Neuron::MessageTypeId TypeIdOf(GameMessageType _type) noexcept
{
  return static_cast<Neuron::MessageTypeId>(_type);
}

} // namespace Species
