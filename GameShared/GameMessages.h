#pragma once

#include "MessageCodec.h"

namespace Species
{

// The type ids of the game's messages (ADR-096), from the first id the engine leaves to the game. An id is never reused
// for another message, and a removed message keeps its id unused.
enum class GameMessageType : Neuron::MessageTypeId
{
  TickReport = Neuron::GAME_MESSAGE_TYPE_FIRST,
  WorldTerrain
};

[[nodiscard]] constexpr Neuron::MessageTypeId TypeIdOf(GameMessageType _type) noexcept
{
  return static_cast<Neuron::MessageTypeId>(_type);
}

} // namespace Species
