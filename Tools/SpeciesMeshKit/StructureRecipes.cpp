#include "pch.h"

#include "Recipes.h"
#include "Shapes.h"

#include <algorithm>
#include <format>
#include <utility>

namespace MeshKit
{

namespace
{

// The palette of the drafts, in sRGB.
constexpr Color STONE_DARK{98, 96, 92};
constexpr Color STONE_LIGHT{150, 146, 136};
constexpr Color METAL_DARK{60, 64, 72};
constexpr Color METAL_MID{110, 116, 126};
constexpr Color GABLE_END{150, 132, 108};
constexpr Color ROOF_TILE{120, 72, 52};
constexpr Color SLATE{72, 78, 90};
constexpr Color WOOD_DARK{86, 60, 42};
constexpr Color EARTH{104, 90, 70};
constexpr Color CRYSTAL{150, 214, 226};
constexpr Color CRYSTAL_DEEP{96, 170, 190};
constexpr Color PAD{64, 66, 74};
constexpr Color PAD_TOP{84, 86, 96};
// The value pass: lighter and darker neighbours of the draft palette, so each structure has a light/dark split that
// reads from the camera's height, and the colours its unlit parts glow at.
constexpr Color STONE_PALE{182, 176, 164};
constexpr Color PLASTER_LIGHT{200, 184, 156};
constexpr Color EARTH_DARK{76, 66, 54};
constexpr Color CRYSTAL_CORE{214, 248, 252};
constexpr Color WATER_GLOW{128, 222, 236};
constexpr Color LANTERN{255, 214, 150};
// A team surface is drawn white and tinted by the Cladan's colour (ADR-020).
constexpr Color TEAM_WHITE{238, 238, 238};

// A crystal: a prism body that narrows a little, under a pointed tip that is a share of its height. The tip is its own
// surface, so a site can make it glow.
void AddCrystal(ModelNode& _node, const Point& _foot, double _radius, double _height, std::uint32_t _sides, Color _body, Color _tip,
                Surface _tipSurface, double _tipShare = 0.38)
{
  const double bodyLength = _height * (1.0 - _tipShare);
  AddFrustum(_node, {.base = _foot,
                     .sides = _sides,
                     .baseRadius = _radius,
                     .topRadius = _radius * 0.92,
                     .length = bodyLength,
                     .topCap = false,
                     .color = _body,
                     .capColor = _body});
  AddFrustum(_node, {.base = {_foot[0], _foot[1] + bodyLength, _foot[2]},
                     .sides = _sides,
                     .baseRadius = _radius * 0.92,
                     .topRadius = 0.0,
                     .length = _height - bodyLength,
                     .color = _tip,
                     .capColor = _tip,
                     .surface = _tipSurface});
}

// A pale marker stone that points from a site towards one of its gathering ports.
void AddMarkerStone(ModelNode& _node, const Point& _foot)
{
  AddFrustum(_node, {.base = _foot,
                     .sides = 4,
                     .halfStep = true,
                     .baseRadius = 1.1,
                     .topRadius = 0.7,
                     .length = 1.8,
                     .color = STONE_PALE,
                     .capColor = STONE_PALE});
}

// A loose rock on a site's mound.
void AddRock(ModelNode& _node, const Point& _foot, double _radius, double _topRadius, double _height)
{
  AddFrustum(_node, {.base = _foot,
                     .sides = 4,
                     .baseRadius = _radius,
                     .topRadius = _topRadius,
                     .length = _height,
                     .color = STONE_DARK,
                     .capColor = STONE_DARK});
}

// One storey of an octagonal plinth (all of the Wellspring's tiers turn by half a side so a flat side faces +x).
void AddOctagon(ModelNode& _node, double _y, double _baseRadius, double _topRadius, double _length, Color _color, Color _capColor,
                bool _topCap = true, Surface _surface = Surface::Facet)
{
  AddFrustum(_node, {.base = {0.0, _y, 0.0},
                     .sides = 8,
                     .halfStep = true,
                     .baseRadius = _baseRadius,
                     .topRadius = _topRadius,
                     .length = _length,
                     .topCap = _topCap,
                     .color = _color,
                     .capColor = _capColor,
                     .surface = _surface});
}

// The Wellspring (MVP-020, MVP-021): the largest and brightest shape of a settlement. A stepped octagonal plinth with a
// glowing water chamfer under a pale coping, stairs front and back, an upper tier with a gate and a post-ringed glowing
// pool, three pylons whose broad team beacons under dark caps are its lights, and a hexagonal crystal, its own node,
// floating over the pool.
constexpr double WELLSPRING_PYLON_RADIUS_WU = 17.0;
constexpr double WELLSPRING_PORT_RADIUS_WU = 38.0;
constexpr double WELLSPRING_DOOR_WU = 33.0;
constexpr double WELLSPRING_TIER_WU = 4.6;
constexpr double WELLSPRING_POOL_WU = 9.5;
constexpr double WELLSPRING_BEACON_WU = 24.7;
constexpr double WELLSPRING_CORE_FOOT_WU = 15.0;
constexpr double WELLSPRING_CORE_TOP_WU = 34.0;
constexpr double WELLSPRING_CORE_RADIUS_WU = 5.4;

// A flight of three steps up the plinth on the side _sign (+1 front, −1 back).
void AddWellspringStairs(ModelNode& _body, double _sign)
{
  const auto step = [&](double _z0, double _z1, double _y0, double _y1)
  {
    AddBox(_body, {-5.0, _y0, std::min(_sign * _z0, _sign * _z1)}, {5.0, _y1, std::max(_sign * _z0, _sign * _z1)}, STONE_LIGHT,
           Surface::Facet);
  };
  step(26.0, 32.4, 0.0, 1.4);
  step(25.0, 31.0, 1.4, 2.8);
  step(24.0, 29.6, 2.8, 4.6);
}

MeshModel Wellspring()
{
  MeshModel model{"Wellspring.cmo", {}, {}, false};
  ModelNode& body = model.Node("wellspring");
  AddOctagon(body, 0.0, 31.6, 31.6, 0.8, STONE_DARK, STONE_DARK);
  AddOctagon(body, 0.8, 30.0, 29.4, 2.6, STONE_DARK, STONE_DARK, false);
  AddOctagon(body, 3.4, 29.4, 28.4, 0.6, WATER_GLOW, WATER_GLOW, false, Surface::Unlit);
  AddOctagon(body, 4.0, 28.8, 28.8, 0.6, STONE_PALE, STONE_PALE);
  AddOctagon(body, WELLSPRING_TIER_WU, 21.0, 20.0, 4.4, STONE_LIGHT, STONE_DARK);
  AddOctagon(body, 9.0, 20.6, 20.6, 0.5, STONE_PALE, STONE_PALE);
  AddOctagon(body, WELLSPRING_POOL_WU, 14.0, 14.6, 2.3, METAL_MID, METAL_MID, false);
  AddOctagon(body, 11.2, 13.8, 13.8, 0.2, WATER_GLOW, WATER_GLOW, true, Surface::Unlit);
  // Eight posts around the pool.
  for (std::uint32_t post = 0; post < 8; ++post)
  {
    const Point p = Around({0.0, 0.0, 0.0}, 14.4, 2 * post + 1, 16, 0.0);
    AddBox(body, {p[0] - 0.6, WELLSPRING_POOL_WU, p[2] - 0.6}, {p[0] + 0.6, 13.0, p[2] + 0.6}, STONE_PALE, Surface::Facet);
  }
  AddWellspringStairs(body, 1.0);
  AddWellspringStairs(body, -1.0);
  // The steps up the upper tier to the gate, and the gate itself with a keystone in the team's colour, on the front (+z).
  AddBox(body, {-4.0, 4.6, 19.0}, {4.0, 6.1, 25.0}, STONE_LIGHT, Surface::Facet);
  AddBox(body, {-4.0, 6.1, 19.0}, {4.0, 7.6, 23.4}, STONE_LIGHT, Surface::Facet);
  AddBox(body, {-4.0, 7.6, 19.0}, {4.0, 9.0, 21.8}, STONE_LIGHT, Surface::Facet);
  AddBox(body, {-5.4, 4.6, 19.2}, {-4.2, 13.0, 21.6}, METAL_MID, Surface::Facet);
  AddBox(body, {4.2, 4.6, 19.2}, {5.4, 13.0, 21.6}, METAL_MID, Surface::Facet);
  AddBox(body, {-5.8, 13.0, 19.0}, {5.8, 14.4, 21.8}, METAL_MID, Surface::Facet, true);
  AddBox(body, {-1.0, 13.3, 21.8}, {1.0, 14.7, 22.1}, TEAM_WHITE, Surface::Team);
  for (std::uint32_t light = 0; light < 3; ++light)
  {
    const Point foot = Around({0.0, 0.0, 0.0}, WELLSPRING_PYLON_RADIUS_WU, 3 + 4 * light, 12, WELLSPRING_POOL_WU);
    AddFrustum(body, {.base = foot,
                      .sides = 4,
                      .halfStep = true,
                      .baseRadius = 3.3,
                      .topRadius = 3.0,
                      .length = 1.6,
                      .color = STONE_DARK,
                      .capColor = STONE_DARK});
    AddFrustum(body, {.base = {foot[0], 11.1, foot[2]},
                      .sides = 4,
                      .halfStep = true,
                      .baseRadius = 2.6,
                      .topRadius = 2.0,
                      .length = 12.9,
                      .color = METAL_DARK,
                      .capColor = METAL_DARK});
    AddFrustum(body, {.base = {foot[0], 24.0, foot[2]},
                      .sides = 4,
                      .halfStep = true,
                      .baseRadius = 2.3,
                      .topRadius = 2.3,
                      .length = 0.7,
                      .baseCap = true,
                      .color = METAL_MID,
                      .capColor = METAL_MID});
    AddFrustum(body, {.base = {foot[0], WELLSPRING_BEACON_WU, foot[2]},
                      .sides = 4,
                      .halfStep = true,
                      .baseRadius = 2.9,
                      .topRadius = 2.9,
                      .length = 4.0,
                      .baseCap = true,
                      .topCap = false,
                      .color = TEAM_WHITE,
                      .capColor = TEAM_WHITE,
                      .surface = Surface::Team});
    AddFrustum(body, {.base = {foot[0], WELLSPRING_BEACON_WU + 4.0, foot[2]},
                      .sides = 4,
                      .halfStep = true,
                      .baseRadius = 3.1,
                      .topRadius = 0.0,
                      .length = 2.4,
                      .baseCap = true,
                      .color = METAL_DARK,
                      .capColor = METAL_DARK});
    model.AddSocket(std::format("light_{:02}", light + 1), "wellspring", {foot[0], WELLSPRING_BEACON_WU + 2.0, foot[2]});
  }

  // The crystal: a hexagonal prism waist between two points.
  ModelNode& core = model.Node("wellspring_core");
  AddFrustum(core, {.base = {0.0, WELLSPRING_CORE_FOOT_WU, 0.0},
                    .sides = 6,
                    .baseRadius = 0.0,
                    .topRadius = WELLSPRING_CORE_RADIUS_WU,
                    .length = 6.0,
                    .topCap = false,
                    .color = TEAM_WHITE,
                    .capColor = TEAM_WHITE,
                    .surface = Surface::Team});
  AddFrustum(core, {.base = {0.0, WELLSPRING_CORE_FOOT_WU + 6.0, 0.0},
                    .sides = 6,
                    .baseRadius = WELLSPRING_CORE_RADIUS_WU,
                    .topRadius = WELLSPRING_CORE_RADIUS_WU,
                    .length = 4.0,
                    .topCap = false,
                    .color = TEAM_WHITE,
                    .capColor = TEAM_WHITE,
                    .surface = Surface::Team});
  AddFrustum(core, {.base = {0.0, WELLSPRING_CORE_FOOT_WU + 10.0, 0.0},
                    .sides = 6,
                    .baseRadius = WELLSPRING_CORE_RADIUS_WU,
                    .topRadius = 0.0,
                    .length = WELLSPRING_CORE_TOP_WU - WELLSPRING_CORE_FOOT_WU - 10.0,
                    .color = TEAM_WHITE,
                    .capColor = TEAM_WHITE,
                    .surface = Surface::Team});

  model.AddSocket("door", "wellspring", {0.0, 0.0, WELLSPRING_DOOR_WU});
  model.AddSocket("entrance", "wellspring", {0.0, 0.0, -WELLSPRING_DOOR_WU});
  model.AddSocket("essence_link", "wellspring_core", {0.0, WELLSPRING_CORE_TOP_WU, 0.0});
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
  AddFrustum(body, {.sides = 8, .halfStep = true, .baseRadius = 31.6, .topRadius = 28.8, .length = WELLSPRING_TIER_WU});
  AddFrustum(body, {.base = {0.0, WELLSPRING_TIER_WU, 0.0},
                    .sides = 8,
                    .halfStep = true,
                    .baseRadius = 21.0,
                    .topRadius = 21.0,
                    .length = WELLSPRING_BEACON_WU + 6.4 - WELLSPRING_TIER_WU});
  ModelNode& core = model.Node("wellspring_core");
  AddFrustum(core, {.base = {0.0, WELLSPRING_CORE_FOOT_WU, 0.0},
                    .sides = 6,
                    .baseRadius = 0.0,
                    .topRadius = WELLSPRING_CORE_RADIUS_WU,
                    .length = 10.0,
                    .topCap = false});
  AddFrustum(core, {.base = {0.0, WELLSPRING_CORE_FOOT_WU + 10.0, 0.0},
                    .sides = 6,
                    .baseRadius = WELLSPRING_CORE_RADIUS_WU,
                    .topRadius = 0.0,
                    .length = WELLSPRING_CORE_TOP_WU - WELLSPRING_CORE_FOOT_WU - 10.0});
  return model;
}

// The dwelling (MVP-022, MVP-034): a light-walled, timber-framed house on a stone footing under a dark tiled gable with
// deep eaves, its ridge in its Cladan's colour so the owner reads from above; a framed door with a step and a lantern
// over it, and four large windows on sills that glow in the team's colour. Built at six ports.
constexpr double DWELLING_PORT_RADIUS_WU = 18.0;

MeshModel Dwelling()
{
  MeshModel model{"Dwelling.cmo", {}, {}, false};
  ModelNode& house = model.Node("dwelling");
  AddBox(house, {-9.4, 0.0, -7.4}, {9.4, 1.4, 7.4}, STONE_DARK, Surface::Facet);
  AddBox(house, {-9.0, 1.4, -7.0}, {9.0, 9.4, 7.0}, PLASTER_LIGHT, Surface::Facet);
  // Corner posts and the wall plate.
  for (const double x : {-1.0, 1.0})
  {
    for (const double z : {-1.0, 1.0})
    {
      AddBox(house, {x < 0.0 ? -9.3 : 8.5, 1.4, z < 0.0 ? -7.3 : 6.5}, {x < 0.0 ? -8.5 : 9.3, 9.4, z < 0.0 ? -6.5 : 7.3}, WOOD_DARK,
             Surface::Facet);
    }
  }
  AddBox(house, {-9.3, 9.4, -7.3}, {9.3, 10.2, 7.3}, WOOD_DARK, Surface::Facet);
  AddGable(house, {-10.6, 10.2, -8.8}, {10.6, 17.4, 8.8}, ROOF_TILE, GABLE_END, Surface::Facet);
  AddBox(house, {-10.8, 16.4, -1.2}, {10.8, 17.8, 1.2}, TEAM_WHITE, Surface::Team);
  AddBox(house, {3.6, 12.0, -4.4}, {6.0, 20.0, -2.0}, STONE_LIGHT, Surface::Facet);
  AddBox(house, {3.3, 20.0, -4.7}, {6.3, 20.6, -1.7}, STONE_DARK, Surface::Facet);
  // The door on the front (+z): its frame, its leaf, its step and the lantern over it.
  AddBox(house, {-2.1, 1.4, 7.0}, {2.1, 7.0, 7.25}, STONE_LIGHT, Surface::Facet);
  AddBox(house, {-1.5, 1.4, 7.25}, {1.5, 6.4, 7.5}, WOOD_DARK, Surface::Facet);
  AddBox(house, {-2.4, 0.0, 7.4}, {2.4, 0.7, 8.8}, STONE_LIGHT, Surface::Facet);
  AddBox(house, {-0.4, 7.3, 7.25}, {0.4, 8.1, 7.9}, LANTERN, Surface::Unlit, true);
  std::uint32_t window = 0;
  for (const double z : {7.0, -7.4})
  {
    for (const double x : {-5.0, 5.0})
    {
      AddBox(house, {x - 2.0, 3.6, z}, {x + 2.0, 7.6, z + 0.4}, TEAM_WHITE, Surface::Team, true);
      AddBox(house, {x - 2.4, 3.2, z < 0.0 ? -7.7 : 7.0}, {x + 2.4, 3.6, z < 0.0 ? -7.0 : 7.7}, STONE_LIGHT, Surface::Facet, true);
      model.AddSocket(std::format("window_{:02}", ++window), "dwelling", {x, 5.6, z < 0.0 ? -7.4 : 7.4});
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
  AddBox(house, {-9.4, 0.0, -7.4}, {9.4, 10.2, 7.4}, {}, Surface::Facet);
  AddGable(house, {-10.6, 10.2, -8.8}, {10.6, 17.4, 8.8}, {}, {}, Surface::Facet);
  return model;
}

// The storehouse (MVP-031, MVP-034): a buttressed hall of pale stone under a dark slate gable with a ridge and two
// vents; a lintelled door opens onto a stepped loading platform, the drop-off point, marked by a square in the team's
// colour between crates of Crystal so it says what it keeps; two lights beside the door. Built at four ports.
constexpr double STOREHOUSE_PORT_RADIUS_WU = 24.0;

MeshModel Storehouse()
{
  MeshModel model{"Storehouse.cmo", {}, {}, false};
  ModelNode& hall = model.Node("storehouse");
  AddBox(hall, {-13.5, 0.0, -9.5}, {13.5, 1.0, 9.5}, STONE_DARK, Surface::Facet);
  AddBox(hall, {-13.0, 1.0, -9.0}, {13.0, 8.4, 9.0}, STONE_PALE, Surface::Facet);
  for (const auto [x0, x1] : {std::pair{-12.4, -11.2}, std::pair{-8.8, -7.6}, std::pair{7.6, 8.8}, std::pair{11.2, 12.4}})
  {
    AddBox(hall, {x0, 1.0, 9.0}, {x1, 8.0, 10.1}, STONE_DARK, Surface::Facet);
    AddBox(hall, {x0, 1.0, -10.1}, {x1, 8.0, -9.0}, STONE_DARK, Surface::Facet);
  }
  AddBox(hall, {-13.3, 8.4, -9.3}, {13.3, 9.2, 9.3}, STONE_DARK, Surface::Facet);
  AddGable(hall, {-14.4, 9.2, -10.6}, {14.4, 15.6, 10.6}, SLATE, STONE_DARK, Surface::Facet);
  AddBox(hall, {-14.6, 15.2, -0.5}, {14.6, 15.9, 0.5}, METAL_DARK, Surface::Facet);
  AddBox(hall, {-6.0, 14.6, -1.2}, {-4.0, 16.8, 1.2}, STONE_DARK, Surface::Facet);
  AddBox(hall, {4.0, 14.6, -1.2}, {6.0, 16.8, 1.2}, STONE_DARK, Surface::Facet);
  // The door: its surround, its leaf and its lintel.
  AddBox(hall, {-4.4, 1.0, 9.0}, {4.4, 7.0, 9.3}, STONE_DARK, Surface::Facet);
  AddBox(hall, {-3.5, 1.6, 9.3}, {3.5, 7.0, 9.6}, WOOD_DARK, Surface::Facet);
  AddBox(hall, {-4.4, 7.0, 9.0}, {4.4, 8.2, 9.8}, STONE_LIGHT, Surface::Facet, true);
  // The loading platform and its step, the drop-off square and the crates of Crystal either side of it.
  AddBox(hall, {-7.0, 0.0, 9.0}, {7.0, 1.6, 13.5}, STONE_DARK, Surface::Facet);
  AddBox(hall, {-7.0, 0.0, 13.5}, {7.0, 0.8, 15.0}, STONE_DARK, Surface::Facet);
  AddBox(hall, {-3.0, 1.6, 10.2}, {3.0, 1.75, 13.0}, TEAM_WHITE, Surface::Team);
  AddBox(hall, {-6.6, 1.6, 10.0}, {-4.2, 3.4, 12.2}, CRYSTAL_DEEP, Surface::Facet);
  AddBox(hall, {-6.2, 3.4, 10.4}, {-4.6, 4.8, 11.8}, CRYSTAL, Surface::Facet);
  AddBox(hall, {4.2, 1.6, 10.2}, {6.6, 3.2, 12.4}, CRYSTAL_DEEP, Surface::Facet);
  AddBox(hall, {-6.1, 5.2, 9.3}, {-4.7, 6.6, 10.1}, TEAM_WHITE, Surface::Team, true);
  AddBox(hall, {4.7, 5.2, 9.3}, {6.1, 6.6, 10.1}, TEAM_WHITE, Surface::Team, true);
  model.AddSocket("entrance", "storehouse", {0.0, 0.0, 16.0});
  model.AddSocket("light_01", "storehouse", {-5.4, 5.9, 10.3});
  model.AddSocket("light_02", "storehouse", {5.4, 5.9, 10.3});
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
  AddBox(hall, {-13.5, 0.0, -10.1}, {13.5, 9.2, 10.1}, {}, Surface::Facet);
  AddGable(hall, {-14.4, 9.2, -10.6}, {14.4, 15.6, 10.6}, {}, {}, Surface::Facet);
  AddBox(hall, {-7.0, 0.0, 10.1}, {7.0, 4.8, 15.0}, {}, Surface::Facet);
  return model;
}

// A cluster of Crystal (O57): a tall glowing-tipped crystal and a ring of five shorter ones of the given heights.
void AddCrystals(ModelNode& _node, const Point& _center, double _baseY, const double (&_heights)[5])
{
  AddCrystal(_node, {_center[0], _baseY + 0.2, _center[2]}, 2.8, 14.0, 6, CRYSTAL_DEEP, CRYSTAL_CORE, Surface::Unlit);
  for (std::uint32_t c = 0; c < 5; ++c)
  {
    AddCrystal(_node, Around(_center, 4.6, c, 5, _baseY), 1.7, _heights[c], 6, CRYSTAL_DEEP, CRYSTAL_CORE, Surface::Unlit);
  }
}

// A home or border resource site of one unit (MVP-030, WLD-080): a two-tier mound of dark earth with loose rocks, a
// glowing-tipped cluster of Crystal with shards around it, and pale stones that point at its gathering ports,
// `mvp.resource.ports_per_unit` (4) of them.
constexpr double SITE_PORT_RADIUS_WU = 18.0;
constexpr double SITE_MARKER_RADIUS_WU = 14.4;
constexpr double SITE_TOP_WU = 17.4;

MeshModel ResourceSite()
{
  MeshModel model{"ResourceSite.cmo", {}, {}, false};
  ModelNode& site = model.Node("resource_site");
  AddFrustum(site, {.sides = 12, .baseRadius = 12.5, .topRadius = 11.0, .length = 1.2, .color = EARTH_DARK, .capColor = EARTH});
  AddFrustum(
    site,
    {.base = {0.0, 1.2, 0.0}, .sides = 12, .baseRadius = 9.6, .topRadius = 6.6, .length = 2.4, .color = EARTH_DARK, .capColor = EARTH});
  for (const std::uint32_t k : {0u, 3u, 7u})
  {
    AddRock(site, Around({0.0, 0.0, 0.0}, 10.4, k, 10, 1.2), 1.2, 0.6, 1.2);
  }
  AddCrystals(site, {0.0, 0.0, 0.0}, 3.2, {9.0, 7.0, 8.4, 6.4, 7.6});
  for (const std::uint32_t k : {1u, 4u, 7u})
  {
    AddCrystal(site, Around({0.0, 0.0, 0.0}, 7.6, k, 10, 1.6), 0.8, 3.0, 4, CRYSTAL_DEEP, CRYSTAL, Surface::Facet);
  }
  for (std::uint32_t port = 0; port < 4; ++port)
  {
    AddMarkerStone(site, Around({0.0, 0.0, 0.0}, SITE_MARKER_RADIUS_WU, port, 4, 0.0));
    model.AddSocket(std::format("port_{:02}", port + 1), "resource_site", Around({0.0, 0.0, 0.0}, SITE_PORT_RADIUS_WU, port, 4, 0.0));
  }
  return model;
}

MeshModel ResourceSiteCollision()
{
  MeshModel model{"ResourceSiteCollision.cmo", {}, {}, true};
  ModelNode& site = model.Node("resource_site");
  AddFrustum(site, {.sides = 12, .baseRadius = 12.5, .topRadius = 6.6, .length = 3.6});
  AddFrustum(site, {.base = {0.0, 3.6, 0.0}, .sides = 6, .baseRadius = 7.0, .topRadius = 0.0, .length = SITE_TOP_WU - 3.6});
  return model;
}

// The valley's central resource site of three units (WLD-083): a broad two-tier mound of dark earth ringed by rocks,
// three glowing-tipped clusters around a tall spire, and twelve marker stones outside the rocks that draw its port ring
// before anyone stands on it.
constexpr double CENTRAL_PORT_RADIUS_WU = 34.0;
constexpr double CENTRAL_MARKER_RADIUS_WU = 30.5;
constexpr double CENTRAL_SPIRE_WU = 24.0;

MeshModel ResourceSiteCentral()
{
  MeshModel model{"ResourceSiteCentral.cmo", {}, {}, false};
  ModelNode& site = model.Node("resource_site_central");
  AddFrustum(site, {.sides = 12, .baseRadius = 27.5, .topRadius = 25.5, .length = 1.5, .color = EARTH_DARK, .capColor = EARTH});
  AddFrustum(
    site,
    {.base = {0.0, 1.5, 0.0}, .sides = 12, .baseRadius = 23.0, .topRadius = 18.0, .length = 3.0, .color = EARTH_DARK, .capColor = EARTH});
  for (std::uint32_t k = 0; k < 6; ++k)
  {
    AddRock(site, Around({0.0, 0.0, 0.0}, 24.4, 2 * k + 1, 12, 1.5), 1.5, 0.7, 1.4);
  }
  for (std::uint32_t cluster = 0; cluster < 3; ++cluster)
  {
    AddCrystals(site, Around({0.0, 0.0, 0.0}, 10.5, 4 * cluster + 2, 12, 0.0), 4.2, {8.4, 6.6, 7.8, 6.0, 7.2});
  }
  AddCrystal(site, {0.0, 4.4, 0.0}, 3.8, CENTRAL_SPIRE_WU, 6, CRYSTAL_DEEP, CRYSTAL_CORE, Surface::Unlit, 0.4);
  for (std::uint32_t port = 0; port < 12; ++port)
  {
    AddMarkerStone(site, Around({0.0, 0.0, 0.0}, CENTRAL_MARKER_RADIUS_WU, port, 12, 0.0));
    model.AddSocket(std::format("port_{:02}", port + 1), "resource_site_central",
                    Around({0.0, 0.0, 0.0}, CENTRAL_PORT_RADIUS_WU, port, 12, 0.0));
  }
  return model;
}

MeshModel ResourceSiteCentralCollision()
{
  MeshModel model{"ResourceSiteCentralCollision.cmo", {}, {}, true};
  ModelNode& site = model.Node("resource_site_central");
  AddFrustum(site, {.sides = 12, .baseRadius = 27.5, .topRadius = 18.0, .length = 4.5});
  AddFrustum(site, {.base = {0.0, 4.4, 0.0}, .sides = 6, .baseRadius = 16.0, .topRadius = 0.0, .length = CENTRAL_SPIRE_WU});
  return model;
}

// The port pad drawn under every port (R-BLDG-017, R-BLDG-046): a bevelled hexagonal plate with three studs whose rim
// is an outline in the team's colour around a dark inset, so a structure's port ring reads as owned from above; a
// diamond in the team's colour over its centre, and its `status` socket places the port's status star.
MeshModel PortPad()
{
  MeshModel model{"StructureDefault.cmo", {}, {}, false};
  ModelNode& pad = model.Node("port_pad");
  AddFrustum(pad, {.sides = 6, .baseRadius = 4.6, .topRadius = 4.3, .length = 0.35, .topCap = false, .color = PAD, .capColor = PAD});
  AddFrustum(
    pad,
    {.base = {0.0, 0.35, 0.0}, .sides = 6, .baseRadius = 4.3, .topRadius = 3.8, .length = 0.15, .color = PAD_TOP, .capColor = PAD_TOP});
  for (std::uint32_t stud = 0; stud < 3; ++stud)
  {
    AddFrustum(pad, {.base = Around({0.0, 0.0, 0.0}, 4.0, 2 * stud, 6, 0.32),
                     .sides = 4,
                     .halfStep = true,
                     .baseRadius = 0.3,
                     .topRadius = 0.3,
                     .length = 0.22,
                     .color = METAL_MID,
                     .capColor = METAL_MID});
  }
  AddFrustum(pad,
             {.base = {0.0, 0.56, 0.0}, .sides = 6, .baseRadius = 3.0, .topRadius = 3.0, .length = 0.05, .color = PAD, .capColor = PAD});
  AddFrustum(model.Node("port_pad_rim"), {.base = {0.0, 0.5, 0.0},
                                          .sides = 6,
                                          .baseRadius = 3.6,
                                          .topRadius = 3.6,
                                          .length = 0.06,
                                          .color = TEAM_WHITE,
                                          .capColor = TEAM_WHITE,
                                          .surface = Surface::Team});
  ModelNode& marker = model.Node("port_pad_marker");
  AddFrustum(marker, {.base = {0.0, 0.6, 0.0},
                      .sides = 4,
                      .halfStep = true,
                      .baseRadius = 0.0,
                      .topRadius = 0.9,
                      .length = 0.5,
                      .topCap = false,
                      .color = TEAM_WHITE,
                      .capColor = TEAM_WHITE,
                      .surface = Surface::Team});
  AddFrustum(marker, {.base = {0.0, 1.1, 0.0},
                      .sides = 4,
                      .halfStep = true,
                      .baseRadius = 0.9,
                      .topRadius = 0.0,
                      .length = 0.8,
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
