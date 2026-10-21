#include "MeshLit.hlsli"

struct Vertex // a cooked MeshVertex (AST-076)
{
  float3 position : POSITION;
  float3 normal : NORMAL;
  float4 color : COLOR;
  uint2 tintCorner : TINT; // x: the triangle's tint weight, 0 to 255; y: its corner
};

struct Instance // MeshInstance
{
  float4 row0 : WORLD0;
  float4 row1 : WORLD1;
  float4 row2 : WORLD2;
  float4 tint : TINTCOLOR;
};

MeshLitVertex main(Vertex _vertex, Instance _instance)
{
  const float3x4 world = float3x4(_instance.row0, _instance.row1, _instance.row2);
  const float3 position = mul(world, float4(_vertex.position, 1.0));
  MeshLitVertex output;
  output.position = mul(float4(position, 1.0), g_frame.viewProjection);
  // RND-324: the facet normal through the instance's axes, renormalised, since the axes carry its scale.
  output.normal = normalize(mul((float3x3)world, _vertex.normal));
  // RND-271: clamp(c + w × (T × f − c), 0, 1), the weight the triangle's times the instance's.
  const float weight = (float)_vertex.tintCorner.x / 255.0 * _instance.tint.a;
  output.color = float4(saturate(_vertex.color.rgb + weight * (_instance.tint.rgb - _vertex.color.rgb)), _vertex.color.a);
  return output;
}
