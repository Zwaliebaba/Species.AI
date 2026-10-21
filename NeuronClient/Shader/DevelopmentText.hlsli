// The development overlay's text (DBG-095, pass 29): EditorFont's glyphs at a whole-number scale, read with point
// sampling so that every drawn pixel is a square of the sheet (O63).
#include "../ShaderLayouts.h"

ConstantBuffer<PassConstants> g_pass : register(b1); // values[0]: the target's width and height, and their reciprocals
ConstantBuffer<DrawConstants> g_draw : register(b2); // indices.x: the face's page

struct TextVertex
{
  float4 position : SV_Position;
  float2 uv : TEXCOORD0;
  nointerpolation float4 color : COLOR0;
};
