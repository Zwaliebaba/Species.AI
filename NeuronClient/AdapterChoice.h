#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>

namespace Neuron
{

// Shader models as D3D_SHADER_MODEL spells them: the major version in the high nibble, the minor in the low one.
inline constexpr std::uint32_t MIN_SHADER_MODEL = 0x66; // ADR-002
inline constexpr std::uint32_t LOWEST_SHADER_MODEL = 0x60;
inline constexpr std::uint32_t REQUIRED_RESOURCE_BINDING_TIER = 3; // ADR-002

// What the capability probe found on one adapter, in the order DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE lists them.
struct AdapterCapabilities
{
  std::string description;
  bool software = false;         // WARP or the basic render driver: never chosen (ADR-092)
  bool featureLevelMet = false;  // a device was created at D3D_FEATURE_LEVEL_12_1
  std::uint32_t shaderModel = 0; // the highest supported, 0 when the query failed
  std::uint32_t resourceBindingTier = 0;
  bool waveOps = false;
};

// The adapter that RND-020 chooses, or the first rejected hardware adapter and what it lacks.
struct AdapterChoice
{
  std::optional<std::size_t> adapter;
  std::string rejectedAdapter;
  std::string missingCapability;
};

// The first capability of ADR-002 that the adapter lacks, or an empty string when it has them all.
[[nodiscard]] std::string MissingCapability(const AdapterCapabilities& _adapter);

// RND-020: the first adapter that passes every check; when none does, the message names the first hardware adapter and
// the capability it lacks (ui.error.gpu_unsupported).
[[nodiscard]] AdapterChoice ChooseAdapter(std::span<const AdapterCapabilities> _adapters);

// ADR-002: the highest shader model, by asking for each model from the highest the runtime's header knows down to 6.0,
// because the query rejects a model the runtime does not know. _query answers one request with the supported model, or
// nothing when it rejects it. Returns 0 when every request is rejected.
[[nodiscard]] std::uint32_t HighestShaderModel(std::uint32_t _highestKnown,
                                               const std::function<std::optional<std::uint32_t>(std::uint32_t)>& _query);

// A shader model for a message: 0x66 is "6.6".
[[nodiscard]] std::string ShaderModelName(std::uint32_t _model);

} // namespace Neuron
