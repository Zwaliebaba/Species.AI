#include "pch.h"

#include "AuthoringParser.h"
#include "DdsHeader.h"
#include "FontAsset.h"
#include "FontPacker.h"
#include "RowSpec.h"
#include "TextGlyphTable.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <format>
#include <limits>
#include <set>
#include <string>
#include <vector>

namespace ContentBuild
{

namespace
{

// The sheet the owner draws (O63): a DDS without a DX10 header, 32 bits a texel, white, with the coverage in alpha.
constexpr std::uint32_t SHEET_BITS = 32;
constexpr std::array<std::uint32_t, 4> SHEET_MASKS{0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000};
constexpr std::size_t SHEET_ALPHA_BYTE = 3; // B8G8R8A8 in memory

/// The sheet's coverage, one byte a texel, rows from the top.
struct Sheet
{
  std::uint32_t width = 0;
  std::uint32_t height = 0;
  std::vector<std::uint8_t> coverage;
};

std::optional<Sheet> ReadSheet(const FaceSources& _face, BuildReport& _report)
{
  const auto header = Neuron::ReadDdsHeader(_face.sheet);
  if (!header)
  {
    _report.Fail(_face.sheetFile, std::format("{} (O63)", header.error()));
    return std::nullopt;
  }
  const bool layout = !header->dx10 && (header->pixelFlags & Neuron::DDS_PIXEL_RGB) != 0 &&
                      (header->pixelFlags & Neuron::DDS_PIXEL_ALPHA) != 0 && header->rgbBitCount == SHEET_BITS &&
                      header->masks == SHEET_MASKS && header->mipLevels == 1;
  if (!layout)
  {
    _report.Fail(_face.sheetFile, "the sheet is not a DDS of 32-bit A8R8G8B8 texels with one level and no DX10 header, as its row "
                                  "states (O63, AST-068 check 5)");
    return std::nullopt;
  }
  const std::optional<PixelSize> size = PixelSizeAfter(_face.sheetRow->spec);
  if (!size || size->width != header->width || size->height != header->height)
  {
    _report.Fail(_face.sheetFile,
                 std::format("the sheet is {}x{} px; its row {} asks for {} (AST-068 check 1)", header->width, header->height,
                             _face.sheetRow->assetId, size ? std::format("{}x{} px", size->width, size->height) : std::string("no size")));
    return std::nullopt;
  }
  const std::uint64_t texels = std::uint64_t{header->width} * header->height;
  if (header->payloadBytes != texels * 4)
  {
    _report.Fail(_face.sheetFile, std::format("the sheet holds {} bytes of texels where {} x {} x 4 are stated (ADR-017)",
                                              header->payloadBytes, header->width, header->height));
    return std::nullopt;
  }
  Sheet sheet;
  sheet.width = header->width;
  sheet.height = header->height;
  sheet.coverage.resize(static_cast<std::size_t>(texels));
  for (std::size_t t = 0; t < sheet.coverage.size(); ++t)
  {
    const std::span<const std::byte> texel = _face.sheet.subspan(header->payloadOffset + t * 4, 4);
    const auto alpha = static_cast<std::uint8_t>(texel[SHEET_ALPHA_BYTE]);
    const bool white = texel[0] == std::byte{0xFF} && texel[1] == std::byte{0xFF} && texel[2] == std::byte{0xFF};
    if ((alpha != 0 && alpha != 0xFF) || (alpha != 0 && !white))
    {
      _report.Fail(_face.sheetFile, std::format("texel ({}, {}) is not white with a coverage of 0 or 255, as its row states (O63)",
                                                t % sheet.width, t / sheet.width));
      return std::nullopt;
    }
    sheet.coverage[t] = alpha;
  }
  return sheet;
}

std::optional<TextGlyphTable> ReadTable(const FaceSources& _face, BuildReport& _report)
{
  auto document = Neuron::AuthoringParser{}.Parse(_face.table, _face.tableFile);
  if (!document)
  {
    _report.Fail(document.error());
    return std::nullopt;
  }
  auto table = ReadTextGlyphTable(*document);
  if (!table)
  {
    for (const Neuron::ContentDiagnostic& diagnostic : table.error())
    {
      _report.Fail(diagnostic);
    }
    return std::nullopt;
  }
  return std::move(*table);
}

// DAT-219 and the rows' specs: the rules a schema cannot state.
bool CheckTable(const FaceSources& _face, const TextGlyphTable& _table, const Sheet& _sheet, BuildReport& _report)
{
  const std::size_t findings = _report.Findings().size();
  const auto fail = [&](std::string_view _message) { _report.Fail(_face.tableFile, _message); };
  if (_table.face != _face.tableRow->assetId)
  {
    fail(std::format("face names {}, and the table is a member of {} (AST-089)", _table.face, _face.tableRow->assetId));
  }
  constexpr std::int64_t SIZE_LIMIT = std::numeric_limits<std::uint16_t>::max();
  if (_table.designSizePx <= 0 || _table.lineHeightPx <= 0 || _table.cellWidthPx <= 0 || _table.cellHeightPx <= 0 ||
      _table.baselinePx > _table.cellHeightPx || _table.cellWidthPx > SIZE_LIMIT || _table.cellHeightPx > SIZE_LIMIT ||
      _table.designSizePx > SIZE_LIMIT || _table.lineHeightPx > SIZE_LIMIT)
  {
    fail("the design size, line height and cell size are 1 to 65535 px and the baseline lies in the cell (DAT-219)");
    return false;
  }
  const std::optional<PixelSize> cell = PixelSizeAfter(_face.sheetRow->spec, "cells of ");
  if (!cell || cell->width != _table.cellWidthPx || cell->height != _table.cellHeightPx)
  {
    fail(std::format("the cell is {}x{} px; the row {} states another (AST-012)", _table.cellWidthPx, _table.cellHeightPx,
                     _face.sheetRow->assetId));
  }
  if (NumberAfter(_face.sheetRow->spec, "design size ") != static_cast<std::uint64_t>(_table.designSizePx) ||
      NumberAfter(_face.sheetRow->spec, "baseline ") != static_cast<std::uint64_t>(_table.baselinePx))
  {
    fail(std::format("the design size {} px and baseline {} px are not those of the row {} (AST-012)", _table.designSizePx,
                     _table.baselinePx, _face.sheetRow->assetId));
  }
  std::set<std::int64_t> codePoints;
  for (const TextGlyphTable::GlyphsItem& glyph : _table.glyphs)
  {
    const std::string name = std::format("U+{:04X}", glyph.cp);
    if (!codePoints.insert(glyph.cp).second)
    {
      fail(std::format("the code point {} is listed twice (DAT-219)", name));
    }
    if (glyph.col >= SIZE_LIMIT || glyph.row >= SIZE_LIMIT || (glyph.col + 1) * _table.cellWidthPx > _sheet.width ||
        (glyph.row + 1) * _table.cellHeightPx > _sheet.height)
    {
      fail(std::format("the cell ({}, {}) of {} lies outside the {}x{} px sheet (DAT-219)", glyph.col, glyph.row, name, _sheet.width,
                       _sheet.height));
    }
    if (glyph.widthPx > _table.cellWidthPx || glyph.advancePx > SIZE_LIMIT)
    {
      fail(std::format("the glyph {} is wider than its cell or advances more than 65535 px (DAT-219)", name));
    }
  }
  if (!codePoints.contains(_table.missing))
  {
    fail(std::format("the missing glyph U+{:04X} is not a glyph of the table (DAT-219)", _table.missing));
  }
  return _report.Findings().size() == findings;
}

} // namespace

std::optional<Neuron::CookedEntry> PackFont(const FaceSources& _face, BuildReport& _report)
{
  const std::optional<Sheet> sheet = ReadSheet(_face, _report);
  const std::optional<TextGlyphTable> table = ReadTable(_face, _report);
  if (!sheet || !table || !CheckTable(_face, *table, *sheet, _report))
  {
    return std::nullopt;
  }
  Neuron::FontAsset font;
  font.designSizePixels = static_cast<std::uint16_t>(table->designSizePx);
  font.lineHeightPixels = static_cast<std::uint16_t>(table->lineHeightPx);
  font.baselinePixels = static_cast<std::uint16_t>(table->baselinePx);
  font.cellWidthPixels = static_cast<std::uint16_t>(table->cellWidthPx);
  font.cellHeightPixels = static_cast<std::uint16_t>(table->cellHeightPx);
  font.missingCodePoint = static_cast<std::uint32_t>(table->missing);
  font.pageWidth = static_cast<std::uint16_t>(sheet->width);
  font.pageHeight = static_cast<std::uint16_t>(sheet->height);
  font.glyphs.reserve(table->glyphs.size());
  for (const TextGlyphTable::GlyphsItem& glyph : table->glyphs)
  {
    Neuron::FontGlyph cooked;
    cooked.codePoint = static_cast<std::uint32_t>(glyph.cp);
    cooked.x = static_cast<std::uint16_t>(glyph.col * table->cellWidthPx);
    cooked.y = static_cast<std::uint16_t>(glyph.row * table->cellHeightPx);
    cooked.widthPixels = static_cast<std::uint16_t>(glyph.widthPx);
    cooked.bearingPixels = static_cast<std::int16_t>(glyph.bearingPx);
    cooked.advancePixels = static_cast<std::uint16_t>(glyph.advancePx);
    cooked.offsetYPixels = static_cast<std::int16_t>(glyph.offsetYPx);
    font.glyphs.push_back(cooked);
  }
  // DAT-219: the cooked table is sorted by code point, so the order of the authored list changes no cooked byte.
  std::ranges::sort(font.glyphs, {}, &Neuron::FontGlyph::codePoint);
  font.page = sheet->coverage;
  Neuron::CookedEntry entry;
  entry.entry.type = Neuron::ContentType::Font;
  entry.bytes = Neuron::WriteFontAsset(font);
  // AST-091: the entry reads back through the runtime's loader as what the packer wrote.
  const auto readBack = Neuron::ReadFontAsset(entry.bytes);
  if (!readBack || Neuron::WriteFontAsset(*readBack) != entry.bytes)
  {
    _report.Fail(_face.sheetFile,
                 std::format("the cooked face does not read back as written: {} (AST-091)", readBack ? "it differs" : readBack.error()));
    return std::nullopt;
  }
  return entry;
}

} // namespace ContentBuild
