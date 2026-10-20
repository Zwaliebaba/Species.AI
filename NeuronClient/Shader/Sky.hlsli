// The sky (RND-150 to RND-154, RND-265): the grid lines and the cloud layers as instances of one draw, each a quad
// facing downwards, added with its own black fog.
#include "../ShaderLayouts.h"

ConstantBuffer<FrameConstants> g_frame : register(b0);

struct SkyVertex
{
  float4 position : SV_Position;
  float2 uv : TEXCOORD0;
  float depth : TEXCOORD1;
  nointerpolation float4 color : COLOR0;
  nointerpolation float2 fog : TEXCOORD2; // the layer's fog start and 1 ÷ (end − start)
  nointerpolation float fogEnd : TEXCOORD3;
  nointerpolation uint3 sampling : TEXCOORD4;
};
