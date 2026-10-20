#include "DevelopmentDot.hlsli"

struct DotInput // DotInstance, one per dot
{
  float4 center : CENTER;
  float4 color : COLOR;
};

static const float2 CORNERS[6] = {float2(-1.0, -1.0), float2(1.0, -1.0), float2(-1.0, 1.0), float2(-1.0, 1.0), float2(1.0, -1.0), float2(1.0, 1.0)};

// The dot's centre is projected, then its corners are moved by half its side in pixels.
DotVertex main(uint _vertex : SV_VertexID, DotInput _dot)
{
  float4 position = mul(float4(_dot.center.xyz, 1.0), g_frame.viewProjection);
  const float2 halfSide = 0.5 * _dot.center.w * 2.0 * g_frame.viewport.zw; // half the side, in normalised coordinates
  position.xy += CORNERS[_vertex % 6] * halfSide * position.w;
  DotVertex vertex;
  vertex.position = position;
  vertex.color = _dot.color;
  return vertex;
}
