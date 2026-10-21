#include "pch.h"

#include "GenerationScene.h"
#include "MarkerTimings.h"
#include "Navigation.h"
#include "RegionTerrain.h"

#include <format>
#include <stdexcept>
#include <utility>

namespace Performance
{

std::vector<GenerationSample> MeasureGeneration(std::uint64_t _firstSeed, std::uint64_t _seeds, const Species::TuningMvp& _tuning)
{
  std::vector<GenerationSample> samples;
  for (std::uint64_t seed = _firstSeed; seed < _firstSeed + _seeds; ++seed)
  {
    GenerationSample sample;
    sample.seed = seed;
    const std::int64_t start = Neuron::MarkerClockNanoseconds();
    const auto generated = Species::GenerateRegionTerrain(seed, _tuning);
    sample.generateNanoseconds = Neuron::MarkerClockNanoseconds() - start;
    if (!generated)
    {
      throw std::runtime_error(std::format("seed {} gives no valley", seed));
    }
    sample.passedSeed = generated->seed;
    sample.attempt = generated->attempt;

    // The graph alone, from the nodes that the passing attempt placed: ADR-104's cost of every attempt on both sides.
    std::vector<Species::NavigationNode> nodes = Species::PlaceNavigationNodes(generated->layout, generated->field, _tuning);
    const std::int64_t graphStart = Neuron::MarkerClockNanoseconds();
    const Species::LandRouteGraph graph = Species::LandRouteGraph::Build(std::move(nodes), generated->field, _tuning);
    sample.graphNanoseconds = Neuron::MarkerClockNanoseconds() - graphStart;
    sample.nodes = graph.Nodes().size();
    if (sample.nodes != generated->routes.Nodes().size())
    {
      throw std::runtime_error(std::format("seed {}: the graph built again has {} nodes where the valley's has {}", seed, sample.nodes,
                                           generated->routes.Nodes().size()));
    }
    samples.push_back(sample);
  }
  return samples;
}

} // namespace Performance
