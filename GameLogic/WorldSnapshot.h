#pragma once

#include "Snapshot.h"
#include "WorldSimulation.h"

#include <cstdint>
#include <expected>
#include <memory>

namespace Species
{

// ADR-099, MVP-095: the version of the world's state as a snapshot holds it. A change to any owner's state layout
// raises it, and adds the step that migrates the previous version; a version below the oldest that a step reaches is
// refused.
inline constexpr std::uint32_t WORLD_STATE_VERSION = 23;
inline constexpr std::uint32_t WORLD_STATE_OLDEST_VERSION = 1;
// ADR-137: the first version a server saves, which holds the world record after the regions; no world of an earlier
// version was ever saved, and their steps stay with their tests.
inline constexpr std::uint32_t WORLD_STATE_FIRST_SAVED_VERSION = 9;

// The world's whole state: its simulation key, its region count, the world owner's state and each region's, then the
// world record (ADR-137).
[[nodiscard]] Neuron::SnapshotContents SnapshotOf(const WorldSimulation& _world);

// The world a snapshot holds, migrated to WORLD_STATE_VERSION, or why it is refused; never a world in part. Version 2
// added each region's teams and figures (M1 task T2), and a version-1 region migrates to one with no teams; version 3
// added the world seed (M1 task T3), and an older world migrates to seed 0. The terrain is not state: the world server
// builds it again from the seed. Version 4 added each figure's movement state and each region's actor-grid history and
// cache (M1 task T4); an older figure stands Idle where it was, and the grid registers it there. Version 5 added each
// region's routes and each figure's route (M1 task T5); an older region has none. Version 6 added each Worker's threat
// and grenade timer, each team's kills, and each region's laser bolts, effects and Essence orbs (M1 task T6); an older
// region has none, and its Workers no threat. Version 7 added each region's commanders and each team's selection and
// pointer, each Worker's Leader and each Leader's orders and formation, and the order markers' routes, and dropped the
// figure's formation flag (M1 task T7); an older region has no commander, and its figures no Leader and no order.
// Version 8 added each region's AIs and its bot navigation nodes' counts, and each Worker's nearest node (M1 task T8);
// an older region has no AI and makes its nodes as a new one does. Version 9 added each region's settlements and its
// structures (M2 task T4), and each figure's port (M2 task T5); an older region has neither, and its figures no port.
// Version 9 is the first saved, and holds the world record after the regions (ADR-137). Version 10 added each region's
// scripted stewards (M2 task T10, ADR-138); an older region has none. Version 11 added each region's grievances (M3
// task T4, ADR-159); an older region has none, so the owner's M2 world continues (O98). Version 12 added each region's
// claims and moved areas, and territory events of more than one kind (M3 task T5, ADR-160); an older region has
// neither, and its events are contacts. Version 13 added each region's expeditions, battles, battle results and
// respites (M3 task T6, ADR-161); an older region has none. Version 14 added each region's AI settlements' decisions (M3
// task T7, ADR-162); an older region makes them afresh. Version 15 added the world's run, each region's renown, and
// statistics of more kinds (M3 task T8, ADR-163); an older world's run begins where it continues, and its renown at none.
// Version 16 added the tick each area's last claim ended (O102, DEC-1008, ADR-160); an older region has none, so no area
// cools down. Version 17 added whether each settlement's growth is held (O106, ADR-178); an older region holds none.
// Version 18 added the world's pace beside its region count (M4 task T2, ADR-184); an older world runs at the session
// pace. Version 19 added each region's absences (M4 task T3, ADR-185); an older region has none, its steward present.
// Version 20 added each region's deputies (M4 task T4, ADR-186); an older region makes each at its steward's next absence.
// Version 21 added each absence's grievances (M4 task T5, ADR-187); an older absence has none.
// Version 22 added the freeze causes of MVP-071 to the world record, a cause in two bits (M4 task T6, ADR-188).
// Version 23 added each gatherer's home port (O112, ADR-195); an older region has none.
// _tuning and _structures outlive the world.
[[nodiscard]] std::expected<std::unique_ptr<WorldSimulation>, Neuron::SnapshotError>
RestoreWorld(const Neuron::SnapshotContents& _snapshot, const TuningMvp& _tuning, const StructureRecords* _structures = nullptr);

} // namespace Species
