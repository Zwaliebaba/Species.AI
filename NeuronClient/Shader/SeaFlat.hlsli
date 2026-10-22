// The flat sea layer (RND-141 to RND-143, RND-268): the sea tile times the shore mask, unlit, with level fog.
#include "../ShaderLayouts.h"

ConstantBuffer<FrameConstants> g_frame : register(b0);
// values[0]: x, the tile's repeats across the plane; y, 1 ÷ its scroll period; z, 1 ÷ the plane's side
ConstantBuffer<PassConstants> g_pass : register(b1);
ConstantBuffer<DrawConstants> g_draw : register(b2); // indices.x: terrain.sea_tile; indices.y: the shore mask

struct SeaFlatVertex
{
  float4 position : SV_Position;
  float2 tile : TEXCOORD0;
  float2 mask : TEXCOORD1;
  float depth : TEXCOORD2;
  float2 ground : TEXCOORD3; // x and z relative to the camera, where the view mask is read (ADR-144)
};
