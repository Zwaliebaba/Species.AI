#include "MeshLit.hlsli"

// RND-163, RND-168 (ADR-140): the structures' meshes of pass 6 and the port pads of pass 8, lit as the actors are and
// fogged as the terrain is.
float4 main(MeshLitVertex _vertex) : SV_Target
{
  return float4(LevelFog(ObjectLit(_vertex.normal, _vertex.color.rgb), _vertex.depth), 1.0);
}
