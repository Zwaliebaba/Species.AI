#include "Ui.hlsli"

// UI-054: an image of the UI's set, a panel strip among them, read with linear filtering and wrapping on both axes, so a
// coordinate past 1 repeats it; its color times the quad's. ADR-149: a mask of one channel (g_draw.indices.y), such as
// the highlight beam, is white with its channel as its alpha.
float4 main(UiVertex _vertex) : SV_Target
{
  Texture2D<float4> image = ResourceDescriptorHeap[g_draw.indices.x];
  SamplerState linearWrap = SamplerDescriptorHeap[SAMPLER_LINEAR_WRAP];
  const float4 texel = image.Sample(linearWrap, _vertex.uv);
  const float4 color = g_draw.indices.y != 0 ? float4(1.0, 1.0, 1.0, texel.r) : texel;
  return color * _vertex.color;
}
