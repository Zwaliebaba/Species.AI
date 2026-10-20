#pragma once

#include "FontAsset.h"
#include "FrameView.h"
#include "SeaSurface.h"
#include "TerrainSurface.h"
#include "Vec3.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Neuron
{

/// The sky's values (RND-150 to RND-154), restated about the region (ADR-115): the grid follows the camera on its
/// lattice from the region's origin, and the clouds are anchored at the region's centre.
struct SkyLook
{
  double gridHeight = 1200.0; // rnd.sky.grid_height, wu
  double gridSpacing = 80.0;  // rnd.sky.grid_spacing, wu
  double gridMin = -4000.0;   // rnd.sky.grid_min: with grid_max, the grid's extent about the camera
  double gridMax = 6000.0;    // rnd.sky.grid_max, exclusive
  double gridHalfWidth = 8.0; // rnd.sky.grid_line_half_width, wu
  std::array<float, 4> gridColor{0.5f, 0.5f, 1.0f, 0.3f};
  double gridFogStart = 2000.0; // wu of view depth
  double gridFogEnd = 4000.0;
  Vec3 velocity{0.03, 0.0, -0.01}; // rnd.sky.velocity, offset units per s of game time
  double tickSeconds = 0.1;        // ref.sim.tick_period: the offset advances once per displayed tick (RND-152)
  double cloudMin = -8000.0;       // rnd.sky.cloud_min: with cloud_max, the clouds' extent about the region's centre
  double cloudMax = 9000.0;
  double cloudFogStart = 2000.0;
  double cloudFogEnd = 5000.0;
  double layer2Height = 1000.0;
  double layer3Height = 800.0;
  std::array<float, 4> blobbyColor{0.7f, 0.7f, 0.9f, 0.6f};
  std::array<float, 4> flatColor{0.7f, 0.7f, 0.9f, 0.3f};
  double layer1Repeats = 9.0;
  double layer2Repeats = 4.5;
  double layer3Repeats = 1.8;
  double regionOriginX = 0.0; // the grid's lattice
  double regionOriginZ = 0.0;
  Vec3 regionCenter; // the clouds' anchor
};

/// The look of a world frame that does not change from frame to frame (ADR-115), from the client's presentation
/// record (ADR-116).
struct WorldLook
{
  std::array<float, 3> fogColor{0.0f, 0.0f, 0.0f}; // rnd.frame.clear_color: the background and the level fog (RND-222)
  double fogStart = 2040.0;                        // O59: a third of the fog end
  double fogEnd = 6120.0;                          // O59: the larger of rnd.fog.min_end and a region's side plus the margin
  Vec3 keyTowards{0.0, 0.0, 1.0};                  // RND-226: normalised when the frame copies it
  std::array<float, 3> keyColor{1.3f, 1.3f, 1.3f};
  Vec3 fillTowards{0.0, 0.0, -1.0};
  std::array<float, 3> fillColor{0.0f, 0.0f, 0.0f};
  std::array<float, 3> overlaySpecular{0.5f, 0.5f, 0.5f}; // rnd.terrain.overlay_specular (RND-136)
  float overlayShininess = 40.0f;                         // rnd.terrain.overlay_shininess
  double seaTileRepeats = 30.0;                           // rnd.sea.tile_repeats (RND-142)
  double seaTileScrollPeriod = 30.0;                      // rnd.sea.tile_scroll_period, s
  ProjectionPlanes planes;                                // rnd.view.* (RND-211, RND-212)
  SkyLook sky;
};

/// A world's surfaces as the frame draws them (ADR-115): built on the CPU from the region's field when the session has
/// it, and kept by the renderer, so that a device rebuild uploads the same surfaces again (RND-034, TER-221).
struct WorldScene
{
  TerrainSurface terrain;
  SeaSurface sea;
  SeaLook seaLook;
  WorldLook look;
  // The textures the passes read, each the bytes of a DDS file of the content (ADR-017): terrain.sea_tile,
  // fx.terrain_grid, fx.sky_cloud and fx.beam_short.
  std::vector<std::byte> seaTile;
  std::vector<std::byte> terrainGrid;
  std::vector<std::byte> skyCloud;
  std::vector<std::byte> beamShort;
};

/// One line of the development overlay (DBG-095), drawn in EditorFont at the overlay's whole-number scale (O63) with
/// its cell's bottom-right corner at the given distances, in unscaled pixels, from the target's bottom-right corner.
struct OverlayLine
{
  std::string text; // UTF-8
  float fromRight = 10.0f;
  float fromBottom = 40.0f;
  std::array<float, 4> color{1.0f, 1.0f, 1.0f, 1.0f};
};

/// A development dot (M1 task T15a): a figure's world position, its colour and its side in unscaled pixels.
struct DevelopmentDot
{
  Vec3 position;
  std::array<float, 4> color{1.0f, 1.0f, 1.0f, 1.0f};
  float sidePixels = 3.0f;
};

/// What one frame shows (RND-053's snapshot as M1 has it).
struct FrameInputs
{
  std::shared_ptr<const WorldScene> world;             // none: no world is drawn, and the frame is the background
  std::array<float, 3> background{0.05f, 0.0f, 0.05f}; // rnd.frame.clear_color_no_match: the frame without a world
  CameraPose camera;
  double presentationSeconds = 0.0; // the smooth presentation time (R-SIM-570)
  double skyTicks = 0.0;            // the ticks displayed since the world's first (RND-152)
  float brightness = 1.0f;          // rnd.option.brightness.default (RND-275)
  // The development overlay (DBG-004, Debug builds): its face, its lines and the dots drawn in the world.
  std::shared_ptr<const FontAsset> overlayFont;
  std::vector<OverlayLine> overlay;
  std::vector<DevelopmentDot> dots;
};

} // namespace Neuron
