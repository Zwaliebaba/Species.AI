// The final pass (RND-275, ADR-012): a triangle that covers the back buffer and reads the resolved scene.
#include "../ShaderLayouts.h"

ConstantBuffer<PassConstants> g_pass : register(b1);
ConstantBuffer<DrawConstants> g_draw : register(b2);

struct ScreenVertex
{
  float4 position : SV_Position;
  float2 uv : TEXCOORD0;
};
