#include "UnlitTextured.hlsli"

// A BC4 mask (ADR-115): its intensity, in .r, scales the quad's alpha; the colour is the quad's (RND-418's glows).
float4 main(QuadVertex _vertex) : SV_Target
{
  Texture2D<float4> texture = ResourceDescriptorHeap[g_draw.indices.x];
  SamplerState linearClamp = SamplerDescriptorHeap[SAMPLER_LINEAR_CLAMP];
  const float mask = texture.Sample(linearClamp, _vertex.uv).r;
  return float4(_vertex.color.rgb, mask * _vertex.color.a);
}
