#include "DevelopmentText.hlsli"

struct GlyphInstance // TextGlyphInstance, one per glyph
{
  float4 rectangle : RECTANGLE;
  float4 uv : TEXCOORD;
  float4 color : COLOR;
};

// Two triangles over the glyph's rectangle; the overlay draws with culling off.
static const float2 CORNERS[6] = {float2(0.0, 0.0), float2(1.0, 0.0), float2(0.0, 1.0), float2(0.0, 1.0), float2(1.0, 0.0), float2(1.0, 1.0)};

TextVertex main(uint _vertex : SV_VertexID, GlyphInstance _glyph)
{
  const float2 corner = CORNERS[_vertex % 6];
  const float2 pixel = lerp(_glyph.rectangle.xy, _glyph.rectangle.zw, corner);
  TextVertex vertex;
  vertex.position = float4(pixel.x * g_pass.values[0].z * 2.0 - 1.0, 1.0 - pixel.y * g_pass.values[0].w * 2.0, 0.0, 1.0);
  vertex.uv = lerp(_glyph.uv.xy, _glyph.uv.zw, corner);
  vertex.color = _glyph.color;
  return vertex;
}
