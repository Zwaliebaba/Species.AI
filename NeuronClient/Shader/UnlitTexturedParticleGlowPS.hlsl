#include "UnlitTextured.hlsli"

// VFX-217's glow batch, `particle_additive`: fx.particle_dot through its point-magnified view (AST-027), its intensity
// scaling the particle's alpha; the colour is the particle's.
float4 main(QuadVertex _vertex) : SV_Target
{
  Texture2D<float4> texture = ResourceDescriptorHeap[g_draw.indices.x];
  SamplerState pointMagnify = SamplerDescriptorHeap[SAMPLER_POINT_MAGNIFY_CLAMP];
  const float mask = texture.Sample(pointMagnify, _vertex.uv).r;
  return float4(_vertex.color.rgb, mask * _vertex.color.a);
}
