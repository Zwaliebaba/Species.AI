#include "UnlitTextured.hlsli"

// VFX-217's smoke batch, `particle_darkening`: the destination is multiplied by one minus the source colour, so the
// dot's intensity scales the particle's shade (RGB × alpha ÷ the shade divisor, written by the game) and alpha is 0.
float4 main(QuadVertex _vertex) : SV_Target
{
  Texture2D<float4> texture = ResourceDescriptorHeap[g_draw.indices.x];
  SamplerState pointMagnify = SamplerDescriptorHeap[SAMPLER_POINT_MAGNIFY_CLAMP];
  const float mask = texture.Sample(pointMagnify, _vertex.uv).r;
  return float4(_vertex.color.rgb * mask, 0.0);
}
