#include "Cloth.hlsli"

struct Vertex // TriangleVertex
{
  float4 position : POSITION;
  float4 uv : TEXCOORD;
  float4 color : COLOR;
};

ClothVertex main(Vertex _vertex)
{
  ClothVertex output;
  output.position = mul(float4(_vertex.position.xyz, 1.0), g_frame.viewProjection);
  output.uv = _vertex.uv.xy;
  output.color = _vertex.color;
  return output;
}
