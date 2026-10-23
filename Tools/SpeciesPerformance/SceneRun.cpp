#include "pch.h"

#include "RegionSimulation.h"
#include "SceneRun.h"
#include "WorkerPool.h"

#include <utility>

namespace Performance
{

std::uint32_t FiguresOf(const Species::WorldSimulation& _world)
{
  std::uint32_t figures = 0;
  for (std::size_t region = 0; region < _world.RegionCount(); ++region)
  {
    const Species::RegionSimulation& simulation = _world.Region(region);
    for (std::size_t team = 0; team < simulation.TeamCount(); ++team)
    {
      figures += simulation.TeamAt(team).figures.Count();
    }
  }
  return figures;
}

SceneRun MeasureWorld(std::string _name, Species::WorldSimulation& _world, std::uint64_t _ticks,
                      const std::function<void(Species::WorldSimulation&, std::uint64_t)>& _beforeTick)
{
  SceneRun run;
  run.name = std::move(_name);
  run.ticks = _ticks;
  run.firstFigures = FiguresOf(_world);
  for (std::size_t region = 0; region < _world.RegionCount(); ++region)
  {
    const Species::RegionSimulation& simulation = _world.Region(region);
    run.ais += simulation.Ais().size() + simulation.SettlementAis().size(); // M1's AIs and M3's AI settlements (ADR-164)
    for (std::size_t team = 0; team < simulation.TeamCount(); ++team)
    {
      run.sides += simulation.TeamAt(team).figures.Count() > 0 ? std::size_t{1} : std::size_t{0};
    }
  }
  run.stepNanoseconds.reserve(_ticks);
  Neuron::WorkerPool serial(0);
  double figureSum = 0.0;
  static_cast<void>(Neuron::TakeMarkerTimings());
  Neuron::SetMarkerTiming(true);
  const std::uint64_t first = _world.NextTick();
  for (std::uint64_t tick = first; tick < first + _ticks; ++tick)
  {
    if (_beforeTick)
    {
      _beforeTick(_world, tick);
    }
    const std::int64_t start = Neuron::MarkerClockNanoseconds();
    _world.Step(tick, serial);
    run.stepNanoseconds.push_back(Neuron::MarkerClockNanoseconds() - start);
    figureSum += static_cast<double>(FiguresOf(_world));
  }
  Neuron::SetMarkerTiming(false);
  run.markers = Neuron::TakeMarkerTimings();
  run.lastFigures = FiguresOf(_world);
  run.meanFigures = _ticks == 0 ? 0.0 : figureSum / static_cast<double>(_ticks);
  return run;
}

} // namespace Performance
