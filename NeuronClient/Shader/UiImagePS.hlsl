#include "Ui.hlsli"

// UI-054: an image of the UI's set, a panel strip among them, read with linear filtering and wrapping on both axes, so a
// coordinate past 1 repeats it; its colour times the quad's.
float4 main(UiVertex _vertex) : SV_Target
{
  Texture2D<float4> image = ResourceDescriptorHeap[g_draw.indices.x];
  SamplerState linearWrap = SamplerDescriptorHeap[SAMPLER_LINEAR_WRAP];
  return image.Sample(linearWrap, _vertex.uv) * _vertex.color;
}
