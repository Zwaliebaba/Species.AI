#include "pch.h"

#include "Log.h"
#include "RootSignature.h"
#include "ShaderLayouts.h"

#include <array>

namespace Neuron
{

ComPtr<ID3D12RootSignature> CreateGraphicsRootSignature(ID3D12Device* _device)
{
  std::array<D3D12_ROOT_PARAMETER1, 3> parameters{};
  parameters[ROOT_FRAME_CONSTANTS].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
  parameters[ROOT_FRAME_CONSTANTS].Descriptor = {0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE};
  parameters[ROOT_FRAME_CONSTANTS].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
  parameters[ROOT_PASS_CONSTANTS].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
  parameters[ROOT_PASS_CONSTANTS].Descriptor = {1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE};
  parameters[ROOT_PASS_CONSTANTS].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
  parameters[ROOT_DRAW_CONSTANTS].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
  parameters[ROOT_DRAW_CONSTANTS].Constants = {2, 0, DRAW_CONSTANT_DWORDS};
  parameters[ROOT_DRAW_CONSTANTS].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

  D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc{.Version = D3D_ROOT_SIGNATURE_VERSION_1_1};
  desc.Desc_1_1.NumParameters = static_cast<UINT>(parameters.size());
  desc.Desc_1_1.pParameters = parameters.data();
  desc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                        D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED |
                        D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;

  ComPtr<ID3DBlob> serialized;
  ComPtr<ID3DBlob> errors;
  if (FAILED(D3D12SerializeVersionedRootSignature(&desc, &serialized, &errors)))
  {
    Log(LogCategory::Render, LogLevel::Error, "the graphics root signature does not serialise: {}",
        errors != nullptr ? std::string_view(static_cast<const char*>(errors->GetBufferPointer()), errors->GetBufferSize()) : "");
    return nullptr;
  }
  ComPtr<ID3D12RootSignature> signature;
  if (FAILED(_device->CreateRootSignature(0, serialized->GetBufferPointer(), serialized->GetBufferSize(), IID_PPV_ARGS(&signature))))
  {
    return nullptr;
  }
  signature->SetName(L"Graphics root signature");
  return signature;
}

} // namespace Neuron
