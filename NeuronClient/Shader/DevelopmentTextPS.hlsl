#include "DevelopmentText.hlsli"

// The page holds coverage in its one channel; the colour is blended by it.
float4 main(TextVertex _vertex) : SV_Target
{
  Texture2D<float4> page = ResourceDescriptorHeap[g_draw.indices.x];
  SamplerState pointClamp = SamplerDescriptorHeap[SAMPLER_POINT_CLAMP];
  const float coverage = page.SampleLevel(pointClamp, _vertex.uv, 0.0).r;
  return float4(_vertex.color.rgb, _vertex.color.a * coverage);
}
