#include "pch.h"

#include "GpuResourceFactory.h"

#include <algorithm>
#include <bit>

namespace Neuron
{

bool IsValidDescription(const GpuTextureDesc& _desc) noexcept
{
  if (_desc.width == 0 || _desc.height == 0 || _desc.levels == 0 || _desc.format == DXGI_FORMAT_UNKNOWN)
  {
    return false;
  }
  const std::uint32_t fullChain = static_cast<std::uint32_t>(std::bit_width(std::max(_desc.width, _desc.height)));
  return _desc.levels <= fullChain;
}

bool IsValidDescription(const GpuBufferDesc& _desc) noexcept
{
  return _desc.sizeBytes != 0 && (_desc.strideBytes == 0 || _desc.sizeBytes % _desc.strideBytes == 0);
}

} // namespace Neuron
