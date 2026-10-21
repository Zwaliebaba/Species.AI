#include "pch.h"

#include "Recipes.h"
#include "Shapes.h"

#include <format>

namespace MeshKit
{

namespace
{

// The palette of the drafts, in sRGB.
constexpr Color STONE_DARK{98, 96, 92};
constexpr Color STONE_LIGHT{150, 146, 136};
constexpr Color METAL_DARK{60, 64, 72};
constexpr Color METAL_MID{110, 116, 126};
constexpr Color PLASTER{168, 150, 124};
constexpr Color GABLE_END{150, 132, 108};
constexpr Color ROOF_TILE{120, 72, 52};
constexpr Color SLATE{72, 78, 90};
constexpr Color WOOD_DARK{86, 60, 42};
constexpr Color EARTH{104, 90, 70};
constexpr Color EARTH_TOP{120, 104, 82};
constexpr Color CRYSTAL{150, 214, 226};
constexpr Color CRYSTAL_DEEP{96, 170, 190};
constexpr Color PAD{64, 66, 74};
constexpr Color PAD_TOP{84, 86, 96};
constexpr Color PAD_RIM{92, 94, 104};
// A team surface is drawn white and tinted by the Cladan's colour (ADR-020).
constexpr Color TEAM_WHITE{238, 238, 238};

// The Wellspring (MVP-020, MVP-021): the largest and brightest shape of a settlement, a stepped octagonal plinth with a
// pool, three pylons whose caps are its lights, and a floating crystal, its own node, over the pool.
constexpr double WELLSPRING_PLINTH_RADIUS_WU = 30.0;
constexpr double WELLSPRING_PYLON_RADIUS_WU = 17.0;
constexpr double WELLSPRING_PORT_RADIUS_WU = 38.0;
constexpr double WELLSPRING_DOOR_WU = 33.0;

MeshModel Wellspring()
{
  MeshModel model{"Wellspring.cmo", {}, {}, false};
  ModelNode& body = model.Node("wellspring");
  AddFrustum(body, {.base = {0.0, 0.0, 0.0},
                    .sides = 8,
                    .halfStep = true,
                    .baseRadius = WELLSPRING_PLINTH_RADIUS_WU,
                    .topRadius = 27.0,
                    .length = 4.0,
                    .color = STONE_DARK,
                    .capColor = STONE_LIGHT});
  AddFrustum(body, {.base = {0.0, 4.0, 0.0},
                    .sides = 8,
                    .halfStep = true,
                    .baseRadius = 21.0,
                    .topRadius = 20.0,
                    .length = 5.0,
                    .color = STONE_LIGHT,
                    .capColor = STONE_DARK});
  AddFrustum(body, {.base = {0.0, 9.0, 0.0},
                    .sides = 8,
                    .halfStep = true,
                    .baseRadius = 12.0,
                    .topRadius = 13.0,
                    .length = 3.0,
                    .topCap = false,
                    .color = METAL_MID,
                    .capColor = METAL_MID});
  AddFrustum(body, {.base = {0.0, 11.0, 0.0},
                    .sides = 8,
                    .halfStep = true,
                    .baseRadius = 11.5,
                    .topRadius = 11.5,
                    .length = 0.5,
                    .color = TEAM_WHITE,
                    .capColor = TEAM_WHITE,
                    .surface = Surface::Team});
  for (std::uint32_t light = 0; light < 3; ++light)
  {
    const Point foot = Around({0.0, 0.0, 0.0}, WELLSPRING_PYLON_RADIUS_WU, 3 + 4 * light, 12, 9.0);
    AddFrustum(body, {.base = foot,
                      .sides = 4,
                      .halfStep = true,
                      .baseRadius = 2.4,
                      .topRadius = 1.6,
                      .length = 18.0,
                      .color = METAL_DARK,
                      .capColor = METAL_DARK});
    AddFrustum(body, {.base = {foot[0], 27.0, foot[2]},
                      .sides = 4,
                      .halfStep = true,
                      .baseRadius = 2.0,
                      .topRadius = 2.0,
                      .length = 2.0,
                      .baseCap = true,
                      .color = TEAM_WHITE,
                      .capColor = TEAM_WHITE,
                      .surface = Surface::Team});
    model.AddSocket(std::format("light_{:02}", light + 1), "wellspring", {foot[0], 30.0, foot[2]});
  }
  // The arch over the door, on the front (+z).
  AddBox(body, {-4.5, 4.0, 19.0}, {-3.5, 10.0, 22.0}, METAL_MID, Surface::Facet);
  AddBox(body, {3.5, 4.0, 19.0}, {4.5, 10.0, 22.0}, METAL_MID, Surface::Facet);
  AddBox(body, {-4.5, 10.0, 19.0}, {4.5, 11.0, 22.0}, METAL_MID, Surface::Facet, true);

  ModelNode& core = model.Node("wellspring_core");
  AddFrustum(core, {.base = {0.0, 14.0, 0.0},
                    .sides = 4,
                    .halfStep = true,
                    .baseRadius = 0.0,
                    .topRadius = 5.0,
                    .length = 8.0,
                    .topCap = false,
                    .color = TEAM_WHITE,
                    .capColor = TEAM_WHITE,
                    .surface = Surface::Team});
  AddFrustum(core, {.base = {0.0, 22.0, 0.0},
                    .sides = 4,
                    .halfStep = true,
                    .baseRadius = 5.0,
                    .topRadius = 0.0,
                    .length = 9.0,
                    .color = TEAM_WHITE,
                    .capColor = TEAM_WHITE,
                    .surface = Surface::Team});

  model.AddSocket("door", "wellspring", {0.0, 0.0, WELLSPRING_DOOR_WU});
  model.AddSocket("entrance", "wellspring", {0.0, 0.0, -WELLSPRING_DOOR_WU});
  model.AddSocket("essence_link", "wellspring_core", {0.0, 31.0, 0.0});
  // Five ports between the door and the entrance: at 36° + 72° k from +x.
  for (std::uint32_t port = 0; port < 5; ++port)
  {
    model.AddSocket(std::format("port_{:02}", port + 1), "wellspring",
                    Around({0.0, 0.0, 0.0}, WELLSPRING_PORT_RADIUS_WU, 1 + 2 * port, 10, 0.0));
  }
  return model;
}

MeshModel WellspringCollision()
{
  MeshModel model{"WellspringCollision.cmo", {}, {}, true};
  ModelNode& body = model.Node("wellspring");
  AddFrustum(body, {.sides = 8, .halfStep = true, .baseRadius = WELLSPRING_PLINTH_RADIUS_WU, .topRadius = 27.0, .length = 4.0});
  AddFrustum(body, {.base = {0.0, 4.0, 0.0}, .sides = 8, .halfStep = true, .baseRadius = 21.0, .topRadius = 21.0, .length = 25.0});
  ModelNode& core = model.Node("wellspring_core");
  AddFrustum(core,
             {.base = {0.0, 14.0, 0.0}, .sides = 4, .halfStep = true, .baseRadius = 0.0, .topRadius = 5.0, .length = 8.0, .topCap = false});
  AddFrustum(core, {.base = {0.0, 22.0, 0.0}, .sides = 4, .halfStep = true, .baseRadius = 5.0, .topRadius = 0.0, .length = 9.0});
  return model;
}

// The dwelling (MVP-022, MVP-034): a gabled house whose four windows glow in its Cladan's colour, built at six ports.
constexpr double DWELLING_PORT_RADIUS_WU = 18.0;

MeshModel Dwelling()
{
  MeshModel model{"Dwelling.cmo", {}, {}, false};
  ModelNode& house = model.Node("dwelling");
  AddBox(house, {-9.0, 0.0, -7.0}, {9.0, 10.0, 7.0}, PLASTER, Surface::Facet);
  AddGable(house, {-10.0, 10.0, -8.0}, {10.0, 17.0, 8.0}, ROOF_TILE, GABLE_END, Surface::Facet);
  AddBox(house, {4.0, 12.0, -3.0}, {6.0, 19.0, -1.0}, STONE_DARK, Surface::Facet);
  AddBox(house, {-1.5, 0.0, 7.0}, {1.5, 6.0, 7.3}, WOOD_DARK, Surface::Facet);
  std::uint32_t window = 0;
  for (const double z : {7.0, -7.4})
  {
    for (const double x : {-4.5, 4.5})
    {
      AddBox(house, {x - 1.5, 4.0, z}, {x + 1.5, 7.0, z + 0.4}, TEAM_WHITE, Surface::Team, true);
      model.AddSocket(std::format("window_{:02}", ++window), "dwelling", {x, 5.5, z < 0.0 ? -7.4 : 7.4});
    }
  }
  // Six construction ports at 30° + 60° k (MVP-033).
  for (std::uint32_t port = 0; port < 6; ++port)
  {
    model.AddSocket(std::format("port_{:02}", port + 1), "dwelling",
                    Around({0.0, 0.0, 0.0}, DWELLING_PORT_RADIUS_WU, 1 + 2 * port, 12, 0.0));
  }
  return model;
}

MeshModel DwellingCollision()
{
  MeshModel model{"DwellingCollision.cmo", {}, {}, true};
  ModelNode& house = model.Node("dwelling");
  AddBox(house, {-9.0, 0.0, -7.0}, {9.0, 10.0, 7.0}, {}, Surface::Facet);
  AddGable(house, {-10.0, 10.0, -8.0}, {10.0, 17.0, 8.0}, {}, {}, Surface::Facet);
  return model;
}

// The storehouse (MVP-031, MVP-034): a long slate-roofed hall with a loading platform at its entrance, the drop-off
// point, and two lights over the door; built at four ports.
constexpr double STOREHOUSE_PORT_RADIUS_WU = 20.0;

MeshModel Storehouse()
{
  MeshModel model{"Storehouse.cmo", {}, {}, false};
  ModelNode& hall = model.Node("storehouse");
  AddBox(hall, {-13.0, 0.0, -9.0}, {13.0, 9.0, 9.0}, STONE_LIGHT, Surface::Facet);
  AddGable(hall, {-14.0, 9.0, -10.0}, {14.0, 15.0, 10.0}, SLATE, STONE_DARK, Surface::Facet);
  AddBox(hall, {-3.5, 1.2, 9.0}, {3.5, 7.0, 9.6}, WOOD_DARK, Surface::Facet);
  AddBox(hall, {-6.0, 0.0, 9.0}, {6.0, 1.2, 13.0}, STONE_DARK, Surface::Facet);
  AddBox(hall, {-5.5, 7.0, 9.0}, {-4.5, 8.0, 9.8}, TEAM_WHITE, Surface::Team, true);
  AddBox(hall, {4.5, 7.0, 9.0}, {5.5, 8.0, 9.8}, TEAM_WHITE, Surface::Team, true);
  model.AddSocket("entrance", "storehouse", {0.0, 0.0, 14.0});
  model.AddSocket("light_01", "storehouse", {-5.0, 8.0, 10.0});
  model.AddSocket("light_02", "storehouse", {5.0, 8.0, 10.0});
  // Four construction ports at the corners, 45° + 90° k (MVP-033).
  for (std::uint32_t port = 0; port < 4; ++port)
  {
    model.AddSocket(std::format("port_{:02}", port + 1), "storehouse",
                    Around({0.0, 0.0, 0.0}, STOREHOUSE_PORT_RADIUS_WU, 1 + 2 * port, 8, 0.0));
  }
  return model;
}

MeshModel StorehouseCollision()
{
  MeshModel model{"StorehouseCollision.cmo", {}, {}, true};
  ModelNode& hall = model.Node("storehouse");
  AddBox(hall, {-13.0, 0.0, -9.0}, {13.0, 9.0, 9.0}, {}, Surface::Facet);
  AddGable(hall, {-14.0, 9.0, -10.0}, {14.0, 15.0, 10.0}, {}, {}, Surface::Facet);
  AddBox(hall, {-6.0, 0.0, 9.0}, {6.0, 1.2, 13.0}, {}, Surface::Facet);
  return model;
}

// A cluster of Crystal (O57): a tall central crystal and four short ones around it.
void AddCrystals(ModelNode& _node, const Point& _center, double _height, double _baseY)
{
  AddFrustum(_node, {.base = {_center[0], _baseY, _center[2]},
                     .sides = 6,
                     .baseRadius = 2.6,
                     .topRadius = 0.0,
                     .length = _height,
                     .color = CRYSTAL,
                     .capColor = CRYSTAL});
  for (std::uint32_t c = 0; c < 4; ++c)
  {
    AddFrustum(_node, {.base = Around(_center, 4.5, 1 + 2 * c, 8, _baseY),
                       .sides = 4,
                       .baseRadius = 1.6,
                       .topRadius = 0.0,
                       .length = _height * 0.625,
                       .color = CRYSTAL_DEEP,
                       .capColor = CRYSTAL_DEEP});
  }
}

// A home or border resource site of one unit (MVP-030, WLD-080): a neutral mound of Crystal ringed by its gathering
// ports, `mvp.resource.ports_per_unit` (4) of them.
constexpr double SITE_PORT_RADIUS_WU = 16.0;

MeshModel ResourceSite()
{
  MeshModel model{"ResourceSite.cmo", {}, {}, false};
  ModelNode& site = model.Node("resource_site");
  AddFrustum(site, {.sides = 6, .baseRadius = 11.0, .topRadius = 7.0, .length = 3.0, .color = EARTH, .capColor = EARTH_TOP});
  AddCrystals(site, {0.0, 0.0, 0.0}, 11.0, 3.0);
  for (std::uint32_t port = 0; port < 4; ++port)
  {
    model.AddSocket(std::format("port_{:02}", port + 1), "resource_site", Around({0.0, 0.0, 0.0}, SITE_PORT_RADIUS_WU, port, 4, 0.0));
  }
  return model;
}

MeshModel ResourceSiteCollision()
{
  MeshModel model{"ResourceSiteCollision.cmo", {}, {}, true};
  ModelNode& site = model.Node("resource_site");
  AddFrustum(site, {.sides = 6, .baseRadius = 11.0, .topRadius = 7.0, .length = 3.0});
  AddFrustum(site, {.base = {0.0, 3.0, 0.0}, .sides = 6, .baseRadius = 7.0, .topRadius = 0.0, .length = 11.0});
  return model;
}

// The valley's central resource site of three units (WLD-083): a broad mound with three clusters and twelve ports.
constexpr double CENTRAL_PORT_RADIUS_WU = 34.0;

MeshModel ResourceSiteCentral()
{
  MeshModel model{"ResourceSiteCentral.cmo", {}, {}, false};
  ModelNode& site = model.Node("resource_site_central");
  AddFrustum(site, {.sides = 12, .baseRadius = 26.0, .topRadius = 18.0, .length = 4.0, .color = EARTH, .capColor = EARTH_TOP});
  for (std::uint32_t cluster = 0; cluster < 3; ++cluster)
  {
    AddCrystals(site, Around({0.0, 0.0, 0.0}, 10.0, 4 * cluster, 12, 0.0), 13.0, 4.0);
  }
  AddFrustum(
    site,
    {.base = {0.0, 4.0, 0.0}, .sides = 6, .baseRadius = 3.5, .topRadius = 0.0, .length = 18.0, .color = CRYSTAL, .capColor = CRYSTAL});
  for (std::uint32_t port = 0; port < 12; ++port)
  {
    model.AddSocket(std::format("port_{:02}", port + 1), "resource_site_central",
                    Around({0.0, 0.0, 0.0}, CENTRAL_PORT_RADIUS_WU, port, 12, 0.0));
  }
  return model;
}

MeshModel ResourceSiteCentralCollision()
{
  MeshModel model{"ResourceSiteCentralCollision.cmo", {}, {}, true};
  ModelNode& site = model.Node("resource_site_central");
  AddFrustum(site, {.sides = 12, .baseRadius = 26.0, .topRadius = 18.0, .length = 4.0});
  AddFrustum(site, {.base = {0.0, 4.0, 0.0}, .sides = 6, .baseRadius = 16.0, .topRadius = 0.0, .length = 18.0});
  return model;
}

// The port pad drawn under every port (R-BLDG-017, R-BLDG-046): a hexagonal plate on a wider rim with a marker in the
// team's colour; its `status` socket places the port's status star.
MeshModel PortPad()
{
  MeshModel model{"StructureDefault.cmo", {}, {}, false};
  AddFrustum(model.Node("port_pad"), {.sides = 6, .baseRadius = 4.5, .topRadius = 4.2, .length = 0.3, .color = PAD, .capColor = PAD_TOP});
  AddFrustum(model.Node("port_pad_rim"),
             {.sides = 6, .baseRadius = 5.0, .topRadius = 4.8, .length = 0.15, .color = PAD_RIM, .capColor = PAD_RIM});
  AddFrustum(model.Node("port_pad_marker"), {.base = {0.0, 0.3, 0.0},
                                             .sides = 4,
                                             .halfStep = true,
                                             .baseRadius = 0.8,
                                             .topRadius = 0.0,
                                             .length = 0.6,
                                             .color = TEAM_WHITE,
                                             .capColor = TEAM_WHITE,
                                             .surface = Surface::Team});
  model.AddSocket("status", "port_pad", {0.0, 3.0, 0.0});
  return model;
}

} // namespace

std::vector<MeshModel> StructureRecipes()
{
  std::vector<MeshModel> models;
  models.push_back(Wellspring());
  models.push_back(WellspringCollision());
  models.push_back(Dwelling());
  models.push_back(DwellingCollision());
  models.push_back(Storehouse());
  models.push_back(StorehouseCollision());
  models.push_back(ResourceSite());
  models.push_back(ResourceSiteCollision());
  models.push_back(ResourceSiteCentral());
  models.push_back(ResourceSiteCentralCollision());
  models.push_back(PortPad());
  return models;
}

} // namespace MeshKit
