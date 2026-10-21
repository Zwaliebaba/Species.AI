#include "OverlayQuad.hlsli"

// `overlay_additive` (RND-357): the colour weighted by the mask's coverage and the quad's alpha, added to the target.
float4 main(OverlayVertex _vertex) : SV_Target
{
  return float4(_vertex.color.rgb * (Coverage(_vertex.uv) * _vertex.color.a), 1.0);
}
