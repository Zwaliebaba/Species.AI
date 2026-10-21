// The development dots of M1 task T15a: a square of a few pixels at each figure until its crowd is drawn (T16), tested
// against the scene's depth so that the terrain hides it.
#include "../ShaderLayouts.h"

ConstantBuffer<FrameConstants> g_frame : register(b0);

struct DotVertex
{
  float4 position : SV_Position;
  nointerpolation float4 color : COLOR0;
};
