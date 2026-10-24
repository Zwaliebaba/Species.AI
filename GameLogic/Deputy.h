#pragma once

#include "BitReader.h"
#include "BitWriter.h"
#include "CommandQueue.h"
#include "StableId.h"

#include <cstdint>

namespace Species
{

class RegionSimulation;

// A region of state version 20 and later holds its deputies (M4 task T4, ADR-182).
inline constexpr std::uint32_t STATE_VERSION_WITH_DEPUTIES = 20;

/// MVP-073 (ADR-182): the deputy of the stewarded settlement of team `team`, made the first time its steward goes away
/// and kept: its sender id, a stable id of the region, and its commands' sequence numbers. It commands only while its
/// settlement's steward is away (PIL-071), and only AI commands for its Leaders.
struct Deputy
{
  std::uint8_t team = 0;
  StableId id = 0;
  std::uint64_t nextSequence = 0;

  friend constexpr bool operator==(const Deputy& _a, const Deputy& _b) noexcept = default;
};

// The deputy of the settlement of _team, or null before its steward was first away.
[[nodiscard]] const Deputy* DeputyOf(const RegionSimulation& _region, std::uint8_t _team) noexcept;

// MVP-073 (ADR-182): _sender is the deputy of a settlement whose steward is away, and _team is that settlement's or its
// expedition's team (PIL-043).
[[nodiscard]] bool DeputyMayCommand(const RegionSimulation& _region, Neuron::SenderId _sender, std::uint8_t _team) noexcept;

// At the end of the region's tick, after its AIs and scripted stewards (ADR-182): while its settlement's steward is
// away, each tick, each live Leader of each side its settlement has in a battle in progress, the
// settlement's own as a defender and its expedition's as an attacker (O109), that is not yet in formation on its way to
// the battle's objective is sent there in formation, each such order a deputy action of its absence with the standing
// order that permits it (PIL-026). It sends nothing else: it never declares, withdraws, claims, pays tribute, places,
// queues, promotes, demotes or changes the priorities (MVP-073, PIL-083). Its commands execute at the next tick.
void StepDeputy(RegionSimulation& _region, Deputy& _deputy);

// The deputy canonically (ADR-081 item 7); false when it does not read whole.
void WriteDeputy(Neuron::BitWriter& _writer, const Deputy& _deputy);
[[nodiscard]] bool ReadDeputy(Neuron::BitReader& _reader, Deputy& _deputy);

} // namespace Species
