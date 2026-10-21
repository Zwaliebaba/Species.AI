#include "pch.h"

#include "DdsHeader.h"
#include "DdsTexture.h"

#include <algorithm>
#include <format>
#include <utility>

namespace Neuron
{

std::expected<DdsTexture, std::string> DescribeDds(std::span<const std::byte> _file, std::string _debugName)
{
  std::expected<DdsHeader, std::string> header = ReadDdsHeader(_file);
  if (!header)
  {
    return std::unexpected(std::move(header.error()));
  }
  const TextureFormat format = header->Format();
  if (format == TextureFormat::Unknown || header->resourceDimension != DDS_TEXTURE_2D || header->arraySize > 1)
  {
    return std::unexpected(std::format("{} is not a 2-D texture of one slice in a format of ADR-017", _debugName));
  }
  if (header->payloadBytes < ExpectedPayloadBytes(*header))
  {
    return std::unexpected(std::format("{} holds fewer bytes than its {} levels", _debugName, header->mipLevels));
  }
  DdsTexture texture;
  texture.desc.width = header->width;
  texture.desc.height = header->height;
  texture.desc.levels = header->mipLevels;
  // The table's formats are their DXGI_FORMAT values (DdsHeader.h).
  texture.desc.format = static_cast<DXGI_FORMAT>(format);
  texture.desc.debugName = std::move(_debugName);
  std::size_t offset = header->payloadOffset;
  for (std::uint32_t level = 0; level < header->mipLevels; ++level)
  {
    const std::uint32_t width = std::max(header->width >> level, 1u);
    const std::uint32_t height = std::max(header->height >> level, 1u);
    const auto bytes = static_cast<std::size_t>(LevelBytes(format, width, height));
    // One row of texels, or of 4 × 4 blocks for a block-compressed format.
    const auto rowPitch = static_cast<std::uint32_t>(LevelBytes(format, width, 1));
    texture.levels.push_back({_file.subspan(offset, bytes), rowPitch});
    offset += bytes;
  }
  return texture;
}

} // namespace Neuron
