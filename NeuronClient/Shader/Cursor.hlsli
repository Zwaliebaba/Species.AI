// The game cursors of pass 20 (RND-366, ADR-119): a cursor's mask, or its authored darkening shadow, over its quad on
// the back buffer. The masks are one level at 128 px drawn smaller, so the pixel shader filters the texels a pixel
// covers instead of reading one bilinear tap.
#include "../ShaderLayouts.h"

ConstantBuffer<PassConstants> g_pass : register(b1); // values[0]: the target's width and height, and their reciprocals
ConstantBuffer<DrawConstants> g_draw : register(b2); // indices.x: the mask

struct CursorVertex
{
  float4 position : SV_Position;
  float2 uv : TEXCOORD0;
  nointerpolation float4 color : COLOR0;
};
