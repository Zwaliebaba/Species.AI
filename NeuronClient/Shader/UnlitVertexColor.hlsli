// The vertex-coloured triangles of pass 10a (RND-263, RND-333, VFX-234; ADR-123): a shockwave ring's disc, relative to
// the camera, or the full-screen flash, whose corners are given in clip space.
#include "../ShaderLayouts.h"

ConstantBuffer<FrameConstants> g_frame : register(b0);
ConstantBuffer<DrawConstants> g_draw : register(b2); // indices.z: TRIANGLE_SCREEN_SPACE for clip-space corners

struct ColorVertex
{
  float4 position : SV_Position;
  float4 color : COLOR0;
};
