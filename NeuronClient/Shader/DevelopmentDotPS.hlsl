#include "DevelopmentDot.hlsli"

float4 main(DotVertex _vertex) : SV_Target
{
  return _vertex.color;
}
