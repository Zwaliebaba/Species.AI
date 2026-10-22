#include "SeaFlat.hlsli"

float4 main(SeaFlatVertex _vertex) : SV_Target
{
  Texture2D<float4> tile = ResourceDescriptorHeap[g_draw.indices.x];
  Texture2D<float4> mask = ResourceDescriptorHeap[g_draw.indices.y];
  SamplerState linearWrap = SamplerDescriptorHeap[SAMPLER_LINEAR_WRAP];
  SamplerState linearClamp = SamplerDescriptorHeap[SAMPLER_LINEAR_CLAMP];
  // RND-268's product of a white vertex colour, the tile and the grey mask.
  const float3 color = tile.Sample(linearWrap, _vertex.tile).rgb * mask.Sample(linearClamp, _vertex.mask).r;
  const float fog = saturate((g_frame.fogRange.y - _vertex.depth) * g_frame.fogRange.z);
  // ADR-144: outside the view mask, darkened towards black by the mask's fog.
  float clear = 1.0;
  if (g_frame.viewMask.w > 0.0)
  {
    Texture2D<float4> viewMask = ResourceDescriptorHeap[g_frame.viewMaskIndex.x];
    const float2 at = (_vertex.ground - g_frame.viewMask.xy) * g_frame.viewMask.z;
    clear = 1.0 - g_frame.viewMask.w * (1.0 - viewMask.SampleLevel(linearClamp, at, 0.0).r);
  }
  return float4(lerp(g_frame.fogColor.rgb, color, fog) * clear, 1.0);
}
