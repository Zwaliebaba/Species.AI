#include "Terrain.hlsli"

struct TerrainInput
{
  float3 position : POSITION; // wu, relative to the chunk's origin
  float4 color : COLOR;
  float3 normal : NORMAL;
  float2 grid : TEXCOORD;
};

TerrainVertex main(TerrainInput _input)
{
  const float3 position = mul(g_draw.world, float4(_input.position, 1.0));
  TerrainVertex vertex;
  vertex.position = mul(float4(position, 1.0), g_frame.viewProjection);
  vertex.color = _input.color;
  vertex.normal = _input.normal;
  vertex.grid = _input.grid;
  vertex.depth = dot(position, g_frame.cameraFront.xyz);
  return vertex;
}
