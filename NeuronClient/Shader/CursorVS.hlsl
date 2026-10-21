#include "Cursor.hlsli"

struct CursorInput // TextGlyphInstance, one per cursor layer
{
  float4 rectangle : RECTANGLE;
  float4 uv : TEXCOORD;
  float4 color : COLOR;
};

static const float2 CORNERS[6] = {float2(0.0, 0.0), float2(1.0, 0.0), float2(0.0, 1.0), float2(0.0, 1.0), float2(1.0, 0.0), float2(1.0, 1.0)};

CursorVertex main(uint _vertex : SV_VertexID, CursorInput _cursor)
{
  const float2 corner = CORNERS[_vertex % 6];
  const float2 pixel = lerp(_cursor.rectangle.xy, _cursor.rectangle.zw, corner);
  CursorVertex vertex;
  vertex.position = float4(pixel.x * g_pass.values[0].z * 2.0 - 1.0, 1.0 - pixel.y * g_pass.values[0].w * 2.0, 0.0, 1.0);
  vertex.uv = lerp(_cursor.uv.xy, _cursor.uv.zw, corner);
  vertex.color = _cursor.color;
  return vertex;
}
