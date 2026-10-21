#include "OverlayQuad.hlsli"

// `overlay_alpha` (RND-357): the colour over the target with the quad's alpha times the mask's coverage, as the black
// shadows of the markers and the badge are drawn.
float4 main(OverlayVertex _vertex) : SV_Target
{
  return float4(_vertex.color.rgb, Coverage(_vertex.uv) * _vertex.color.a);
}
