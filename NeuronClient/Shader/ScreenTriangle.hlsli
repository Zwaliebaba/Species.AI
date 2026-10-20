// The vertex of a triangle that covers the screen (RND-063's full-screen passes), made from the vertex index alone.
struct ScreenVertex
{
  float4 position : SV_Position;
  float2 uv : TEXCOORD0;
};
