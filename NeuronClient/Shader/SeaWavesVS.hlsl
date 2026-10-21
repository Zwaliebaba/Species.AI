#include "SeaWaves.hlsli"

struct SeaInput
{
  float3 position : POSITION; // wu, relative to the sea plane's origin, at this frame's wave height
  float4 color : COLOR;
};

SeaWavesVertex main(SeaInput _input)
{
  const float3 position = mul(g_draw.world, float4(_input.position, 1.0));
  SeaWavesVertex vertex;
  vertex.position = mul(float4(position, 1.0), g_frame.viewProjection);
  vertex.color = _input.color.rgb;
  vertex.depth = dot(position, g_frame.cameraFront.xyz);
  return vertex;
}
