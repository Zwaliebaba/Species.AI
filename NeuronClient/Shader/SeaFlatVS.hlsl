#include "SeaFlat.hlsli"

struct SeaInput
{
  float3 position : POSITION; // wu, relative to the sea plane's origin
  float4 color : COLOR;
};

SeaFlatVertex main(SeaInput _input)
{
  const float3 position = mul(g_draw.world, float4(_input.position, 1.0));
  const float2 across = _input.position.xz * g_pass.values[0].z; // 0 to 1 across the plane
  SeaFlatVertex vertex;
  vertex.position = mul(float4(position, 1.0), g_frame.viewProjection);
  // RND-142: the tile repeats across the plane and scrolls on both axes; the mask's v is 0 at the plane's far edge.
  vertex.tile = across * g_pass.values[0].x + g_frame.time.x * g_pass.values[0].y;
  vertex.mask = float2(across.x, 1.0 - across.y);
  vertex.depth = dot(position, g_frame.cameraFront.xyz);
  vertex.ground = position.xz;
  return vertex;
}
