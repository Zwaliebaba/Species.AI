// The UI's quads (UI-184, ADR-145), drawn in pass 19 and with the cursor in pass 20: rectangles in pixels of the back
// buffer, each reading its batch's texture as its source states.
#include "../ShaderLayouts.h"

ConstantBuffer<PassConstants> g_pass : register(b1); // values[0]: the target's width and height, and their reciprocals
ConstantBuffer<DrawConstants> g_draw : register(b2); // indices.x: the batch's texture

struct UiVertex
{
  float4 position : SV_Position;
  float2 uv : TEXCOORD0;
  nointerpolation float4 color : COLOR0;
};
