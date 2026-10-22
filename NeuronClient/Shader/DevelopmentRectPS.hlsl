#include "DevelopmentRect.hlsli"

// ADR-120: a filled rectangle of the overlay, the frame-time graph's, in its color; no texture is read.
float4 main(RectVertex _vertex) : SV_Target
{
  return _vertex.color;
}
