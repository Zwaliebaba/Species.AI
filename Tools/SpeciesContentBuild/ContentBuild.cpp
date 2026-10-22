#include "pch.h"

#include "AssetManifest.h"
#include "CollisionCooker.h"
#include "ContentBuild.h"
#include "ContentKey.h"
#include "Files.h"
#include "FontPacker.h"
#include "MeshCooker.h"
#include "PlaceholderTexture.h"
#include "RecordCooker.h"
#include "SimulationRecordCooker.h"
#include "StringTableCooker.h"
#include "TextEncoding.h"
#include "TextureCheck.h"

#include <algorithm>
#include <array>
#include <format>
#include <map>
#include <optional>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace ContentBuild
{

namespace
{

constexpr std::string_view CONTENT_ROOT = "GameData";
// GameData/Schemas/ holds the JSON Schema documents, which the generator reads and nothing packs (ADR-098).
constexpr std::string_view SCHEMA_FOLDER = "Schemas/";
constexpr std::string_view RECORD_FOLDER = "Records/";
// The manifest category of the string tables (DAT-206), and the face whose glyphs the en-US table's coverage is
// checked against: GameFont, which every font role of the MVP uses (UI-199 as O63 amends it, ADR-146).
constexpr std::string_view STRING_TABLE_CATEGORY = "String table";
constexpr std::string_view BASE_LANGUAGE_FACE = "font.ui.primary";

// The manifest categories whose files are textures (AST table 3.2-A).
constexpr std::array<std::string_view, 8> TEXTURE_CATEGORIES{"Unit sprites",
                                                             "VFX textures",
                                                             "UI panels and title",
                                                             "HUD, supply-crate and power icons",
                                                             "Cursors and markers",
                                                             "Input glyphs",
                                                             "Help images and explainer animations",
                                                             "Terrain ramps, water and waves"};

/// One file under the content root: its path from the content root and from the repository root, with forward
/// slashes, and its bytes.
struct Source
{
  std::string contentPath;
  std::string file;
  std::vector<std::byte> bytes;
};

std::string Generic(const std::filesystem::path& _path)
{
  std::string text = Neuron::ToUtf8(_path.wstring());
  std::ranges::replace(text, '\\', '/');
  return text;
}

// Every file under the content root, in the byte-wise order of its path, so a run reads them in the same order on
// every host (AST-066).
std::vector<Source> ReadSources(const std::filesystem::path& _contentRoot, BuildReport& _report)
{
  std::vector<Source> sources;
  std::error_code error;
  for (auto it = std::filesystem::recursive_directory_iterator(_contentRoot, error);
       !error && it != std::filesystem::recursive_directory_iterator(); it.increment(error))
  {
    if (!it->is_regular_file(error))
    {
      continue;
    }
    Source source;
    source.contentPath = Generic(std::filesystem::relative(it->path(), _contentRoot, error));
    source.file = std::format("{}/{}", CONTENT_ROOT, source.contentPath);
    sources.push_back(std::move(source));
  }
  if (error)
  {
    _report.Fail(CONTENT_ROOT, std::format("the content root cannot be read: {}", error.message()));
  }
  std::ranges::sort(sources, {}, &Source::contentPath);
  for (Source& source : sources)
  {
    auto bytes = Neuron::ReadWholeFile(_contentRoot / std::filesystem::path(Neuron::ToUtf16(source.contentPath)));
    if (!bytes)
    {
      _report.Fail(source.file, "the file cannot be read");
      continue;
    }
    source.bytes = std::move(*bytes);
  }
  return sources;
}

bool IsTextureCategory(std::string_view _category)
{
  return std::ranges::find(TEXTURE_CATEGORIES, _category) != TEXTURE_CATEGORIES.end();
}

// A face's members, the sheet and the table, as the run meets them (DAT-219 amendment).
struct FaceFiles
{
  const Source* sheet = nullptr;
  const ManifestRow* sheetRow = nullptr;
  const Source* table = nullptr;
  const ManifestRow* tableRow = nullptr;
};

} // namespace

BuildResult CookContent(const std::filesystem::path& _root, BuildReport& _report)
{
  BuildResult result;
  const auto manifestBytes = Neuron::ReadWholeFile(_root / std::filesystem::path(Neuron::ToUtf16(MANIFEST_FILE)));
  if (!manifestBytes)
  {
    _report.Fail(MANIFEST_FILE, "the asset manifest cannot be read (AST-012)");
    return result;
  }
  const auto manifest = AssetManifest::Read(*manifestBytes);
  if (!manifest)
  {
    _report.Fail(MANIFEST_FILE, std::format("{} (AST-012)", manifest.error()));
    return result;
  }
  const std::vector<Source> sources = ReadSources(_root / std::filesystem::path(Neuron::ToUtf16(CONTENT_ROOT)), _report);

  std::map<std::string, FaceFiles> faces;  // by asset id
  std::map<std::string, std::string> keys; // a key's canonical form → the file it came from
  std::vector<CookedGeometry> meshes;      // for the pairing pass of ADR-129
  std::vector<CookedGeometry> collisions;
  std::vector<std::pair<const ManifestRow*, const Source*>> stringTables; // cooked once the faces are, for their coverage
  for (const Source& source : sources)
  {
    if (source.contentPath.starts_with(SCHEMA_FOLDER))
    {
      continue;
    }
    std::optional<Neuron::CookedEntry> entry;
    const ManifestRow* row = manifest->RowOf(source.file);
    if (row != nullptr && row->category == "Fonts")
    {
      FaceFiles& face = faces[row->assetId];
      (source.file.ends_with(".json") ? face.table : face.sheet) = &source;
      (source.file.ends_with(".json") ? face.tableRow : face.sheetRow) = row;
      continue;
    }
    if (row != nullptr && row->category == STRING_TABLE_CATEGORY)
    {
      stringTables.emplace_back(row, &source);
      continue;
    }
    if (row != nullptr && row->category == "Meshes" && row->subcategory == COLLISION_SUBCATEGORY)
    {
      entry = CookCollision(*row, source.file, source.bytes, _report);
      if (entry)
      {
        collisions.push_back(CookedGeometry{row, source.file, result.entries.size()});
      }
    }
    else if (row != nullptr && row->category == "Meshes")
    {
      entry = CookMesh(*row, source.file, source.bytes, _report);
      if (entry)
      {
        meshes.push_back(CookedGeometry{row, source.file, result.entries.size()});
      }
    }
    else if (row != nullptr && IsTextureCategory(row->category) && source.file.ends_with(".dds"))
    {
      entry = CheckTexture(*row, source.file, source.bytes, _report);
    }
    else if (row != nullptr)
    {
      _report.Fail(source.file,
                   std::format("its row {} is of the category {}, whose cooker arrives after M1 (ADR-108)", row->assetId, row->category));
    }
    else if (source.contentPath.starts_with(RECORD_FOLDER) && source.contentPath.ends_with(".json"))
    {
      entry = CookRecord(source.file, source.contentPath, source.bytes, _report);
    }
    else
    {
      _report.Fail(source.file, "a stray file: it belongs to no manifest row and is no data record (AST-013)");
    }
    if (entry)
    {
      entry->entry.key = Neuron::KeyOfSource(source.contentPath);
      result.entries.push_back(std::move(*entry));
    }
  }
  CookSimulationRecords(*manifest, meshes, collisions, result.entries, _report);
  std::optional<Neuron::FontAsset> baseLanguageFace;
  for (const auto& [assetId, face] : faces)
  {
    if (face.sheet == nullptr || face.table == nullptr)
    {
      const std::string& file = face.sheet != nullptr ? face.sheet->file : (face.table != nullptr ? face.table->file : assetId);
      _report.Fail(file, std::format("the face {} needs both its glyph sheet and its glyph table (DAT-219, O63)", assetId));
      continue;
    }
    const FaceSources members{face.sheetRow, face.sheet->file, face.sheet->bytes, face.tableRow, face.table->file, face.table->bytes};
    std::optional<Neuron::CookedEntry> entry = PackFont(members, _report);
    if (entry)
    {
      // The face's one entry takes the sheet's key; its table is folded into it (ADR-108).
      entry->entry.key = Neuron::KeyOfSource(face.sheet->contentPath);
      if (assetId == BASE_LANGUAGE_FACE)
      {
        if (auto cooked = Neuron::ReadFontAsset(entry->bytes))
        {
          baseLanguageFace = std::move(*cooked);
        }
      }
      result.entries.push_back(std::move(*entry));
    }
  }
  // DAT-217 (ADR-146): each language's table, its coverage checked against the face its font roles use.
  for (const auto& [row, source] : stringTables)
  {
    if (row->assetId != "strings.en_us")
    {
      _report.Fail(source->file, std::format("its row {} is a language the MVP's faces do not draw (UI-199, ADR-146)", row->assetId));
      continue;
    }
    if (!baseLanguageFace)
    {
      _report.Fail(source->file, std::format("its coverage needs the face {}, which did not cook (UI-199)", BASE_LANGUAGE_FACE));
      continue;
    }
    std::optional<Neuron::CookedEntry> entry = CookStringTable(*row, source->file, source->bytes, *baseLanguageFace, _report);
    if (entry)
    {
      entry->entry.key = Neuron::KeyOfSource(source->contentPath);
      result.entries.push_back(std::move(*entry));
    }
  }
  // AST-014, AST-015: every key is canonical, and no two sources give one key, whatever their case.
  for (const Neuron::CookedEntry& entry : result.entries)
  {
    const auto source =
      std::ranges::find_if(sources, [&](const Source& _source) { return Neuron::KeyOfSource(_source.contentPath) == entry.entry.key; });
    const std::string file = source == sources.end() ? entry.entry.key : source->file;
    if (!Neuron::IsCanonicalKey(entry.entry.key))
    {
      _report.Fail(file, std::format("its key {} holds a character outside a-z, 0-9, _, ., / and @ or passes {} bytes (AST-014)",
                                     entry.entry.key, Neuron::MAX_KEY_BYTES));
    }
    if (const auto [at, added] = keys.emplace(entry.entry.key, file); !added)
    {
      _report.Fail(file, std::format("its key {} is the key of {} too (AST-015)", entry.entry.key, at->second));
    }
  }
  // AST-082 (O88, ADR-140): a row of the MVP's with no authored file takes a stand-in at its key, when the build makes one.
  for (const ManifestRow& row : manifest->Rows())
  {
    const bool authored = std::ranges::any_of(sources, [&](const Source& _source) { return _source.file == row.speciesFilename; });
    if (authored)
    {
      continue;
    }
    ++result.rowsWithoutFile;
    if (std::optional<Neuron::CookedEntry> standIn = MakePlaceholderTexture(row))
    {
      // A stand-in meets its row as an authored file must, or the build fails with the ways it does not.
      if (!CheckTexture(row, row.speciesFilename, standIn->bytes, _report))
      {
        continue;
      }
      if (const auto [at, added] = keys.emplace(standIn->entry.key, row.speciesFilename); !added)
      {
        _report.Fail(row.speciesFilename,
                     std::format("its stand-in's key {} is the key of {} too (AST-015)", standIn->entry.key, at->second));
        continue;
      }
      result.entries.push_back(std::move(*standIn));
      ++result.placeholders;
    }
  }
  return result;
}

} // namespace ContentBuild
