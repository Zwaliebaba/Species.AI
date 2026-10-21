// The textured quads of the 2-D overlay (ADR-125): the world markers of pass 16 (RND-356, RND-357) and the cursor's
// badge of pass 20 (R-CTRL-366), each a single-channel mask over a square turned about its centre. The masks are drawn
// at about their size or smaller, so the pixel shaders filter the texels a pixel covers, as the cursor's does.
#include "../ShaderLayouts.h"

ConstantBuffer<PassConstants> g_pass : register(b1); // values[0]: the target's width and height, and their reciprocals
ConstantBuffer<DrawConstants> g_draw : register(b2); // indices.x: the mask

struct OverlayVertex
{
  float4 position : SV_Position;
  float2 uv : TEXCOORD0;
  nointerpolation float4 color : COLOR0;
};

// The mask's mean over the pixel's footprint, from a 3 × 3 grid of bilinear taps across it.
float Coverage(float2 _uv)
{
  Texture2D<float4> mask = ResourceDescriptorHeap[g_draw.indices.x];
  SamplerState linearClamp = SamplerDescriptorHeap[SAMPLER_LINEAR_CLAMP];
  const float2 acrossX = ddx(_uv) / 3.0;
  const float2 acrossY = ddy(_uv) / 3.0;
  float coverage = 0.0;
  [unroll] for (int y = -1; y <= 1; ++y)
  {
    [unroll] for (int x = -1; x <= 1; ++x)
    {
      coverage += mask.SampleLevel(linearClamp, _uv + acrossX * x + acrossY * y, 0.0).r;
    }
  }
  return coverage / 9.0;
}
