#include "Ui.hlsli"

// UI-184: a solid quad in its colour; no texture is read.
float4 main(UiVertex _vertex) : SV_Target
{
  return _vertex.color;
}
