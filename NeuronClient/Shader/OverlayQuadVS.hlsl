#include "OverlayQuad.hlsli"

struct OverlayInput // OverlayQuadInstance, one per quad
{
  float4 placement : PLACEMENT;
  float4 color : COLOR;
};

static const float2 CORNERS[6] = {float2(-1.0, -1.0), float2(1.0, -1.0), float2(-1.0, 1.0),
                                  float2(-1.0, 1.0),  float2(1.0, -1.0), float2(1.0, 1.0)};

OverlayVertex main(uint _vertex : SV_VertexID, OverlayInput _quad)
{
  // The corner turned clockwise on the screen, whose y runs down, and the whole mask across the square.
  const float2 corner = CORNERS[_vertex % 6];
  float s;
  float c;
  sincos(_quad.placement.w, s, c);
  const float2 turned = float2(corner.x * c - corner.y * s, corner.x * s + corner.y * c);
  const float2 pixel = _quad.placement.xy + turned * _quad.placement.z;
  OverlayVertex vertex;
  vertex.position = float4(pixel.x * g_pass.values[0].z * 2.0 - 1.0, 1.0 - pixel.y * g_pass.values[0].w * 2.0, 0.0, 1.0);
  vertex.uv = corner * 0.5 + 0.5;
  vertex.color = _quad.color;
  return vertex;
}
