#include "Cloth.hlsli"

// VFX-141: the cloth filtered linearly through its mips; where the mask is set, its grey levels take the tint.
float4 main(ClothVertex _vertex) : SV_Target
{
  Texture2D<float4> cloth = ResourceDescriptorHeap[g_draw.indices.x];
  Texture2D<float4> mask = ResourceDescriptorHeap[g_draw.indices.y];
  SamplerState linearClamp = SamplerDescriptorHeap[SAMPLER_LINEAR_CLAMP];
  const float4 texel = cloth.Sample(linearClamp, _vertex.uv);
  const float tinted = mask.Sample(linearClamp, _vertex.uv).r;
  return float4(texel.rgb * lerp(float3(1.0, 1.0, 1.0), _vertex.color.rgb, tinted), texel.a * _vertex.color.a);
}
