// The coastline waves (RND-144 to RND-146): the wave mesh of this frame, unlit, added with level fog.
#include "../ShaderLayouts.h"

ConstantBuffer<FrameConstants> g_frame : register(b0);
ConstantBuffer<DrawConstants> g_draw : register(b2);

struct SeaWavesVertex
{
  float4 position : SV_Position;
  nointerpolation float3 color : COLOR0; // each wave triangle's facet colour (RND-145)
  float depth : TEXCOORD0;
  float2 ground : TEXCOORD1; // x and z relative to the camera, where the view mask is read (ADR-144)
};
