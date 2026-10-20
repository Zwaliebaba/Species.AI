#include "Sky.hlsli"

// RND-265: the colour times the texel, fogged to black by the layer's own range; the alpha is the colour's, so the
// additive blend adds rgb × a. A BC4 mask's intensity is .r and its alpha is 1 (ADR-115).
float4 main(SkyVertex _vertex) : SV_Target
{
  Texture2D<float4> layer = ResourceDescriptorHeap[NonUniformResourceIndex(_vertex.sampling.x)];
  SamplerState layerSampler = SamplerDescriptorHeap[NonUniformResourceIndex(_vertex.sampling.y)];
  const float2 uv = (_vertex.sampling.z & SKY_WRAP_IN_SHADER) != 0 ? frac(_vertex.uv) : _vertex.uv;
  const float texel = (_vertex.sampling.z & SKY_LEVEL_ZERO) != 0 ? layer.SampleLevel(layerSampler, uv, 0.0).r : layer.Sample(layerSampler, uv).r;
  const float fog = saturate((_vertex.fogEnd - _vertex.depth) * _vertex.fog.y);
  return float4(_vertex.color.rgb * texel * fog, _vertex.color.a);
}
