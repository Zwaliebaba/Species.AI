#include "pch.h"

#include "AssetManifest.h"

#include <algorithm>
#include <array>
#include <format>
#include <optional>

namespace ContentBuild
{

namespace
{

// The columns the content build reads, in the order of ManifestRow (AST table 2.4-A).
constexpr std::array<std::string_view, 12> COLUMNS{"asset_id",     "category",   "subcategory", "target_format",    "spec",
                                                   "colour_space", "alpha_mode", "variants",    "species_filename", "source_format",
                                                   "priority",     "placeholder"};

/// Splits RFC 4180 text into records of fields.
class CsvReader
{
public:
  explicit CsvReader(std::string_view _text) noexcept
    : m_text(_text)
  {
  }

  // The next record and the line it starts on; none at the end of the text.
  std::expected<std::optional<std::vector<std::string>>, std::string> Next()
  {
    if (m_at >= m_text.size())
    {
      return std::optional<std::vector<std::string>>{};
    }
    std::vector<std::string> fields(1);
    bool quoted = false;
    while (m_at < m_text.size())
    {
      const char c = m_text[m_at++];
      if (quoted)
      {
        if (c == '"' && m_at < m_text.size() && m_text[m_at] == '"')
        {
          fields.back().push_back('"');
          ++m_at;
        }
        else if (c == '"')
        {
          quoted = false;
        }
        else
        {
          if (c == '\n')
          {
            ++m_line;
          }
          fields.back().push_back(c);
        }
      }
      else if (c == '"' && fields.back().empty())
      {
        quoted = true;
      }
      else if (c == ',')
      {
        fields.emplace_back();
      }
      else if (c == '\n' || c == '\r')
      {
        if (c == '\r' && m_at < m_text.size() && m_text[m_at] == '\n')
        {
          ++m_at;
        }
        ++m_line;
        return std::optional<std::vector<std::string>>{std::move(fields)};
      }
      else
      {
        fields.back().push_back(c);
      }
    }
    if (quoted)
    {
      return std::unexpected(std::format("line {}: a quoted field is not closed", m_line));
    }
    return std::optional<std::vector<std::string>>{std::move(fields)};
  }

  [[nodiscard]] std::uint32_t Line() const noexcept
  {
    return m_line;
  }

private:
  std::string_view m_text;
  std::size_t m_at = 0;
  std::uint32_t m_line = 1;
};

} // namespace

std::expected<AssetManifest, std::string> AssetManifest::Read(std::span<const std::byte> _csv)
{
  std::string_view text(reinterpret_cast<const char*>(_csv.data()), _csv.size());
  if (text.starts_with("\xEF\xBB\xBF"))
  {
    text.remove_prefix(3);
  }
  CsvReader reader(text);
  auto header = reader.Next();
  if (!header || !*header)
  {
    return std::unexpected(header ? std::string("the manifest has no header") : header.error());
  }
  const std::vector<std::string>& names = **header;
  std::array<std::size_t, COLUMNS.size()> index{};
  for (std::size_t c = 0; c < COLUMNS.size(); ++c)
  {
    const auto found = std::ranges::find(names, COLUMNS[c]);
    if (found == names.end())
    {
      return std::unexpected(std::format("the header has no column {}", COLUMNS[c]));
    }
    index[c] = static_cast<std::size_t>(found - names.begin());
  }
  AssetManifest manifest;
  while (true)
  {
    const std::uint32_t line = reader.Line();
    auto record = reader.Next();
    if (!record)
    {
      return std::unexpected(record.error());
    }
    if (!*record)
    {
      break;
    }
    const std::vector<std::string>& fields = **record;
    if (fields.size() == 1 && fields.front().empty())
    {
      continue; // a blank line
    }
    if (fields.size() != names.size())
    {
      return std::unexpected(std::format("line {}: {} fields where the header names {}", line, fields.size(), names.size()));
    }
    ManifestRow row;
    row.assetId = fields[index[0]];
    row.category = fields[index[1]];
    row.subcategory = fields[index[2]];
    row.targetFormat = fields[index[3]];
    row.spec = fields[index[4]];
    row.colorSpace = fields[index[5]];
    row.alphaMode = fields[index[6]];
    row.variants = fields[index[7]];
    row.speciesFilename = fields[index[8]];
    row.sourceFormat = fields[index[9]];
    row.priority = fields[index[10]];
    row.placeholder = fields[index[11]];
    row.line = line;
    manifest.m_rows.push_back(std::move(row));
  }
  return manifest;
}

const ManifestRow* AssetManifest::RowOf(std::string_view _speciesFilename) const
{
  const auto found = std::ranges::find(m_rows, _speciesFilename, &ManifestRow::speciesFilename);
  return found == m_rows.end() ? nullptr : &*found;
}

std::vector<const ManifestRow*> AssetManifest::MembersOf(std::string_view _assetId) const
{
  std::vector<const ManifestRow*> members;
  for (const ManifestRow& row : m_rows)
  {
    if (row.assetId == _assetId)
    {
      members.push_back(&row);
    }
  }
  return members;
}

} // namespace ContentBuild
