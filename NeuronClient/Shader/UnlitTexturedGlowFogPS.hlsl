#include "UnlitTextured.hlsli"

// RND-167, RND-168 (ADR-140): `glow_additive_fog`, the additive glows of pass 8 (the port stars, the ownership lights,
// the Wellspring's glow and the completion flash): a BC4 mask scaling the quad's alpha, faded with the level fog, which
// is black, so an additive glow goes out as the terrain goes dark.
float4 main(QuadVertex _vertex) : SV_Target
{
  Texture2D<float4> texture = ResourceDescriptorHeap[g_draw.indices.x];
  SamplerState linearClamp = SamplerDescriptorHeap[SAMPLER_LINEAR_CLAMP];
  const float mask = texture.Sample(linearClamp, _vertex.uv).r;
  const float fog = saturate((g_frame.fogRange.y - _vertex.depth) * g_frame.fogRange.z);
  return float4(_vertex.color.rgb, mask * _vertex.color.a * fog);
}
