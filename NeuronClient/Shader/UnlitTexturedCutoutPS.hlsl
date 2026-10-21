#include "UnlitTextured.hlsli"

// RND-060, RND-272, RND-413: a crowd card's hard cut-out edge. A texel at or below the threshold is discarded; the rest
// takes the quad's alpha alone, so a card fades by its colour and never by soft texture edges.
float4 main(QuadVertex _vertex) : SV_Target
{
  Texture2D<float4> texture = ResourceDescriptorHeap[g_draw.indices.x];
  SamplerState linearClamp = SamplerDescriptorHeap[SAMPLER_LINEAR_CLAMP];
  const float4 texel = texture.Sample(linearClamp, _vertex.uv);
  if (texel.a <= g_pass.values[0].x)
  {
    discard;
  }
  return float4(texel.rgb * _vertex.color.rgb, _vertex.color.a);
}
