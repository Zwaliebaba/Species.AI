#include "UnlitVertexColor.hlsli"

struct Vertex // TriangleVertex; its texture coordinate is not read
{
  float4 position : POSITION;
  float4 uv : TEXCOORD;
  float4 color : COLOR;
};

ColorVertex main(Vertex _vertex)
{
  ColorVertex output;
  // A clip-space corner lies on the far plane, depth 0 of the reversed range, which no entry of this shader tests.
  output.position = (g_draw.indices.z & TRIANGLE_SCREEN_SPACE) != 0 ? float4(_vertex.position.xy, 0.0, 1.0)
                                                                    : mul(float4(_vertex.position.xyz, 1.0), g_frame.viewProjection);
  output.color = _vertex.color;
  return output;
}
