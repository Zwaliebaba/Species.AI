// The unlit textured quads of the actor, effect, orb and particle passes (RND-263, RND-305, RND-306; ADR-009): one
// QuadInstance per quad, expanded in the vertex shader, read with the batch's texture.
#include "../ShaderLayouts.h"

ConstantBuffer<FrameConstants> g_frame : register(b0);
ConstantBuffer<PassConstants> g_pass : register(b1); // values[0].x: the cut-out threshold (RND-060)
ConstantBuffer<DrawConstants> g_draw : register(b2); // indices.x: the batch's texture

struct QuadVertex
{
  float4 position : SV_Position;
  float2 uv : TEXCOORD0;
  float4 color : COLOR0;
};
