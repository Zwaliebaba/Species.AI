#include "UnlitTextured.hlsli"

// A colour texture times the quad's colour, its alpha the texel's times the quad's (RND-263).
float4 main(QuadVertex _vertex) : SV_Target
{
  Texture2D<float4> texture = ResourceDescriptorHeap[g_draw.indices.x];
  SamplerState linearClamp = SamplerDescriptorHeap[SAMPLER_LINEAR_CLAMP];
  const float4 texel = texture.Sample(linearClamp, _vertex.uv);
  return float4(texel.rgb * _vertex.color.rgb, texel.a * _vertex.color.a);
}
