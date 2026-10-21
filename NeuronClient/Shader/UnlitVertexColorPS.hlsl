#include "UnlitVertexColor.hlsli"

// RND-333: the colour interpolated across each triangle, its alpha the vertices'; no texture is read.
float4 main(ColorVertex _vertex) : SV_Target
{
  return _vertex.color;
}
