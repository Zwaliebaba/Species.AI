#pragma once

namespace Species
{

class RegionSimulation;

// ADR-106: registers the battle commands of M1 task T7 on the region's command queue (ADR-087 item 1). On arrival each
// is permitted when its sender commands the team it names, and an AI command only when that commander is an AI
// (DEC-122). On execution each is checked again against the tick's state, and a figure that no longer qualifies makes it
// do nothing (R-SIM-453, ADR-087 item 3).
void RegisterRegionCommands(RegionSimulation& _region);

} // namespace Species
