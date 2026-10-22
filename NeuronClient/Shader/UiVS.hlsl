#include "Ui.hlsli"

struct UiInstance // TextGlyphInstance, one per quad
{
  float4 rectangle : RECTANGLE;
  float4 uv : TEXCOORD;
  float4 color : COLOR;
};

// Two triangles over the quad's rectangle; the UI draws with culling off.
static const float2 CORNERS[6] = {float2(0.0, 0.0), float2(1.0, 0.0), float2(0.0, 1.0), float2(0.0, 1.0), float2(1.0, 0.0), float2(1.0, 1.0)};

UiVertex main(uint _vertex : SV_VertexID, UiInstance _quad)
{
  const float2 corner = CORNERS[_vertex % 6];
  const float2 pixel = lerp(_quad.rectangle.xy, _quad.rectangle.zw, corner);
  UiVertex vertex;
  vertex.position = float4(pixel.x * g_pass.values[0].z * 2.0 - 1.0, 1.0 - pixel.y * g_pass.values[0].w * 2.0, 0.0, 1.0);
  vertex.uv = lerp(_quad.uv.xy, _quad.uv.zw, corner);
  vertex.color = _quad.color;
  return vertex;
}
