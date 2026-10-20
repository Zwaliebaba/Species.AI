#pragma once

#include "GraphicsDevice.h"

#include <cstdint>

namespace Neuron
{

// ADR-006: the root parameters of the one graphics root signature, which every graphics pipeline state uses.
inline constexpr UINT ROOT_FRAME_CONSTANTS = 0; // root CBV b0: FrameConstants
inline constexpr UINT ROOT_PASS_CONSTANTS = 1;  // root CBV b1: PassConstants
inline constexpr UINT ROOT_DRAW_CONSTANTS = 2;  // root constants b2: DrawConstants

// ADR-006: version 1.1, the input assembler's input layout allowed, and both shader-visible heaps indexed directly.
// Null when the device refuses it.
[[nodiscard]] ComPtr<ID3D12RootSignature> CreateGraphicsRootSignature(ID3D12Device* _device);

} // namespace Neuron
