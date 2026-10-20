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

// ADR-006: the per-frame constants, register b0. Positions are relative to the camera (WLD-246), so the view matrix
// holds the camera's rotation alone and the camera stands at the origin of the coordinates every shader sees.
struct FrameConstants
{
  Float4x4 view;
  Float4x4 projection;
  Float4x4 viewProjection;
  Float4 cameraPosition; // xyz: the camera's world position, wrapped into a float's range for effects that need it
  Float4 fogColor;       // rgb, a: 1 when fog is on
  Float4 fogRange;       // x: start, y: end, in world units from the camera
  Float4 keyDirection;   // xyz: the direction the key light travels, normalised
  Float4 keyColor;
  Float4 fillDirection;
  Float4 fillColor;
  Float4 ambientColor;
  Float4 time;     // x: presentation time in seconds, y: the displayed tick's fraction
  Float4 viewport; // x, y: size in pixels; z, w: their reciprocals
};

// ADR-006: the per-pass constants, register b1, which a pass fills as it needs.
struct PassConstants
{
  Float4 values[8];
};

// ADR-006: the per-draw root constants, register b2: the draw's transform from model to camera-relative space, its tint,
// and the heap indices of what it reads.
struct DrawConstants
{
  Float3x4 world;
  Float4 tint;
  Uint4 indices;
};

#if !defined(__HLSL_VERSION)
static_assert(sizeof(FrameConstants) == 3 * 64 + 10 * 16);
static_assert(sizeof(PassConstants) == 8 * 16);
static_assert(sizeof(DrawConstants) == 48 + 16 + 16);
inline constexpr std::uint32_t DRAW_CONSTANT_DWORDS = sizeof(DrawConstants) / 4;
} // namespace Neuron
#endif
