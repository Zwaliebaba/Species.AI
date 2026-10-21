#pragma once

#include "GpuResourceFactory.h"

#include <cstddef>
#include <expected>
#include <span>
#include <string>
#include <vector>

namespace Neuron
{

/// A DDS file's 2-D texture as the GPU resource factory takes it (ADR-017, ADR-078): its description and one entry per
/// level, each pointing into the file's bytes, which outlive it.
struct DdsTexture
{
  GpuTextureDesc desc;
  std::vector<GpuSubresourceData> levels;
};

// The texture of _file, or why the bytes are not one the factory can take: not a DDS file, not a 2-D texture of one
// slice in a format of ADR-017's table, or a payload shorter than its levels.
[[nodiscard]] std::expected<DdsTexture, std::string> DescribeDds(std::span<const std::byte> _file, std::string _debugName);

} // namespace Neuron
