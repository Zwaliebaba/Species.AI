#include "Cursor.hlsli"

// The mask's mean over the pixel's footprint, from a 3 × 3 grid of bilinear taps across it; the colour is weighted by
// it. Drawn with Additive the art adds colour × mask, and drawn with Darkening the shadow multiplies the destination by
// one minus the mask (RND-058).
float4 main(CursorVertex _vertex) : SV_Target
{
  Texture2D<float4> mask = ResourceDescriptorHeap[g_draw.indices.x];
  SamplerState linearClamp = SamplerDescriptorHeap[SAMPLER_LINEAR_CLAMP];
  const float2 acrossX = ddx(_vertex.uv) / 3.0;
  const float2 acrossY = ddy(_vertex.uv) / 3.0;
  float coverage = 0.0;
  [unroll] for (int y = -1; y <= 1; ++y)
  {
    [unroll] for (int x = -1; x <= 1; ++x)
    {
      coverage += mask.SampleLevel(linearClamp, _vertex.uv + acrossX * x + acrossY * y, 0.0).r;
    }
  }
  coverage /= 9.0;
  return float4(_vertex.color.rgb * coverage, _vertex.color.a);
}
