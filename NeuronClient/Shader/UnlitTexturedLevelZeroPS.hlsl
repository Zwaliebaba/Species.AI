#include "UnlitTextured.hlsli"

// RND-339: a laser bolt's BC4 beam read at its first mip level alone, with linear clamp-to-edge filtering, u along the
// bolt and v across it; its intensity scales the segment's alpha, and the colour is the segment's (ADR-123).
float4 main(QuadVertex _vertex) : SV_Target
{
  Texture2D<float4> texture = ResourceDescriptorHeap[g_draw.indices.x];
  SamplerState linearClamp = SamplerDescriptorHeap[SAMPLER_LINEAR_CLAMP];
  const float mask = texture.SampleLevel(linearClamp, _vertex.uv, 0.0).r;
  return float4(_vertex.color.rgb, mask * _vertex.color.a);
}
