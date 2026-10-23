#include "Ui.hlsli"

struct UiInstance // UiQuadInstance, one per quad
{
  float4 rectangle : RECTANGLE;
  float4 uv : TEXCOORD;
  float4 colorFrom : COLOR0;
  float4 colorTo : COLOR1;
  uint4 shape : SHAPE;
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
  // UI-184, ADR-149: a gradient from the top edge to the bottom, or from the left edge to the right.
  const float along = _quad.shape.x == UI_GRADIENT_ACROSS ? corner.x : corner.y;
  vertex.color = along < 0.5 ? _quad.colorFrom : _quad.colorTo;
  return vertex;
}
