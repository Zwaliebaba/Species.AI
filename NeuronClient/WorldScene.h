#pragma once

#include "FontAsset.h"
#include "FrameView.h"
#include "MeshAsset.h"
#include "SeaSurface.h"
#include "TerrainSurface.h"
#include "Vec3.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
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

/// The corner of the target that an overlay line is anchored to (ADR-120).
enum class OverlayCorner : std::uint8_t
{
  BottomRight,    // the line's cell ends there: right-aligned, as the frame-rate line is (DBG-095)
  TopLeft,        // the line's cell starts there, as a page's lines do (DBG-096)
  PixelLeftMiddle // ADR-127: the cell's left edge and vertical middle lie at the insets, physical pixels from the
                  // target's top-left corner that do not scale with the text, as the cursor's badge count does
};

/// One line of the development overlay (DBG-095), drawn in EditorFont at the overlay's whole-number scale (O63) with
/// the matching corner of its cell at the given distances, in unscaled pixels, from the target's corner.
struct OverlayLine
{
  std::string text; // UTF-8
  float insetX = 10.0f;
  float insetY = 40.0f;
  OverlayCorner corner = OverlayCorner::BottomRight;
  std::array<float, 4> color{1.0f, 1.0f, 1.0f, 1.0f};
};

/// A filled rectangle of the development overlay (the frame-time graph of DBG-089; ADR-120), in unscaled pixels from the
/// target's top-left corner, drawn under the overlay's text at the same scale.
struct OverlayRect
{
  float left = 0.0f;
  float top = 0.0f;
  float width = 0.0f;
  float height = 0.0f;
  std::array<float, 4> color{1.0f, 1.0f, 1.0f, 1.0f};
};

/// A development dot (M1 task T15a): a figure's world position, its colour and its side in unscaled pixels.
struct DevelopmentDot
{
  Vec3 position;
  std::array<float, 4> color{1.0f, 1.0f, 1.0f, 1.0f};
  float sidePixels = 3.0f;
};

/// The textures the actor, effect, orb and particle passes read (ADR-009, ADR-121): each the bytes of a DDS file of the
/// content, by the index a QuadBatch names. They are uploaded once and kept across device rebuilds (RND-035).
struct SceneTextures
{
  std::vector<std::vector<std::byte>> files;
};

// RND-306: a quad's orientation mode.
enum class QuadOrientation : std::uint8_t
{
  CameraFacing,
  UprightCard,
  WorldOriented,
  Segment
};

// RND-305: where a quad's position lies on it.
enum class QuadAnchor : std::uint8_t
{
  Center,
  BottomCenter
};

/// One quad of the scene's passes (RND-305, RND-306), in world units and binary64: the renderer makes it relative to the
/// camera (WLD-246). Sizes are half-sizes.
struct SceneQuad
{
  Vec3 position; // the centre, the bottom-centre anchor, or a segment's start
  Vec3 endPoint; // a segment's end
  Vec3 axisRight{1.0, 0.0, 0.0};
  Vec3 axisUp{0.0, 1.0, 0.0};
  float halfWidth = 1.0f;
  float halfHeight = 1.0f;
  float blendStart = 0.0f;  // a world-oriented quad's billboard blend: from this camera distance, wu
  float blendLength = 0.0f; // over this distance; 0: no blend
  float rotationRadians = 0.0f;
  bool blendLeft = false; // the blend turns towards the camera's left (VFX-117)
  std::array<float, 4> color{1.0f, 1.0f, 1.0f, 1.0f};
  std::array<float, 4> endColor{1.0f, 1.0f, 1.0f, 1.0f};
  std::array<float, 4> uv{0.0f, 0.0f, 1.0f, 1.0f};
  QuadOrientation orientation = QuadOrientation::CameraFacing;
  QuadAnchor anchor = QuadAnchor::Center;
};

// The world passes that draw the scene's quads, meshes and triangles (RND-070), in their order.
enum class ScenePass : std::uint8_t
{
  Structures,     // pass 6: the structures' solid parts (RND-163, ADR-140)
  Actors,         // pass 7b
  Pointers,       // pass 7b after the cloth: the flag pointers over the banners (ADR-125)
  Workers,        // pass 7d
  StructureAlpha, // pass 8: the port pads, the stars, the lights and the structures' glows (RND-164 to RND-168, ADR-140)
  Effects,        // pass 10a
  Lasers,         // pass 10b
  Orbs,           // pass 11
  Routes,         // pass 12
  Particles,      // pass 15
  Count
};

/// One draw of a quad pass (ADR-009's M1 form, ADR-121): its catalogue entry, the texture of SceneTextures it reads,
/// the view test's radius about each quad's position, and its quads in the order they are drawn. The renderer writes
/// only the quads that pass the view test into the upload ring and draws them with one instanced call.
struct QuadBatch
{
  ScenePass pass = ScenePass::Workers;
  std::string_view entry;    // the catalogue key, a literal
  std::uint16_t texture = 0; // an index of SceneTextures::files
  float cullRadius = 0.0f;   // wu; 0: no view test
  float cutout = 0.0f;       // the cut-out threshold of a cut-out entry (RND-060)
  std::vector<SceneQuad> quads;
};

/// The meshes the scene's passes draw (ADR-019, ADR-122), cooked, by the index a MeshBatch names. They are uploaded once
/// and kept across device rebuilds (RND-035).
struct SceneMeshes
{
  std::vector<std::shared_ptr<const MeshAsset>> meshes;
};

