#include "pch.h"

#include "Recipes.h"
#include "Shapes.h"

#include <array>
#include <format>
#include <string>

namespace MeshKit
{

namespace
{

constexpr Color METAL_DARK{60, 64, 72};
constexpr Color METAL_MID{110, 116, 126};
constexpr Color SLATE{72, 78, 90};
constexpr Color BRASS{226, 190, 96};
constexpr Color BRASS_DARK{180, 140, 60};
constexpr Color TEAM_WHITE{238, 238, 238};

// The reference turret (MVP-034, R-BLDG-405): a mount that carries the four manning ports and their status lights, a
// head that turns towards its aim point, and a gun barrel set of four barrels; barrel i belongs to port i. Its structure
// radius is `ref.bldg.turret.radius` (30 wu), so its ports stand just outside it.
constexpr double TURRET_PORT_RADIUS_WU = 34.0;
constexpr double TURRET_STATUS_RADIUS_WU = 24.0;
constexpr double TURRET_STATUS_HEIGHT_WU = 2.0;

MeshModel TurretMount()
{
  MeshModel model{"TurretMount.cmo", {}, {}, false};
  static_cast<void>(model.Node("turret_mount"));
  for (std::uint32_t port = 0; port < 4; ++port)
  {
    model.AddSocket(std::format("port_{:02}", port + 1), "turret_mount",
                    Around({0.0, 0.0, 0.0}, TURRET_PORT_RADIUS_WU, 1 + 2 * port, 8, 0.0));
  }
  for (std::uint32_t port = 0; port < 4; ++port)
  {
    model.AddSocket(std::format("status_{:02}", port + 1), "turret_mount",
                    Around({0.0, 0.0, 0.0}, TURRET_STATUS_RADIUS_WU, 1 + 2 * port, 8, TURRET_STATUS_HEIGHT_WU));
  }
  return model;
}

// The head: a pedestal with a band in the team's colour, and above it the turning body with its shield and sight. Its
// barrel mount is at the shield's face; the turret's front is +z (R-BLDG-407).
MeshModel TurretHead()
{
  MeshModel model{"TurretHead.cmo", {}, {}, false};
  ModelNode& pedestal = model.Node("turret_pedestal");
  AddFrustum(
    pedestal,
    {.sides = 8, .halfStep = true, .baseRadius = 7.0, .topRadius = 5.0, .length = 5.0, .color = METAL_DARK, .capColor = METAL_DARK});
  AddFrustum(pedestal, {.base = {0.0, 5.0, 0.0},
                        .sides = 8,
                        .halfStep = true,
                        .baseRadius = 5.2,
                        .topRadius = 5.2,
                        .length = 1.0,
                        .color = TEAM_WHITE,
                        .capColor = TEAM_WHITE,
                        .surface = Surface::Team});
  ModelNode& head = model.Node("turret_head");
  AddFrustum(head, {.base = {0.0, 6.0, 0.0},
                    .sides = 8,
                    .halfStep = true,
                    .baseRadius = 8.0,
                    .topRadius = 6.0,
                    .length = 7.0,
                    .baseCap = true,
                    .color = METAL_MID,
                    .capColor = SLATE});
  AddBox(head, {-6.0, 7.0, 6.0}, {6.0, 12.0, 7.5}, METAL_DARK, Surface::Facet, true);
  AddBox(head, {-1.0, 13.0, -2.0}, {1.0, 15.0, 1.0}, TEAM_WHITE, Surface::Team, true);
  model.AddSocket("barrel_mount", "turret_head", {0.0, 10.0, 7.5});
  model.AddSocket("kill_counter", "turret_head", {0.0, 15.5, -3.0});
  return model;
}

MeshModel TurretHeadCollision()
{
  MeshModel model{"TurretHeadCollision.cmo", {}, {}, true};
  AddFrustum(model.Node("turret_pedestal"), {.sides = 8, .halfStep = true, .baseRadius = 7.0, .topRadius = 5.2, .length = 6.0});
  ModelNode& head = model.Node("turret_head");
  AddFrustum(head,
             {.base = {0.0, 6.0, 0.0}, .sides = 8, .halfStep = true, .baseRadius = 8.0, .topRadius = 8.0, .length = 9.0, .baseCap = true});
  return model;
}

// The gun barrel set, from the barrel mount forward along +z: four barrels in a square, each of five nodes (breech,
// tube, jacket, muzzle brake and casing ejector), with its muzzle at the brake's end and its casing ejector on its outer
// side (AST-042, R-BLDG-418).
struct BarrelPlace
{
  double x;
  double y;
};
constexpr std::array<BarrelPlace, 4> BARRELS{BarrelPlace{-2.0, 1.4}, BarrelPlace{2.0, 1.4}, BarrelPlace{-2.0, -1.4},
                                             BarrelPlace{2.0, -1.4}};
constexpr double TUBE_RADIUS_WU = 0.6;
constexpr double TUBE_START_WU = 3.0;
constexpr double TUBE_LENGTH_WU = 11.0;
constexpr double BRAKE_LENGTH_WU = 1.5;

MeshModel TurretBarrelGun()
{
  MeshModel model{"TurretBarrelGun.cmo", {}, {}, false};
  for (std::uint32_t b = 0; b < BARRELS.size(); ++b)
  {
    const BarrelPlace place = BARRELS[b];
    const std::string prefix = std::format("barrel_{:02}", b + 1);
    const double outward = place.x < 0.0 ? -1.0 : 1.0;
    AddBox(model.Node(prefix + "_breech"), {place.x - 1.0, place.y - 0.9, 0.0}, {place.x + 1.0, place.y + 0.9, TUBE_START_WU}, METAL_DARK,
           Surface::Facet, true);
    AddFrustum(model.Node(prefix + "_tube"), {.base = {place.x, place.y, TUBE_START_WU},
                                              .axis = Axis::Forward,
                                              .sides = 6,
                                              .baseRadius = TUBE_RADIUS_WU,
                                              .topRadius = TUBE_RADIUS_WU,
                                              .length = TUBE_LENGTH_WU,
                                              .baseCap = true,
                                              .color = METAL_MID,
                                              .capColor = METAL_DARK});
    AddFrustum(model.Node(prefix + "_jacket"), {.base = {place.x, place.y, TUBE_START_WU},
                                                .axis = Axis::Forward,
                                                .sides = 6,
                                                .baseRadius = 0.85,
                                                .topRadius = 0.85,
                                                .length = 4.0,
                                                .baseCap = true,
                                                .color = METAL_DARK,
                                                .capColor = METAL_DARK});
    AddFrustum(model.Node(prefix + "_brake"), {.base = {place.x, place.y, TUBE_START_WU + TUBE_LENGTH_WU},
                                               .axis = Axis::Forward,
                                               .sides = 6,
                                               .baseRadius = 0.9,
                                               .topRadius = 0.9,
                                               .length = BRAKE_LENGTH_WU,
                                               .baseCap = true,
                                               .color = METAL_DARK,
                                               .capColor = METAL_DARK});
    const double side = place.x + outward * 1.0;
    AddBox(model.Node(prefix + "_ejector"), {outward < 0.0 ? side - 0.4 : side, place.y - 0.4, 1.0},
           {outward < 0.0 ? side : side + 0.4, place.y + 0.4, 2.2}, BRASS_DARK, Surface::Facet, true);
    model.AddSocket(std::format("muzzle_{:02}", b + 1), prefix + "_tube",
                    {place.x, place.y, TUBE_START_WU + TUBE_LENGTH_WU + BRAKE_LENGTH_WU});
    model.AddSocket(std::format("casing_eject_{:02}", b + 1), prefix + "_ejector", {side + outward * 0.6, place.y, 1.6});
  }
  return model;
}

MeshModel TurretBarrelGunCollision()
{
  MeshModel model{"TurretBarrelGunCollision.cmo", {}, {}, true};
  for (std::uint32_t b = 0; b < BARRELS.size(); ++b)
  {
    const BarrelPlace place = BARRELS[b];
    const std::string prefix = std::format("barrel_{:02}", b + 1);
    AddBox(model.Node(prefix + "_breech"), {place.x - 1.0, place.y - 0.9, 0.0}, {place.x + 1.0, place.y + 0.9, TUBE_START_WU}, {},
           Surface::Facet, true);
    AddBox(model.Node(prefix + "_tube"), {place.x - 0.9, place.y - 0.9, TUBE_START_WU},
           {place.x + 0.9, place.y + 0.9, TUBE_START_WU + TUBE_LENGTH_WU + BRAKE_LENGTH_WU}, {}, Surface::Facet, true);
  }
  return model;
}

// The turret round (R-BLDG-420) and the casing it ejects (R-BLDG-418), both along +z.
MeshModel TurretRound()
{
  MeshModel model{"TurretRound.cmo", {}, {}, false};
  ModelNode& round = model.Node("turret_round");
  AddFrustum(round, {.base = {0.0, 0.0, -1.0},
                     .axis = Axis::Forward,
                     .sides = 6,
                     .baseRadius = 0.35,
                     .topRadius = 0.35,
                     .length = 2.0,
                     .baseCap = true,
                     .topCap = false,
                     .color = BRASS,
                     .capColor = BRASS});
  AddFrustum(round, {.base = {0.0, 0.0, 1.0},
                     .axis = Axis::Forward,
                     .sides = 6,
                     .baseRadius = 0.35,
                     .topRadius = 0.0,
                     .length = 0.8,
                     .color = BRASS,
                     .capColor = BRASS});
  return model;
}

MeshModel ShellCasing()
{
  MeshModel model{"ShellCasing.cmo", {}, {}, false};
  AddFrustum(model.Node("shell_casing"), {.base = {0.0, 0.0, -0.6},
                                          .axis = Axis::Forward,
                                          .sides = 8,
                                          .baseRadius = 0.3,
                                          .topRadius = 0.3,
                                          .length = 1.2,
                                          .baseCap = true,
                                          .color = BRASS_DARK,
                                          .capColor = BRASS_DARK});
  return model;
}

} // namespace

std::vector<MeshModel> TurretRecipes()
{
  std::vector<MeshModel> models;
  models.push_back(TurretMount());
  models.push_back(TurretHead());
  models.push_back(TurretHeadCollision());
  models.push_back(TurretBarrelGun());
  models.push_back(TurretBarrelGunCollision());
  models.push_back(TurretRound());
  models.push_back(ShellCasing());
  return models;
}

} // namespace MeshKit
