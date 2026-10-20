#include "Final.hlsli"

// Vertices 0, 1 and 2 cover the viewport with one triangle and no vertex buffer; uv runs from (0, 0) at the top left.
ScreenVertex main(uint _vertex : SV_VertexID)
{
  const float2 uv = float2((_vertex << 1) & 2, _vertex & 2);
  ScreenVertex vertex;
  vertex.position = float4(uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
  vertex.uv = uv;
  return vertex;
}
