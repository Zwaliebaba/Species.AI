#pragma once

// The layouts that C++ and HLSL share (ADR-003, ADR-006): every constant-buffer and root-constant layout is declared
// here once, and both compilers read this file. Matrices are row-major in both, since the shaders compile with -Zpr
// (RND-260, RND-262). Every field is a whole 16-byte register, so the two packings cannot differ.

#if defined(__HLSL_VERSION)

typedef float4 Float4;
typedef float4x4 Float4x4;
typedef float3x4 Float3x4;
typedef uint4 Uint4;
#   define LAYOUT_CONSTANT static const uint

#else

#   include <DirectXMath.h>

#   include <cstdint>

namespace Neuron
{

using Float4 = DirectX::XMFLOAT4;
using Float4x4 = DirectX::XMFLOAT4X4;
using Float3x4 = DirectX::XMFLOAT3X4;
struct Uint4
{
  std::uint32_t x = 0;
  std::uint32_t y = 0;
  std::uint32_t z = 0;
  std::uint32_t w = 0;
};
#   define LAYOUT_CONSTANT inline constexpr std::uint32_t

#endif

// ADR-018: the closed sampler set at its fixed indices of the sampler heap.
LAYOUT_CONSTANT SAMPLER_LINEAR_CLAMP = 0;
LAYOUT_CONSTANT SAMPLER_LINEAR_WRAP = 1;
LAYOUT_CONSTANT SAMPLER_POINT_CLAMP = 2;
LAYOUT_CONSTANT SAMPLER_POINT_MAGNIFY_CLAMP = 3;
// ADR-006: index 0 of the resource heap holds a valid default view.
LAYOUT_CONSTANT DEFAULT_RESOURCE_INDEX = 0;

// ADR-006: the per-frame constants, register b0, written once at the start of the frame (RND-057). Positions are relative
// to the camera (WLD-246, ADR-115): the view matrix holds the camera's rotation alone, the camera stands at the origin of
// the coordinates every shader sees, and a pixel's view depth is the dot product of its position with cameraFront.
struct FrameConstants
{
  Float4x4 view;
  Float4x4 projection;
  Float4x4 viewProjection;
  Float4 cameraFront; // xyz: the unit direction the camera looks along
  Float4 fogColor;    // rgb: the level fog's colour (RND-222)
  Float4 fogRange;    // x: start, y: end, in wu of view depth; z: 1 ÷ (end − start)
  Float4 keyTowards;  // xyz: the unit direction towards the key light (RND-226)
  Float4 keyColor;    // rgb: its diffuse and specular colour
  Float4 fillTowards; // the fill light, as the key
  Float4 fillColor;
  Float4 time;     // x: the smooth presentation time in seconds (R-SIM-570)
  Float4 viewport; // x, y: the render size in pixels; z, w: their reciprocals
};

// ADR-006: the per-pass constants, register b1, which a pass fills as it needs.
struct PassConstants
{
  Float4 values[8];
};

// ADR-006: the per-draw root constants, register b2: the draw's transform from model to camera-relative space (the
// transpose of the row-vector transform, applied as mul(world, float4(p, 1)); RND-262), its tint, and the heap indices
// of what it reads.
struct DrawConstants
{
  Float3x4 world;
  Float4 tint;
  Uint4 indices;
};

// RND-150 to RND-154 and ADR-115: one quad of the sky, a grid line or a cloud layer, drawn as an instance of one draw.
// The quad lies at a height over a rectangle, both relative to the camera, and faces downwards.
struct SkyLayerInstance
{
  Float4 rectangle; // x, y: the minimum corner's x and z; z, w: the maximum corner's x and z
  Float4 color;     // rgba: the layer's colour
  Float4 uv;        // xy: the texture coordinate at the minimum corner; zw: its change across the rectangle
  Float4 shape;     // x: the height; y, z: the layer's black fog start and end in wu of view depth
  Uint4 sampling;   // x: the texture's heap index; y: the sampler's index; z: the SKY_* flags
};

// SkyLayerInstance.sampling.z: how a layer reads its texture.
LAYOUT_CONSTANT SKY_WRAP_IN_SHADER = 1; // the coordinate wraps before a clamping sampler reads it (RND-154)
LAYOUT_CONSTANT SKY_LEVEL_ZERO = 2;     // only the first mip level is read (RND-153)
LAYOUT_CONSTANT SKY_ALONG_Z = 4;        // u runs along z and v along x: a grid line parallel to z (RND-151)

// DBG-095 and O63: one glyph of the development overlay's text, drawn in pass 29 at a whole-number scale.
struct TextGlyphInstance
{
  Float4 rectangle; // left, top, right and bottom on the target, in whole pixels
  Float4 uv;        // the glyph's rectangle on the face's page, from 0 to 1
  Float4 color;
};

// M1 task T15a: a development dot at a figure's position until its crowd is drawn (T16), relative to the camera.
struct DotInstance
{
  Float4 center; // xyz: the position relative to the camera; w: the dot's side in pixels
  Float4 color;
};

// RND-262, RND-305 and RND-306 (ADR-009): one quad of the actor, effect, orb and particle passes, expanded in the vertex
// shader from its orientation mode. Positions are relative to the camera (WLD-246); sizes are half-sizes in wu.
struct QuadInstance
{
  Float4 position;  // xyz: the centre, the anchor of a QUAD_ANCHOR_BOTTOM quad, or a segment's start; w: the half-width
  Float4 endPoint;  // xyz: a segment's end; w: the half-height
  Float4 axisRight; // xyz: a world-oriented quad's right axis; w: its billboard blend's start, wu from the camera
  Float4 axisUp;    // xyz: its up axis; w: the blend's length in wu, 0 for no blend
  Float4 color;     // rgba; a segment's colour at its start
  Float4 endColor;  // a segment's colour at its end
  Float4 uv;        // the texture's rectangle: u and v at the left and top, then at the right and bottom
  Uint4 shape;      // x: the orientation mode; y: the anchor; z: the rotation in radians, as its float's bits; w: QUAD_* flags
};

// RND-262, RND-324 (ADR-122): one instance of a mesh, its model-to-camera-relative transform in the form of
// DrawConstants::world, and its team tint (RND-271).
struct MeshInstance
{
  Float4 row0;
  Float4 row1;
  Float4 row2;
  Float4 tint;  // rgb: the tinted colour T × f; a: the instance's tint weight
  Float4 shade; // x: the instance's brightness, which an inactive structure lowers (ADR-140)
};

// ADR-122: a vertex of the scene's textured triangles, relative to the camera.
struct TriangleVertex
{
  Float4 position; // xyz
  Float4 uv;       // xy
  Float4 color;
};

// ADR-125: one textured quad of the 2-D overlay, the world markers of pass 16 and the cursor's badge of pass 20, in
// physical pixels of the target from its top-left corner.
struct OverlayQuadInstance
{
  Float4 placement; // xy: the centre; z: the half-size; w: the turn in radians, clockwise on the screen
  Float4 color;     // rgba
};

// QuadInstance.shape.x: the orientation modes of RND-306.
LAYOUT_CONSTANT QUAD_CAMERA_FACING = 0;
LAYOUT_CONSTANT QUAD_UPRIGHT_CARD = 1;
LAYOUT_CONSTANT QUAD_WORLD_ORIENTED = 2;
LAYOUT_CONSTANT QUAD_SEGMENT = 3;
// QuadInstance.shape.y: the anchors of RND-305.
LAYOUT_CONSTANT QUAD_ANCHOR_CENTER = 0;
LAYOUT_CONSTANT QUAD_ANCHOR_BOTTOM = 1;
// QuadInstance.shape.w: a world-oriented quad blends towards the camera's left rather than its right, so that a card
// seen from behind is not drawn mirrored (VFX-117).
LAYOUT_CONSTANT QUAD_BLEND_LEFT = 1;
// DrawConstants.indices.z of a vertex-coloured triangle batch (ADR-123): its corners are in clip space, not relative to
// the camera, as the full-screen flash's are (VFX-234).
LAYOUT_CONSTANT TRIANGLE_SCREEN_SPACE = 1;

#if !defined(__HLSL_VERSION)
static_assert(sizeof(FrameConstants) == 3 * sizeof(Float4x4) + 9 * sizeof(Float4));
static_assert(sizeof(PassConstants) == 8 * sizeof(Float4));
static_assert(sizeof(DrawConstants) == sizeof(Float3x4) + sizeof(Float4) + sizeof(Uint4));
static_assert(sizeof(SkyLayerInstance) == 4 * sizeof(Float4) + sizeof(Uint4));
static_assert(sizeof(TextGlyphInstance) == 3 * sizeof(Float4));
static_assert(sizeof(DotInstance) == 2 * sizeof(Float4));
static_assert(sizeof(QuadInstance) == 7 * sizeof(Float4) + sizeof(Uint4));
static_assert(sizeof(MeshInstance) == 5 * sizeof(Float4));
static_assert(sizeof(TriangleVertex) == 3 * sizeof(Float4));
static_assert(sizeof(OverlayQuadInstance) == 2 * sizeof(Float4));
inline constexpr std::uint32_t DRAW_CONSTANT_DWORDS = sizeof(DrawConstants) / 4;
} // namespace Neuron
#endif
