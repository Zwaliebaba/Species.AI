#include "Ui.hlsli"

// UI-198 as O63 amends it: the face's coverage, read with the point sampler so that every texel covers k × k pixels,
// multiplies the colour and the alpha (UI-180), so an ink glyph darkens only where it covers; the blend state adds it,
// lays it over, or inks with it.
float4 main(UiVertex _vertex) : SV_Target
{
  Texture2D<float4> page = ResourceDescriptorHeap[g_draw.indices.x];
  SamplerState pointClamp = SamplerDescriptorHeap[SAMPLER_POINT_CLAMP];
  const float coverage = page.SampleLevel(pointClamp, _vertex.uv, 0.0).r;
  return _vertex.color * coverage;
}
