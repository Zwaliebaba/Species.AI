#include "pch.h"

#include "AdapterChoice.h"

#include <format>

namespace Neuron
{

std::string ShaderModelName(std::uint32_t _model)
{
  return std::format("{}.{}", _model >> 4, _model & 0xF);
}

std::string MissingCapability(const AdapterCapabilities& _adapter)
{
  if (_adapter.software)
  {
    return "a hardware adapter";
  }
  if (!_adapter.featureLevelMet)
  {
    return "Direct3D feature level 12_1";
  }
  if (_adapter.shaderModel < MIN_SHADER_MODEL)
  {
    return std::format("Shader Model {}", ShaderModelName(MIN_SHADER_MODEL));
  }
  if (_adapter.resourceBindingTier < REQUIRED_RESOURCE_BINDING_TIER)
  {
    return std::format("resource binding tier {}", REQUIRED_RESOURCE_BINDING_TIER);
  }
  if (!_adapter.waveOps)
  {
    return "wave operations";
  }
  return {};
}

AdapterChoice ChooseAdapter(std::span<const AdapterCapabilities> _adapters)
{
  AdapterChoice choice;
  for (std::size_t index = 0; index < _adapters.size(); ++index)
  {
    const std::string missing = MissingCapability(_adapters[index]);
    if (missing.empty())
    {
      choice.adapter = index;
      return choice;
    }
    if (choice.rejectedAdapter.empty() && !_adapters[index].software)
    {
      choice.rejectedAdapter = _adapters[index].description;
      choice.missingCapability = missing;
    }
  }
  if (choice.rejectedAdapter.empty())
  {
    choice.missingCapability = "a hardware adapter";
  }
  return choice;
}

std::uint32_t HighestShaderModel(std::uint32_t _highestKnown, const std::function<std::optional<std::uint32_t>(std::uint32_t)>& _query)
{
  for (std::uint32_t model = _highestKnown; model >= LOWEST_SHADER_MODEL; --model)
  {
    if (const std::optional<std::uint32_t> supported = _query(model))
    {
      return *supported;
    }
  }
  return 0;
}

} // namespace Neuron
