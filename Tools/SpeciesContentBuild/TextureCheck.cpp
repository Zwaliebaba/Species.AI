#include "pch.h"

#include "DdsHeader.h"
#include "RowSpec.h"
#include "TextureCheck.h"

#include <array>
#include <format>
#include <string>

namespace ContentBuild
{

namespace
{

// The formats a texture row may name, the longer spellings first, so that R8G8B8A8_UNORM_SRGB is not read as
// R8G8B8A8_UNORM (ADR-017's table).
constexpr std::array<std::string_view, 7> FORMAT_NAMES{"R8G8B8A8_UNORM_SRGB", "BC7_UNORM_SRGB", "R8G8B8A8_UNORM", "R8G8_UNORM",
                                                       "BC4_UNORM",           "BC5_UNORM",      "R8_UNORM"};

/// What a texture row asks for: its format, its level policy, its size and whether the CPU reads it (AST table 3.3-B).
struct RowTexture
{
  Neuron::TextureFormat format = Neuron::TextureFormat::Unknown;
  bool fullChain = false;
  PixelSize size;
  bool cpuTable = false;
};

std::optional<RowTexture> ReadRow(const ManifestRow& _row, std::string_view _file, BuildReport& _report)
{
  RowTexture texture;
  for (const std::string_view name : FORMAT_NAMES)
  {
    if (_row.targetFormat.find(name) != std::string::npos)
    {
      texture.format = Neuron::TextureFormatNamed(name).value_or(Neuron::TextureFormat::Unknown);
      break;
    }
  }
  const bool full = _row.targetFormat.find("full mip chain") != std::string::npos;
  const bool single =
    _row.targetFormat.find("no mips") != std::string::npos || _row.targetFormat.find("one mip level") != std::string::npos;
  const std::optional<PixelSize> display = PixelSizeAfter(_row.spec);
  const bool authorAt2x = _row.spec.find("author at 2x") != std::string::npos;
  const bool srgbRow = _row.colorSpace == "sRGB";
  if (texture.format == Neuron::TextureFormat::Unknown || full == single || !display || (!srgbRow && _row.colorSpace != "linear"))
  {
    _report.Fail(_file, std::format("its row {} (manifest line {}) states no format of ADR-017, no single level policy, no size or no "
                                    "colour space that the content build can read (AST-012)",
                                    _row.assetId, _row.line));
    return std::nullopt;
  }
  if (srgbRow != Neuron::IsSrgb(texture.format))
  {
    _report.Fail(_file, std::format("its row {} states the colour space {} and the format {}, which disagree (AST-012, AST-020)",
                                    _row.assetId, _row.colorSpace, Neuron::TextureFormatName(texture.format)));
    return std::nullopt;
  }
  texture.fullChain = full;
  const std::uint32_t scale = authorAt2x ? UI_AUTHORING_SCALE : 1;
  texture.size = PixelSize{display->width * scale, display->height * scale};
  texture.cpuTable = _row.targetFormat.find("read on the CPU") != std::string::npos;
  return texture;
}

} // namespace

std::optional<Neuron::CookedEntry> CheckTexture(const ManifestRow& _row, std::string_view _file, std::span<const std::byte> _bytes,
                                                BuildReport& _report)
{
  const std::optional<RowTexture> row = ReadRow(_row, _file, _report);
  if (!row)
  {
    return std::nullopt;
  }
  const auto header = Neuron::ReadDdsHeader(_bytes);
  if (!header)
  {
    _report.Fail(_file, std::format("{} (ADR-017)", header.error()));
    return std::nullopt;
  }
  // ADR-017: a DDS in its row's format is its own source, and the content build names each way it differs from its row.
  const std::size_t findings = _report.Findings().size();
  if (!header->dx10)
  {
    _report.Fail(_file, "the DDS has no DX10 header, which every packed texture states its format in (ADR-017)");
    return std::nullopt;
  }
  if (header->Format() != row->format)
  {
    _report.Fail(_file, std::format("the format is {} (DXGI {}); its row {} asks for {} (ADR-017, O61)",
                                    Neuron::TextureFormatName(header->Format()), header->dxgiFormat, _row.assetId,
                                    Neuron::TextureFormatName(row->format)));
  }
  if (header->resourceDimension != Neuron::DDS_TEXTURE_2D || header->arraySize != 1 || header->miscFlag != 0)
  {
    _report.Fail(_file, std::format("the DDS is not one 2-D texture: dimension {}, array size {}, flags {:#x} (ADR-017)",
                                    header->resourceDimension, header->arraySize, header->miscFlag));
  }
  if (header->width != row->size.width || header->height != row->size.height)
  {
    _report.Fail(_file, std::format("the size is {}x{} px; its row {} asks for {}x{} px (AST-068 check 1)", header->width, header->height,
                                    _row.assetId, row->size.width, row->size.height));
  }
  if (header->width > TEXTURE_MAX_DIMENSION || header->height > TEXTURE_MAX_DIMENSION)
  {
    _report.Fail(_file, std::format("the size {}x{} px passes ast.texture.max_dimension, {} px (AST-068 check 2)", header->width,
                                    header->height, TEXTURE_MAX_DIMENSION));
  }
  if (Neuron::IsBlockCompressed(header->Format()) &&
      (header->width % TEXTURE_BLOCK_DIMENSION != 0 || header->height % TEXTURE_BLOCK_DIMENSION != 0))
  {
    _report.Fail(_file, std::format("a block-compressed size of {}x{} px is not a multiple of {} (AST-068 check 3)", header->width,
                                    header->height, TEXTURE_BLOCK_DIMENSION));
  }
  const std::uint32_t levels = row->fullChain ? Neuron::FullMipLevels(header->width, header->height) : 1;
  if (header->mipLevels != levels)
  {
    _report.Fail(_file, std::format("the DDS holds {} levels; its row {} asks for {} ({}) (ADR-017)", header->mipLevels, _row.assetId,
                                    levels, row->fullChain ? "the full mip chain" : "one level"));
  }
  if (_report.Findings().size() == findings && header->payloadBytes != Neuron::ExpectedPayloadBytes(*header))
  {
    _report.Fail(_file, std::format("the levels hold {} bytes where {} are stated by the header (ADR-017)", header->payloadBytes,
                                    Neuron::ExpectedPayloadBytes(*header)));
  }
  if (_report.Findings().size() != findings)
  {
    return std::nullopt;
  }
  Neuron::CookedEntry entry;
  entry.entry.type = row->cpuTable ? Neuron::ContentType::CpuTable : Neuron::ContentType::Texture;
  entry.bytes.assign(_bytes.begin(), _bytes.end());
  return entry;
}

} // namespace ContentBuild