/// One instance of a scene mesh (RND-324): its origin and the model's axes in world units and binary64, their lengths
/// its scale, and its team tint (RND-271).
struct SceneMeshInstance
{
  Vec3 position;
  Vec3 axisX{1.0, 0.0, 0.0};
  Vec3 axisY{0.0, 1.0, 0.0};
  Vec3 axisZ{0.0, 0.0, 1.0};
  std::array<float, 3> tint{1.0f, 1.0f, 1.0f}; // the tinted colour T × f
  float tintWeight = 0.0f;                     // the instance's weight: 0 for an instance of no team
  float brightness = 1.0f;                     // ADR-140: what its lit colour is multiplied by, lower for an inactive structure
};

/// One draw of a scene mesh (ADR-122): its catalogue entry, its mesh, the view test's radius about each instance's
/// position and its instances, drawn with one instanced call.
struct MeshBatch
{
  ScenePass pass = ScenePass::Actors;
  std::string_view entry;
  std::uint16_t mesh = 0;
  float cullRadius = 0.0f;
  std::vector<SceneMeshInstance> instances;
};

/// A vertex of a textured triangle list (the cloth flags of VFX-141), in world units and binary64: its texture
/// coordinate, and its colour, which tints the texture where the batch's mask is set.
struct SceneVertex
{
  Vec3 position;
  std::array<float, 2> uv{0.0f, 0.0f};
  std::array<float, 4> color{1.0f, 1.0f, 1.0f, 1.0f};
};

/// One draw of triangles (ADR-122, ADR-123): its catalogue entry, its texture and tint mask of SceneTextures, and its
/// vertices, three a triangle. A batch of vertex colours alone reads no texture; a screen-space batch gives its corners'
/// x and y in clip space, as the full-screen flash does (VFX-234).
struct TriangleBatch
{
  ScenePass pass = ScenePass::Actors;
  std::string_view entry;
  std::uint16_t texture = 0;
  std::uint16_t mask = 0;
  bool textured = true;
  bool screenSpace = false;
  std::vector<SceneVertex> vertices;
};

/// The game cursors' art (RND-366, ADR-119): each cursor's mask and its authored darkening shadow, as DDS files, by the
/// index a CursorQuad names. Single-channel masks at their display size with one level (O64).
struct CursorImages
{
  std::vector<std::vector<std::byte>> art;
  std::vector<std::vector<std::byte>> shadows; // one per art; empty for a cursor without a shadow
};

/// The cursor drawn in pass 20 (R-CTRL-352, RND-366): an image of CursorImages, its top-left corner and side in
/// physical pixels, and its colour; its shadow first, then the art.
struct CursorQuad
{
  std::uint8_t image = 0;
  float left = 0.0f;
  float top = 0.0f;
  float sidePixels = 0.0f;
  std::array<float, 4> color{1.0f, 1.0f, 1.0f, 1.0f};
};

// RND-357: the blend of an overlay quad, `overlay_alpha` or `overlay_additive`.
enum class OverlayBlend : std::uint8_t
{
  Alpha,
  Additive
};

/// A textured quad of the 2-D overlay (ADR-125): a world marker's layer in pass 16 (RND-356, RND-357) or a layer of the
/// cursor's badge in pass 20 (R-CTRL-366). Its mask is a texture of SceneTextures; its centre and half-size are in
/// physical pixels of the target from its top-left corner, and it turns clockwise on the screen by its rotation. Quads
/// are drawn in their order, and a run of the same texture and blend with one call.
struct OverlayQuad
{
  std::uint16_t texture = 0;
  OverlayBlend blend = OverlayBlend::Additive;
  float centerX = 0.0f;
  float centerY = 0.0f;
  float halfSize = 0.0f;
  float rotationRadians = 0.0f;
  std::array<float, 4> color{1.0f, 1.0f, 1.0f, 1.0f};
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
  // The development overlay (DBG-004, Debug builds): its face, its lines and rectangles, and the dots drawn in the world.
  std::shared_ptr<const FontAsset> overlayFont;
  std::vector<OverlayLine> overlay;
  std::vector<OverlayRect> overlayRects;
  std::vector<DevelopmentDot> dots;
  // The scene's quads (ADR-121): their textures, and the batches of each quad pass in the order drawn.
  std::shared_ptr<const SceneTextures> sceneTextures;
  std::vector<QuadBatch> quadBatches;
  std::shared_ptr<const SceneMeshes> sceneMeshes;
  std::vector<MeshBatch> meshBatches;
  std::vector<TriangleBatch> triangleBatches;
  // The world markers' layers of pass 16 and the cursor's badge, under the cursor in pass 20 (ADR-125), drawn with the
  // scene's textures in a world frame only.
  std::vector<OverlayQuad> markers;
  std::vector<OverlayQuad> cursorBadge;
  // ADR-127: the badge's text over it in pass 20, in its face at the text scale: the selected Workers' count in GameFont.
  std::shared_ptr<const FontAsset> badgeFont;
  std::vector<OverlayLine> badgeText;
  // The game cursor (ADR-119): its art, and the quad of this frame; none draws no cursor.
  std::shared_ptr<const CursorImages> cursorImages;
  std::optional<CursorQuad> cursor;
};

} // namespace Neuron
