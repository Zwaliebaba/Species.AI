// The lit meshes of the actor passes (RND-263, RND-324; ADR-122): a cooked mesh's facets, instanced, under the team tint
// of RND-271 and the object lighting of RND-228, with no fog (RND-323); the structures' permutation adds the level fog
// (RND-163, ADR-140).
#include "../ShaderLayouts.h"

ConstantBuffer<FrameConstants> g_frame : register(b0);

struct MeshLitVertex
{
  float4 position : SV_Position;
  nointerpolation float4 color : COLOR0;
  nointerpolation float3 normal : NORMAL0;
  float depth : TEXCOORD0; // view depth, the level fog's coordinate (RND-222), which the structures' passes read
};

// RND-222: the level fog of the structures' passes, as the terrain's (RND-163, ADR-140).
float3 LevelFog(float3 _color, float _depth)
{
  const float fog = saturate((g_frame.fogRange.y - _depth) * g_frame.fogRange.z);
  return lerp(g_frame.fogColor.rgb, _color, fog);
}

// RND-228: diffuse from both lights, with no specular term and no ambient one.
float3 ObjectLit(float3 _normal, float3 _diffuse)
{
  const float3 key = max(dot(_normal, g_frame.keyTowards.xyz), 0.0) * g_frame.keyColor.rgb;
  const float3 fill = max(dot(_normal, g_frame.fillTowards.xyz), 0.0) * g_frame.fillColor.rgb;
  return saturate((key + fill) * _diffuse);
}
