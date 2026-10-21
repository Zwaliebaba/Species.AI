#include "UnlitTextured.hlsli"

// VFX-141's flag pole, `cloth_pole`: the quad's colour alone; no texture is read.
float4 main(QuadVertex _vertex) : SV_Target
{
  return _vertex.color;
}
