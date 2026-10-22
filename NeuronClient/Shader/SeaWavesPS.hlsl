#include "SeaWaves.hlsli"

// Alpha 1, so the additive blend adds the colour in full (RND-146); outside the view mask the colour is darkened towards
// black by the mask's fog, as the ground under it is (ADR-144).
float4 main(SeaWavesVertex _vertex) : SV_Target
{
  const float fog = saturate((g_frame.fogRange.y - _vertex.depth) * g_frame.fogRange.z);
  float clear = 1.0;
  if (g_frame.viewMask.w > 0.0)
  {
    Texture2D<float4> viewMask = ResourceDescriptorHeap[g_frame.viewMaskIndex.x];
    SamplerState linearClamp = SamplerDescriptorHeap[SAMPLER_LINEAR_CLAMP];
    const float2 at = (_vertex.ground - g_frame.viewMask.xy) * g_frame.viewMask.z;
    clear = 1.0 - g_frame.viewMask.w * (1.0 - viewMask.SampleLevel(linearClamp, at, 0.0).r);
  }
  return float4(lerp(g_frame.fogColor.rgb, _vertex.color, fog) * clear, 1.0);
}
