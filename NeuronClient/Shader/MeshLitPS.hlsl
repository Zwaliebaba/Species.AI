#include "MeshLit.hlsli"

float4 main(MeshLitVertex _vertex) : SV_Target
{
  return float4(ObjectLit(_vertex.normal, _vertex.color.rgb), 1.0);
}
