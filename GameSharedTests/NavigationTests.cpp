#include "pch.h"

#include "Navigation.h"
#include "RegionTerrain.h"
#include "TerrainField.h"
#include "TestSupport.h"
#include "Tuning.h"

#include <cstdint>
#include <format>
#include <utility>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameSharedTests
{

namespace
{

const Species::TuningMvp& Tuning()
{
  static const Species::TuningMvp COMMITTED = []
  {
    const auto loaded = Species::LoadTuningMvp(RepositoryContentRoot());
    Assert::IsTrue(loaded.has_value(), L"the committed tuning record reads whole");
    return loaded.value_or(Species::TuningMvp{});
  }();
  return COMMITTED;
}

// A field of 10 wu cells over [0, _size] at the outside height _base, shaped by _areas only.
Neuron::TerrainField FieldOf(std::int32_t _size, double _base, const std::vector<Neuron::TerrainFlattenArea>& _areas)
{
  Neuron::TerrainDefinition definition;
  definition.cellSize = 10.0;
  definition.worldSizeX = _size;
  definition.worldSizeZ = _size;
  definition.outsideHeight = _base;
  return Neuron::TerrainField::Generate(definition, {}, _areas);
}

Neuron::Vec3 On(const Neuron::TerrainField& _field, double _x, double _z)
{
  return {_x, _field.Height(_x, _z), _z};
}

// R-SIM-339 to R-SIM-341 as the reference states them, with no pruning: every pass offers every entry of every next
// hop, N passes; the oracle the graph's tables must equal, entry by entry and in order.
std::vector<std::vector<Species::LandRouteGraph::Hop>> LiteralTables(const std::vector<Species::NavigationNode>& _nodes,
                                                                     const Neuron::TerrainField& _field)
{
  const std::size_t count = _nodes.size();
  std::vector<std::vector<std::int8_t>> walkable(count, std::vector<std::int8_t>(count, -1));
  std::vector<std::vector<double>> cost(count, std::vector<double>(count, 0.0));
  const auto edge = [&](std::size_t _i, std::size_t _j)
  {
    const std::size_t a = std::min(_i, _j);
    const std::size_t b = std::max(_i, _j);
    if (walkable[a][b] < 0)
    {
      const bool clear = Species::IsPathClear(_nodes[_i].position, _nodes[_j].position, true, false, _field, Tuning());
      walkable[a][b] = clear ? 1 : 0;
      cost[a][b] = clear ? Species::WalkCost(_nodes[_i].position, _nodes[_j].position, _field, Tuning()) : 0.0;
    }
    return walkable[a][b] != 0;
  };
  std::vector<std::vector<Species::LandRouteGraph::Hop>> tables(count);
  std::vector<std::vector<std::int32_t>> slots(count, std::vector<std::int32_t>(count, -1));
  const auto offer = [&](std::size_t _i, std::uint32_t _target, std::uint32_t _next, double _total)
  {
    std::int32_t& slot = slots[_i][_target];
    if (slot < 0)
    {
      slot = static_cast<std::int32_t>(tables[_i].size());
      tables[_i].push_back({_target, _next, _total});
    }
    else if (tables[_i][static_cast<std::size_t>(slot)].total > _total)
    {
      tables[_i][static_cast<std::size_t>(slot)] = {_target, _next, _total};
    }
  };
  for (std::size_t i = 0; i < count; ++i)
  {
    for (std::size_t j = 0; j < count; ++j)
    {
      if (Neuron::Length(_nodes[j].position - _nodes[i].position) < Tuning().ref.sim.route.linkRange && edge(i, j))
      {
        offer(i, static_cast<std::uint32_t>(j), static_cast<std::uint32_t>(j), cost[std::min(i, j)][std::max(i, j)]);
      }
    }
  }
  for (std::size_t pass = 0; pass < count; ++pass)
  {
    for (std::size_t i = 0; i < count; ++i)
    {
      for (std::size_t e = 0; e < tables[i].size(); ++e)
      {
        const std::uint32_t hop = tables[i][e].next;
        const double toHop = cost[std::min<std::size_t>(i, hop)][std::max<std::size_t>(i, hop)];
        for (std::size_t k = 0; k < tables[hop].size(); ++k)
        {
          const Species::LandRouteGraph::Hop entry = tables[hop][k];
          offer(i, entry.target, hop, toHop + entry.total);
        }
      }
    }
  }
  return tables;
}

void ExpectSameTables(const Species::LandRouteGraph& _graph, const std::vector<std::vector<Species::LandRouteGraph::Hop>>& _literal)
{
  for (std::uint32_t node = 0; node < _literal.size(); ++node)
  {
    const std::vector<Species::LandRouteGraph::Hop>& table = _graph.TableOf(node);
    Assert::IsTrue(table.size() == _literal[node].size(), std::format(L"node {}'s table size", node).c_str());
    for (std::size_t n = 0; n < table.size(); ++n)
    {
      const Species::LandRouteGraph::Hop& a = table[n];
      const Species::LandRouteGraph::Hop& b = _literal[node][n];
      Assert::IsTrue(a.target == b.target && a.next == b.next && a.total == b.total,
                     std::format(L"node {}'s entry {}: the same target, next and total", node, n).c_str());
    }
  }
}

} // namespace

TEST_CLASS(NavigationTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RSim336TheSlopeTestIsRelativeToTheStart)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-336")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-335")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RSim336TheSlopeTestIsRelativeToTheStart)
  {
    // AC-SIM-05-06: a ramp that rises 1 wu every 20 wu, a feathered square's band: 960 wu uphill is clear, 1,000 wu is
    // not, and both are clear downhill.
    const Neuron::TerrainField ramp = FieldOf(2000, 10.0, {{1500.0, 60.0, 1000.0, 100.0, Neuron::FlattenOp::Feathered, 1000.0}});
    const Neuron::Vec3 foot = On(ramp, 400.0, 1000.0);
    Assert::IsTrue(foot.y == 10.0 && On(ramp, 1400.0, 1000.0).y == 60.0, L"the ramp's ends");
    Assert::IsTrue(Species::IsPathClear(foot, On(ramp, 1360.0, 1000.0), true, false, ramp, Tuning()), L"960 wu uphill");
    Assert::IsFalse(Species::IsPathClear(foot, On(ramp, 1400.0, 1000.0), true, false, ramp, Tuning()), L"1,000 wu uphill");
    Assert::IsTrue(Species::IsPathClear(On(ramp, 1400.0, 1000.0), foot, true, false, ramp, Tuning()), L"1,000 wu downhill");
    Assert::IsTrue(Species::IsPathClear(On(ramp, 1360.0, 1000.0), foot, true, false, ramp, Tuning()), L"960 wu downhill");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RSim337ShortSegmentsAndWetSamples)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-337")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-338")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RSim337ShortSegmentsAndWetSamples)
  {
    // AC-SIM-05-07: a 15 wu segment between land points is clear; one wet sample is clear for the path test and −1 for
    // the walking cost; two wet samples are not clear.
    const Neuron::TerrainField ditch = FieldOf(1000, 10.0, {{500.0, -5.0, 500.0, 15.0, Neuron::FlattenOp::Absolute, 40.0}});
    // The ditch's banks are cliffs, so the path test runs with its default, no cliff evaluation.
    Assert::IsTrue(Species::IsPathClear({100.0, 10.0, 100.0}, {115.0, 10.0, 100.0}, true, false, ditch, Tuning()), L"15 wu");
    const Neuron::Vec3 from{380.0, 10.0, 500.0};
    const Neuron::Vec3 to{620.0, 10.0, 500.0};
    Assert::IsTrue(Species::IsPathClear(from, to, false, false, ditch, Tuning()), L"one wet sample is clear");
    Assert::IsTrue(Species::WalkCost(from, to, ditch, Tuning()) == -1.0, L"and blocks the walking cost");
    const Neuron::TerrainField wide = FieldOf(1000, 10.0, {{500.0, -5.0, 500.0, 25.0, Neuron::FlattenOp::Absolute, 40.0}});
    Assert::IsFalse(Species::IsPathClear(from, to, false, false, wide, Tuning()), L"two wet samples are not");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RSim340TheWalkabilityMemoIsSymmetric)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-340")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RSim340TheWalkabilityMemoIsSymmetric)
  {
    // AC-SIM-05-08: node 0 at the foot and node 1 at the top of a slope of 70 wu over 100 wu: the path down is clear,
    // the path up is not, and the memo's answer, from node 0, gives neither node an edge to the other.
    const Neuron::TerrainField slope = FieldOf(1000, 10.0, {{400.0, 80.0, 500.0, 50.0, Neuron::FlattenOp::Feathered, 100.0}});
    const Neuron::Vec3 foot = On(slope, 550.0, 500.0);
    const Neuron::Vec3 top = On(slope, 450.0, 500.0);
    Assert::IsTrue(foot.y == 10.0 && top.y == 80.0, L"the slope's ends");
    Assert::IsTrue(Species::IsPathClear(top, foot, true, false, slope, Tuning()), L"down from node 1");
    Assert::IsFalse(Species::IsPathClear(foot, top, true, false, slope, Tuning()), L"up from node 0");
    const Species::LandRouteGraph graph = Species::LandRouteGraph::Build({{foot, true}, {top, true}}, slope, Tuning());
    Assert::IsTrue(graph.HopOf(0, 1) == nullptr && graph.HopOf(1, 0) == nullptr, L"no edge either way");
    Assert::IsTrue(graph.HopOf(0, 0) != nullptr && graph.HopOf(0, 0)->total == 0.0, L"each node reaches itself");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RSim341TheTablesAreTheReferencePropagations)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-341")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-339")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RSim341TheTablesAreTheReferencePropagations)
  {
    // ADR-104: the pruned propagation gives the literal loops' tables, entry by entry, order, next hops and totals
    // included; on a level lattice, where equal-cost paths are everywhere, and on 40 nodes of seed 1's valley.
    const Neuron::TerrainField level = FieldOf(2000, 10.0, {});
    std::vector<Species::NavigationNode> lattice;
    for (int j = 0; j < 6; ++j)
    {
      for (int i = 0; i < 6; ++i)
      {
        lattice.push_back({On(level, 160.0 + 320.0 * i, 160.0 + 320.0 * j), true});
      }
    }
    ExpectSameTables(Species::LandRouteGraph::Build(lattice, level, Tuning()), LiteralTables(lattice, level));
    const auto region = Species::GenerateRegionTerrain(1, Tuning());
    Assert::IsTrue(region.has_value(), L"seed 1's valley");
    std::vector<Species::NavigationNode> nodes(region->routes.Nodes().begin(), region->routes.Nodes().begin() + 40);
    ExpectSameTables(Species::LandRouteGraph::Build(nodes, region->field, Tuning()), LiteralTables(nodes, region->field));
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RSim345TheRoutesShapes)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-345")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-351")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RSim345TheRoutesShapes)
  {
    // AC-SIM-05-09 on level land: a clear corridor and a direct route give [from, to]; a direct route under 50 wu with a
    // clear path is its midpoint alone (DEC-263's stated behaviour).
    const Neuron::TerrainField level = FieldOf(2000, 10.0, {});
    const Species::LandRouteGraph graph =
      Species::LandRouteGraph::Build({{On(level, 500.0, 500.0), true}, {On(level, 900.0, 500.0), true}}, level, Tuning());
    const Neuron::Vec3 from = On(level, 400.0, 400.0);
    const Neuron::Vec3 to = On(level, 1000.0, 600.0);
    Assert::IsTrue(graph.BuildRoute(from, to, false, true, false, level, Tuning()) == std::vector<Neuron::Vec3>{from, to},
                   L"a clear corridor");
    Assert::IsTrue(graph.BuildRoute(from, to, true, true, false, level, Tuning()) == std::vector<Neuron::Vec3>{from, to},
                   L"a direct route");
    const Neuron::Vec3 beside = On(level, 430.0, 400.0);
    const std::vector<Neuron::Vec3> merged = graph.BuildRoute(from, beside, true, true, false, level, Tuning());
    Assert::IsTrue(merged.size() == 1 && merged[0] == Neuron::Vec3{415.0, 10.0, 400.0}, L"under 50 wu: the midpoint alone");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RSim348ARouteGoesRoundWaterThroughTheNodes)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-348")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-349")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-350")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RSim348ARouteGoesRoundWaterThroughTheNodes)
  {
    // A pond on level land between the ends: the corridor and the walking cost are blocked, so the route follows the
    // nodes' next hops round it; every leg is clear and dry, and the smoothing pulls its corners in.
    const Neuron::TerrainField pond = FieldOf(2000, 10.0, {{1000.0, -5.0, 1000.0, 150.0, Neuron::FlattenOp::Absolute, 40.0}});
    std::vector<Species::NavigationNode> lattice;
    for (int j = 0; j < 6; ++j)
    {
      for (int i = 0; i < 6; ++i)
      {
        const Neuron::Vec3 point = On(pond, 160.0 + 320.0 * i, 160.0 + 320.0 * j);
        if (point.y > 0.0)
        {
          lattice.push_back({point, true});
        }
      }
    }
    Assert::IsTrue(lattice.size() == 35, L"one lattice point is in the pond");
    const Species::LandRouteGraph graph = Species::LandRouteGraph::Build(lattice, pond, Tuning());
    const Neuron::Vec3 from = On(pond, 300.0, 1000.0);
    const Neuron::Vec3 to = On(pond, 1700.0, 1000.0);
    Assert::IsTrue(Species::WalkCost(from, to, pond, Tuning()) == -1.0, L"the straight walk is wet");
    const std::vector<Neuron::Vec3> route = graph.BuildRoute(from, to, false, true, false, pond, Tuning());
    const std::vector<Neuron::Vec3> corners = graph.BuildRoute(from, to, false, true, true, pond, Tuning());
    Assert::IsTrue(route.size() > 2 && route.front() == from && route.back() == to,
                   std::format(L"round the pond; {} points", route.size()).c_str());
    for (std::size_t leg = 1; leg < route.size(); ++leg)
    {
      Assert::IsTrue(route[leg].y > 0.0 && Species::IsPathClear(route[leg - 1], route[leg], true, false, pond, Tuning()),
                     L"a clear, dry leg");
    }
    const double walked = Species::WalkingDistance(route, pond, Tuning());
    Assert::IsTrue(walked > 0.0 && walked < Species::WalkingDistance(corners, pond, Tuning()), L"shorter than the nodes' corners");
    Assert::IsTrue(route == graph.BuildRoute(from, to, false, true, false, pond, Tuning()), L"the same route");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp017TheNodesStandOnTheValleysLandInAStableOrder)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-017")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp017TheNodesStandOnTheValleysLandInAStableOrder)
  {
    // The lattice's dry, walkable points, then the site centres, the central resource site and the eight border-area
    // middles; the same nodes on every run.
    const auto region = Species::GenerateRegionTerrain(1, Tuning());
    const auto again = Species::GenerateRegionTerrain(1, Tuning());
    Assert::IsTrue(region.has_value() && again.has_value(), L"seed 1's valley");
    const std::vector<Species::NavigationNode>& nodes = region->routes.Nodes();
    const std::size_t lattice = nodes.size() - 13;
    Assert::IsTrue(lattice > 150 && lattice <= 256, std::format(L"about 200 lattice nodes; {}", lattice).c_str());
    for (std::size_t n = 0; n < lattice; ++n)
    {
      Assert::IsTrue(nodes[n].position.y > 0.0, L"dry");
    }
    for (std::size_t local = 0; local < 4; ++local)
    {
      const Neuron::Vec3& site = region->layout.sites[local];
      Assert::IsTrue(nodes[lattice + local].position.x == site.x && nodes[lattice + local].position.z == site.z, L"the site centres");
    }
    Assert::IsTrue(Neuron::HorizontalDistance(nodes[lattice + 4].position, region->layout.valleyCenter) == 0.0, L"the central site");
    const double fromSite0 = Neuron::HorizontalDistance(nodes[lattice + 5].position, region->layout.sites[0]);
    Assert::IsTrue(std::abs(fromSite0 - 860.0) < 1e-9, L"site 0's half of the lens with site 1, 860 wu out");
    for (std::size_t n = 0; n < nodes.size(); ++n)
    {
      Assert::IsTrue(nodes[n].position == again->routes.Nodes()[n].position, L"the same nodes");
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp040EveryPairOfSitesHasARouteOnEverySeed)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-040")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-345")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp040EveryPairOfSitesHasARouteOnEverySeed)
  {
    // M1 plan T5's done-when: on seeds 1 to 5, a route between every pair of the five site centres that starts and ends
    // at them, or within the merge of a node beside them (R-SIM-350), stays dry, lies on the terrain, and is the same on a
    // second build.
    for (std::uint64_t seed = 1; seed <= 5; ++seed)
    {
      const auto region = Species::GenerateRegionTerrain(seed, Tuning());
      Assert::IsTrue(region.has_value(), L"the valley");
      std::vector<Neuron::Vec3> ends(region->layout.sites.begin(), region->layout.sites.end());
      ends.push_back(region->layout.valleyCenter);
      for (Neuron::Vec3& end : ends)
      {
        end.y = region->field.Height(end.x, end.z);
      }
      for (std::size_t a = 0; a < ends.size(); ++a)
      {
        for (std::size_t b = 0; b < ends.size(); ++b)
        {
          if (a == b)
          {
            continue;
          }
          const std::vector<Neuron::Vec3> route = region->routes.BuildRoute(ends[a], ends[b], false, true, false, region->field, Tuning());
          Assert::IsTrue(route.size() >= 2 && Neuron::HorizontalDistance(route.front(), ends[a]) < 100.0 &&
                           Neuron::HorizontalDistance(route.back(), ends[b]) < 100.0,
                         L"from one centre to the other");
          Assert::IsTrue(Species::WalkingDistance(route, region->field, Tuning()) > 0.0, L"a dry walk");
          for (const Neuron::Vec3& point : route)
          {
            Assert::IsTrue(point.y == region->field.Height(point.x, point.z), L"on the terrain");
          }
          Assert::IsTrue(route == region->routes.BuildRoute(ends[a], ends[b], false, true, false, region->field, Tuning()),
                         L"the same route");
        }
      }
    }
  }
};

} // namespace GameSharedTests
