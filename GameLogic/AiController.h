#pragma once

#include "BitReader.h"
#include "BitWriter.h"
#include "StableId.h"

#include <cstdint>
#include <map>

namespace Species
{

class RegionSimulation;

/// An AI that commands one team of a region (MVP-049, MVP-058 at M1, ADR-107): the reference easy bot's formation
/// Leaders and Leader policy, its dispatch with random target selection and its stray recall (O70). It reads the whole
/// region (R-BOT-017) at the end of each tick and acts only through the region's command queue as its team's AI
/// commander, so what it decides executes at the next tick (ADR-087). Region state.
struct AiController
{
  StableId id = 0; // its sender id, from the region's counter
  std::uint8_t team = 0;
  std::int32_t cycleTicks = 0;                   // R-BOT-074: the main-cycle countdown
  std::uint64_t nextSequence = 0;                // its commands' sequence numbers (ADR-095)
  std::map<StableId, std::int32_t> leaderTimers; // R-UNIT-188: each live Leader's policy countdown, by its id
};

// R-BOT-009: an AI of _team whose first main cycle is max(1, ⌈NextUnit(ref.bot.cycle.phase_easy)⌉) ticks away; it
// makes no Beam Dish draw, since the MVP has none.
[[nodiscard]] AiController MakeAi(RegionSimulation& _region, StableId _id, std::uint8_t _team);

// R-BOT-074 at the end of the region's tick: the stray recall, the main-cycle countdown, the Leader policy of each of
// its team's Leaders, and the main cycle when the countdown is out.
void StepAi(RegionSimulation& _region, AiController& _ai);

// The AI canonically (ADR-081 item 7), its Leader timers in id order, and the values the writer wrote; false when they
// do not read whole.
void WriteAi(Neuron::BitWriter& _writer, const AiController& _ai);
[[nodiscard]] bool ReadAi(Neuron::BitReader& _reader, AiController& _ai);

} // namespace Species
