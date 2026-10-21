#pragma once

#include <cstdint>
#include <optional>

namespace Species
{

class RegionSimulation;

// The sandbox's two sides (M1 plan §6): the steward commands the first, an AI the second.
inline constexpr std::uint8_t SANDBOX_STEWARD_TEAM = 0;
inline constexpr std::uint8_t SANDBOX_AI_TEAM = 1;

// M1 plan §6: a new sandbox in a region with its terrain: the steward's side and the AI's side, each
// mvp.sandbox.side_workers Workers with no Leader spawned around a site centre (mvp.sandbox.steward_site and
// mvp.sandbox.ai_site, diagonally opposite), and the AI that commands its side (ADR-107). A watched sandbox (O77) gives
// the steward's side an AI too. Called between ticks.
void PopulateSandbox(RegionSimulation& _region, bool _watched = false);

// ADR-080 item 2's synthetic battle: _figures Workers, mvp.perf.synthetic_figures when none is given, in two halves
// mvp.perf.synthetic_gap apart across the valley's centre, each spread as a sandbox side and commanded by an AI
// (ADR-110); the frame bench's scene of mvp.client.figures_max figures is one (O79, ADR-126). Called between ticks, on a
// region with its terrain.
void PopulateSyntheticBattle(RegionSimulation& _region, std::optional<std::uint32_t> _figures = std::nullopt);

} // namespace Species
