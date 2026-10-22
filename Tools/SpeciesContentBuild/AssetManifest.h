#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ContentBuild
{

// Where the content build reads the asset manifest, relative to the repository root (AST-012).
inline constexpr std::string_view MANIFEST_FILE = "Design/assets/asset_manifest.csv";

/// One row of the asset manifest: the columns that the content build reads, as table 2.4-A of the asset pipeline
/// states their use, and the row's line in the file.
struct ManifestRow
{
  std::string assetId;
  std::string category;
  std::string subcategory;
  std::string targetFormat;
  std::string spec;
  std::string colorSpace; // the column `colour_space`
  std::string alphaMode;
  std::string variants;
  std::string speciesFilename; // the source under GameData/, which gives the member's key (AST-014)
  std::string sourceFormat;
  std::string priority;    // the stage that first ships it (AST table 2.4-A)
  std::string placeholder; // what stands in for it until its file is authored (GameAssets.md §2.6)
  std::uint32_t line = 0;
};

/// The asset manifest, `Design/assets/asset_manifest.csv`: one row per file, with a header naming its columns
/// (AST-012). The file is RFC 4180 CSV: a field with a comma, a quote or a line break is quoted, and a quote inside it
/// is doubled.
class AssetManifest
{
public:
  // The manifest, or why its text is not one: a quote left open, a row of another column count, or a column the
  // content build reads missing from the header.
  [[nodiscard]] static std::expected<AssetManifest, std::string> Read(std::span<const std::byte> _csv);

  // The row of a source, by its path from the repository root ("GameData/Fonts/GameFont.dds"); null when no row names it.
  [[nodiscard]] const ManifestRow* RowOf(std::string_view _speciesFilename) const;
  // The rows of one asset, in member order (AST-016).
  [[nodiscard]] std::vector<const ManifestRow*> MembersOf(std::string_view _assetId) const;
  [[nodiscard]] std::span<const ManifestRow> Rows() const noexcept
  {
    return m_rows;
  }

private:
  std::vector<ManifestRow> m_rows;
};

} // namespace ContentBuild
