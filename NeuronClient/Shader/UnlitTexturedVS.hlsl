#include "UnlitTextured.hlsli"

struct Instance // QuadInstance, one per quad
{
  float4 position : POSITION;
  float4 endPoint : ENDPOINT;
  float4 axisRight : AXISRIGHT;
  float4 axisUp : AXISUP;
  float4 color : COLOR0;
  float4 endColor : COLOR1;
  float4 uv : TEXCOORD;
  uint4 shape : SHAPE;
};

// RND-305: two triangles, 0, 1, 2 and 0, 2, 3, over the corners bottom-left, bottom-right, top-right and top-left; every
// quad is drawn with culling off.
static const uint CORNER_OF_VERTEX[6] = {0, 1, 2, 0, 2, 3};
static const float2 CORNERS[4] = {float2(-1.0, -1.0), float2(1.0, -1.0), float2(1.0, 1.0), float2(-1.0, 1.0)};

QuadVertex main(uint _vertex : SV_VertexID, Instance _quad)
{
  const uint corner = CORNER_OF_VERTEX[_vertex % 6];
  const float2 at = CORNERS[corner];
  // Row vectors: the view matrix's first two columns are the camera's right and up axes in world space.
  const float3 cameraRight = float3(g_frame.view[0][0], g_frame.view[1][0], g_frame.view[2][0]);
  const float3 cameraUp = float3(g_frame.view[0][1], g_frame.view[1][1], g_frame.view[2][1]);
  const float3 p = _quad.position.xyz;
  const float halfWidth = _quad.position.w;
  const float halfHeight = _quad.endPoint.w;
  float3 world;
  float4 color = _quad.color;
  float2 uvAt = float2((at.x + 1.0) * 0.5, (1.0 - at.y) * 0.5); // the texture's top is the quad's top
  if (_quad.shape.x == QUAD_SEGMENT)
  {
    // RND-306: a ribbon from the start to the end, its width across the camera ray; u along it and v across.
    const float3 e = _quad.endPoint.xyz;
    const float3 mid = (p + e) * 0.5; // the camera is at the origin
    const float3 side = normalize(cross(mid, mid - e)) * halfWidth;
    const float3 segment[4] = {p - side, p + side, e + side, e - side};
    world = segment[corner];
    color = corner >= 2 ? _quad.endColor : _quad.color;
    uvAt = float2(corner >= 2 ? 1.0 : 0.0, (corner == 1 || corner == 2) ? 1.0 : 0.0);
  }
  else
  {
    float3 right;
    float3 up;
    if (_quad.shape.x == QUAD_CAMERA_FACING)
    {
      float s;
      float c;
      sincos(asfloat(_quad.shape.z), s, c);
      right = cameraRight * c + cameraUp * s;
      up = cameraUp * c - cameraRight * s;
    }
    else if (_quad.shape.x == QUAD_UPRIGHT_CARD)
    {
      up = float3(0.0, 1.0, 0.0);
      right = normalize(cross(up, -p));
    }
    else
    {
      right = _quad.axisRight.xyz;
      up = _quad.axisUp.xyz;
      const float d = length(p);
      if (_quad.axisUp.w > 0.0 && d > _quad.axisRight.w)
      {
        // RND-306's billboard blend, linear and not renormalised.
        const float f = min((d - _quad.axisRight.w) / _quad.axisUp.w, 1.0);
        const float3 towards = (_quad.shape.w & QUAD_BLEND_LEFT) != 0 ? -cameraRight : cameraRight;
        right = lerp(right, towards, f);
        up = lerp(up, cameraUp, f);
      }
    }
    const float upAt = _quad.shape.y == QUAD_ANCHOR_BOTTOM ? at.y + 1.0 : at.y;
    world = p + right * (at.x * halfWidth) + up * (upAt * halfHeight);
  }
  QuadVertex vertex;
  vertex.position = mul(float4(world, 1.0), g_frame.viewProjection);
  vertex.uv = lerp(_quad.uv.xy, _quad.uv.zw, uvAt);
  vertex.color = color;
  vertex.depth = dot(world, g_frame.cameraFront.xyz);
  return vertex;
}
