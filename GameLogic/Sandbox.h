#pragma once

#include <cstdint>

namespace Species
{

class RegionSimulation;

// The sandbox's two sides (M1 plan §6): the steward commands the first, an AI the second.
inline constexpr std::uint8_t SANDBOX_STEWARD_TEAM = 0;
inline constexpr std::uint8_t SANDBOX_AI_TEAM = 1;

// M1 plan §6: a new sandbox in a region with its terrain: the steward's side and the AI's side, each
// mvp.sandbox.side_workers Workers with no Leader spawned around a site centre (mvp.sandbox.steward_site and
// mvp.sandbox.ai_site, diagonally opposite), and the AI that commands its side (ADR-107). Called between ticks.
void PopulateSandbox(RegionSimulation& _region);

// ADR-080 item 2's synthetic battle: mvp.perf.synthetic_figures Workers in two halves mvp.perf.synthetic_gap apart
// across the valley's centre, each spread as a sandbox side and commanded by an AI (ADR-110). Called between ticks, on
// a region with its terrain.
void PopulateSyntheticBattle(RegionSimulation& _region);

} // namespace Species
