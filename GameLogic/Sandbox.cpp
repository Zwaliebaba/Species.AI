#include "pch.h"

#include "RegionSimulation.h"
#include "Sandbox.h"

namespace Species
{

namespace
{

// _count Workers of _team around the centre of site _site, with no Leader.
void SpawnSide(RegionSimulation& _region, std::uint8_t _team, std::int64_t _site, std::uint32_t _count)
{
  const RegionTerrain& terrain = *_region.Terrain();
  RegionSimulation::Spawn spawn;
  spawn.team = _team;
  spawn.kind = FigureKind::Worker;
  spawn.position = terrain.layout.sites[static_cast<std::size_t>(_site) % terrain.layout.sites.size()];
  spawn.position.y = terrain.field.Height(spawn.position.x, spawn.position.z);
  spawn.count = _count;
  spawn.spread = _region.Tuning().mvp.sandbox.sideSpread;
  static_cast<void>(_region.SpawnFigures(spawn));
}

} // namespace

void PopulateSandbox(RegionSimulation& _region)
{
  const auto& sandbox = _region.Tuning().mvp.sandbox;
  const auto workers = static_cast<std::uint32_t>(sandbox.sideWorkers);
  SpawnSide(_region, SANDBOX_STEWARD_TEAM, sandbox.stewardSite, workers);
  SpawnSide(_region, SANDBOX_AI_TEAM, sandbox.aiSite, workers);
  static_cast<void>(_region.AddAi(SANDBOX_AI_TEAM));
}

void PopulateSyntheticBattle(RegionSimulation& _region)
{
  const auto& tuning = _region.Tuning();
  const auto half = static_cast<std::uint32_t>(tuning.mvp.perf.syntheticFigures / 2);
  SpawnSide(_region, SANDBOX_STEWARD_TEAM, tuning.mvp.sandbox.stewardSite, half);
  SpawnSide(_region, SANDBOX_AI_TEAM, tuning.mvp.sandbox.aiSite, half);
  static_cast<void>(_region.AddAi(SANDBOX_STEWARD_TEAM));
  static_cast<void>(_region.AddAi(SANDBOX_AI_TEAM));
}

} // namespace Species
