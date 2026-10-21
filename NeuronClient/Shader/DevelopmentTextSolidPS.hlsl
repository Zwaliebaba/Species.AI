#include "DevelopmentText.hlsli"

// ADR-120: a filled rectangle of the overlay, the frame-time graph's, in its colour; no page is read.
float4 main(TextVertex _vertex) : SV_Target
{
  return _vertex.color;
}
