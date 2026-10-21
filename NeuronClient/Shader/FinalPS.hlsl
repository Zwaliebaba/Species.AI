#include "Final.hlsli"

// The resolved scene, scaled to the back buffer by the linear clamp sampler and multiplied by the brightness
// (g_pass.values[0].x, RND-275); HDR output is off (ADR-014), so the result is written as it is.
float4 main(ScreenVertex _vertex) : SV_Target
{
  Texture2D<float4> scene = ResourceDescriptorHeap[g_draw.indices.x];
  SamplerState linearClamp = SamplerDescriptorHeap[SAMPLER_LINEAR_CLAMP];
  const float3 color = scene.SampleLevel(linearClamp, _vertex.uv, 0.0).rgb * g_pass.values[0].x;
  return float4(saturate(color), 1.0);
}
