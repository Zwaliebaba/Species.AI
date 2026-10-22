#include "DevelopmentRect.hlsli"

struct RectInstance // TextGlyphInstance, one per rectangle; its texture coordinates are not read
{
  float4 rectangle : RECTANGLE;
  float4 uv : TEXCOORD;
  float4 color : COLOR;
};

// Two triangles over the rectangle; the overlay draws with culling off.
static const float2 CORNERS[6] = {float2(0.0, 0.0), float2(1.0, 0.0), float2(0.0, 1.0), float2(0.0, 1.0), float2(1.0, 0.0), float2(1.0, 1.0)};

RectVertex main(uint _vertex : SV_VertexID, RectInstance _rect)
{
  const float2 corner = CORNERS[_vertex % 6];
  const float2 pixel = lerp(_rect.rectangle.xy, _rect.rectangle.zw, corner);
  RectVertex vertex;
  vertex.position = float4(pixel.x * g_pass.values[0].z * 2.0 - 1.0, 1.0 - pixel.y * g_pass.values[0].w * 2.0, 0.0, 1.0);
  vertex.color = _rect.color;
  return vertex;
}
