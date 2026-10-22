// The development overlay's filled rectangles (ADR-120, pass 29), under its text, which the UI's glyph pipelines draw
// (ADR-147).
#include "../ShaderLayouts.h"

ConstantBuffer<PassConstants> g_pass : register(b1); // values[0]: the target's width and height, and their reciprocals

struct RectVertex
{
  float4 position : SV_Position;
  nointerpolation float4 color : COLOR0;
};
