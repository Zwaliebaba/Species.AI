#include "Terrain.hlsli"

// RND-233: the two directional lights on a facet of diffuse colour _diffuse, with a specular term of colour _specular
// and shininess _shininess when _specular is not black. The viewer is at infinity, and there is no ambient term.
float3 Lit(float3 _normal, float3 _diffuse, float3 _specular, float _shininess)
{
  const float3 towardsViewer = -g_frame.cameraFront.xyz;
  float3 sum = float3(0.0, 0.0, 0.0);
  const float3 towards[2] = {g_frame.keyTowards.xyz, g_frame.fillTowards.xyz};
  const float3 colors[2] = {g_frame.keyColor.rgb, g_frame.fillColor.rgb};
  [unroll] for (uint i = 0; i < 2; ++i)
  {
    const float facing = dot(_normal, towards[i]);
    sum += max(facing, 0.0) * colors[i] * _diffuse;
    if (facing > 0.0)
    {
      const float3 halfway = normalize(towards[i] + towardsViewer);
      sum += pow(max(dot(_normal, halfway), 0.0), _shininess) * colors[i] * _specular;
    }
  }
  return saturate(sum);
}

// RND-266 with INVERTED off: the surface term, plus the grid term times the grid texel, each fogged towards the fog
// colour by the pixel's view depth.
float4 main(TerrainVertex _vertex) : SV_Target
{
  Texture2D<float4> gridTexture = ResourceDescriptorHeap[g_draw.indices.x];
  SamplerState linearWrap = SamplerDescriptorHeap[SAMPLER_LINEAR_WRAP];
  const float grid = gridTexture.Sample(linearWrap, _vertex.grid).r; // a BC4 mask: its intensity is .r (ADR-115)
  const float3 surface = Lit(_vertex.normal, _vertex.color.rgb, float3(0.0, 0.0, 0.0), 1.0);
  const float3 overlay = Lit(_vertex.normal, _vertex.color.rgb, g_pass.values[0].rgb, g_pass.values[0].a);
  const float fog = saturate((g_frame.fogRange.y - _vertex.depth) * g_frame.fogRange.z);
  const float3 s = lerp(g_frame.fogColor.rgb, surface, fog);
  const float3 q = lerp(g_frame.fogColor.rgb, overlay * grid, fog);
  return float4(saturate(s + q * _vertex.color.a), _vertex.color.a);
}
