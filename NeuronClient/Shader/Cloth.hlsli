// The cloth flags (VFX-141; ADR-122): textured triangles whose team area, which the tint mask marks, takes the vertex
// colour (ADR-020, AST-022).
#include "../ShaderLayouts.h"

ConstantBuffer<FrameConstants> g_frame : register(b0);
ConstantBuffer<DrawConstants> g_draw : register(b2); // indices.x: the cloth's texture; indices.y: its tint mask

struct ClothVertex
{
  float4 position : SV_Position;
  float2 uv : TEXCOORD0;
  float4 color : COLOR0;
};
