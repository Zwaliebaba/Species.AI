#include "pch.h"

#include "DdsHeader.h"

#include <algorithm>
#include <bit>
#include <format>

namespace Neuron
{

namespace
{

constexpr std::uint32_t DDS_MAGIC = 0x20534444;           // "DDS "
constexpr std::uint32_t DDS_HEADER_SIZE = 124;            // DDS_HEADER::dwSize
constexpr std::uint32_t DDS_PIXEL_FORMAT_SIZE = 32;       // DDS_PIXELFORMAT::dwSize
constexpr std::uint32_t DDS_FOURCC = 0x4;                 // DDPF_FOURCC
constexpr std::uint32_t DDS_FOURCC_DX10 = 0x30315844;     // "DX10"
constexpr std::uint32_t DDS_FLAG_MIP_MAP_COUNT = 0x20000; // DDSD_MIPMAPCOUNT
constexpr std::uint32_t DDS_MAX_LEVELS = 32;

struct FormatInfo
{
  TextureFormat format;
  std::string_view name;
  std::uint32_t bytesPerBlockOrTexel; // a 4 × 4 block for a block-compressed format, a texel otherwise
  bool blockCompressed;
  bool srgb;
};

constexpr std::array<FormatInfo, 7> FORMATS{{
  {TextureFormat::R8G8B8A8Unorm, "R8G8B8A8_UNORM", 4, false, false},
  {TextureFormat::R8G8B8A8UnormSrgb, "R8G8B8A8_UNORM_SRGB", 4, false, true},
  {TextureFormat::R8G8Unorm, "R8G8_UNORM", 2, false, false},
  {TextureFormat::R8Unorm, "R8_UNORM", 1, false, false},
  {TextureFormat::Bc4Unorm, "BC4_UNORM", 8, true, false},
  {TextureFormat::Bc5Unorm, "BC5_UNORM", 16, true, false},
  {TextureFormat::Bc7UnormSrgb, "BC7_UNORM_SRGB", 16, true, true},
}};

const FormatInfo* InfoOf(TextureFormat _format) noexcept
{
  const auto found = std::ranges::find(FORMATS, _format, &FormatInfo::format);
  return found == FORMATS.end() ? nullptr : &*found;
}

std::uint32_t Word(std::span<const std::byte> _file, std::size_t _offset) noexcept
{
  return static_cast<std::uint32_t>(_file[_offset]) | (static_cast<std::uint32_t>(_file[_offset + 1]) << 8) |
         (static_cast<std::uint32_t>(_file[_offset + 2]) << 16) | (static_cast<std::uint32_t>(_file[_offset + 3]) << 24);
}

} // namespace

std::string_view TextureFormatName(TextureFormat _format) noexcept
{
  const FormatInfo* info = InfoOf(_format);
  return info == nullptr ? std::string_view{"UNKNOWN"} : info->name;
}

std::optional<TextureFormat> TextureFormatNamed(std::string_view _name) noexcept
{
  const auto found = std::ranges::find(FORMATS, _name, &FormatInfo::name);
  return found == FORMATS.end() ? std::nullopt : std::optional<TextureFormat>(found->format);
}

bool IsBlockCompressed(TextureFormat _format) noexcept
{
  const FormatInfo* info = InfoOf(_format);
  return info != nullptr && info->blockCompressed;
}

bool IsSrgb(TextureFormat _format) noexcept
{
  const FormatInfo* info = InfoOf(_format);
  return info != nullptr && info->srgb;
}

std::uint64_t LevelBytes(TextureFormat _format, std::uint32_t _width, std::uint32_t _height) noexcept
{
  const FormatInfo* info = InfoOf(_format);
  if (info == nullptr)
  {
    return 0;
  }
  if (info->blockCompressed)
  {
    const std::uint64_t blocksWide = std::max<std::uint64_t>(1, (std::uint64_t{_width} + 3) / 4);
    const std::uint64_t blocksHigh = std::max<std::uint64_t>(1, (std::uint64_t{_height} + 3) / 4);
    return blocksWide * blocksHigh * info->bytesPerBlockOrTexel;
  }
  return std::uint64_t{_width} * _height * info->bytesPerBlockOrTexel;
}

std::uint32_t FullMipLevels(std::uint32_t _width, std::uint32_t _height) noexcept
{
  const std::uint32_t largest = std::max(_width, _height);
  return largest == 0 ? 0 : static_cast<std::uint32_t>(std::bit_width(largest));
}

TextureFormat DdsHeader::Format() const noexcept
{
  const auto format = static_cast<TextureFormat>(dxgiFormat);
  return dx10 && InfoOf(format) != nullptr ? format : TextureFormat::Unknown;
}

std::expected<DdsHeader, std::string> ReadDdsHeader(std::span<const std::byte> _file)
{
  if (_file.size() < DDS_HEADER_BYTES)
  {
    return std::unexpected(std::format("{} bytes, fewer than a DDS header's {}", _file.size(), DDS_HEADER_BYTES));
  }
  if (Word(_file, 0) != DDS_MAGIC)
  {
    return std::unexpected(std::string("byte 0: not a DDS file (its magic is not \"DDS \")"));
  }
  if (Word(_file, 4) != DDS_HEADER_SIZE || Word(_file, 76) != DDS_PIXEL_FORMAT_SIZE)
  {
    return std::unexpected(std::string("byte 4: the DDS header's sizes are not 124 and 32"));
  }
  DdsHeader header;
  const std::uint32_t flags = Word(_file, 8);
  header.height = Word(_file, 12);
  header.width = Word(_file, 16);
  const std::uint32_t levels = Word(_file, 28);
  header.mipLevels = (flags & DDS_FLAG_MIP_MAP_COUNT) != 0 && levels != 0 ? levels : 1;
  header.pixelFlags = Word(_file, 80);
  const std::uint32_t fourCc = Word(_file, 84);
  header.rgbBitCount = Word(_file, 88);
  header.masks = {Word(_file, 92), Word(_file, 96), Word(_file, 100), Word(_file, 104)};
  header.payloadOffset = DDS_HEADER_BYTES;
  if ((header.pixelFlags & DDS_FOURCC) != 0 && fourCc == DDS_FOURCC_DX10)
  {
    if (_file.size() < DDS_HEADER_BYTES + DDS_DX10_HEADER_BYTES)
    {
      return std::unexpected(std::string("byte 128: the file ends inside its DX10 header"));
    }
    header.dx10 = true;
    header.dxgiFormat = Word(_file, 128);
    header.resourceDimension = Word(_file, 132);
    header.miscFlag = Word(_file, 136);
    header.arraySize = Word(_file, 140);
    header.payloadOffset = DDS_HEADER_BYTES + DDS_DX10_HEADER_BYTES;
  }
  if (header.width == 0 || header.height == 0 || header.mipLevels > DDS_MAX_LEVELS)
  {
    return std::unexpected(std::format("byte 12: a size of {} x {} px with {} levels", header.width, header.height, header.mipLevels));
  }
  header.payloadBytes = _file.size() - header.payloadOffset;
  return header;
}

std::uint64_t ExpectedPayloadBytes(const DdsHeader& _header) noexcept
{
  std::uint64_t bytes = 0;
  std::uint32_t width = _header.width;
  std::uint32_t height = _header.height;
  for (std::uint32_t level = 0; level < _header.mipLevels; ++level)
  {
    bytes += LevelBytes(_header.Format(), width, height);
    width = std::max<std::uint32_t>(1, width / 2);
    height = std::max<std::uint32_t>(1, height / 2);
  }
  return bytes;
}

} // namespace Neuron
