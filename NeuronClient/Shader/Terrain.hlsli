// The terrain pass (RND-136, RND-266): the lit facets and the wire grid in one draw per chunk, with level fog.
#include "../ShaderLayouts.h"

ConstantBuffer<FrameConstants> g_frame : register(b0);
ConstantBuffer<PassConstants> g_pass : register(b1); // values[0]: the grid's specular colour (rgb) and shininess (a)
ConstantBuffer<DrawConstants> g_draw : register(b2); // indices.x: fx.terrain_grid

struct TerrainVertex
{
  float4 position : SV_Position;
  nointerpolation float4 color : COLOR0; // every corner of a facet holds its colour and normal (ADR-020)
  nointerpolation float3 normal : NORMAL0;
  float2 grid : TEXCOORD0;
  float depth : TEXCOORD1; // view depth, the fog coordinate (RND-222)
};
