#include "Sky.hlsli"

struct SkyInstance // SkyLayerInstance, one per quad
{
  float4 rectangle : RECTANGLE;
  float4 color : COLOR;
  float4 uv : TEXCOORD;
  float4 shape : SHAPE;
  uint4 sampling : SAMPLING;
};

// The quad's corners as fractions of its rectangle, counter-clockwise seen from below, so it faces downwards and a
// camera above it culls it (RND-150).
static const float2 CORNERS[6] = {float2(0.0, 0.0), float2(1.0, 0.0), float2(0.0, 1.0), float2(0.0, 1.0), float2(1.0, 0.0), float2(1.0, 1.0)};

SkyVertex main(uint _vertex : SV_VertexID, SkyInstance _instance)
{
  const float2 corner = CORNERS[_vertex % 6];
  const float3 position = float3(lerp(_instance.rectangle.x, _instance.rectangle.z, corner.x), _instance.shape.x,
                                 lerp(_instance.rectangle.y, _instance.rectangle.w, corner.y));
  const float2 along = (_instance.sampling.z & SKY_ALONG_Z) != 0 ? corner.yx : corner;
  SkyVertex vertex;
  vertex.position = mul(float4(position, 1.0), g_frame.viewProjection);
  vertex.uv = _instance.uv.xy + _instance.uv.zw * along;
  vertex.depth = dot(position, g_frame.cameraFront.xyz);
  vertex.color = _instance.color;
  vertex.fog = float2(_instance.shape.y, 1.0 / max(_instance.shape.z - _instance.shape.y, 1.0));
  vertex.fogEnd = _instance.shape.z;
  vertex.sampling = _instance.sampling.xyz;
  return vertex;
}
