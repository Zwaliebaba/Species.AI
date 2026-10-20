#include "SeaWaves.hlsli"

// Alpha 1, so the additive blend adds the colour in full (RND-146).
float4 main(SeaWavesVertex _vertex) : SV_Target
{
  const float fog = saturate((g_frame.fogRange.y - _vertex.depth) * g_frame.fogRange.z);
  return float4(lerp(g_frame.fogColor.rgb, _vertex.color, fog), 1.0);
}
