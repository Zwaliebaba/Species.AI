#include "pch.h"

#include "AlwaysOnServer.h"
#include "Files.h"
#include "Log.h"
#include "ProgramStart.h"
#include "SettingsFile.h"
#include "TextEncoding.h"
#include "WorldSaves.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <format>
#include <string>
#include <system_error>
#include <utility>

namespace Species
{

namespace
{

// The description's keys (DAT-196).
constexpr std::string_view SEED_KEY = "world.seed";
constexpr std::string_view SITE_KEY = "world.site";
constexpr std::string_view AI_SETTLEMENTS_KEY = "world.ai_settlements"; // O103: the AI settlements it was founded with
constexpr std::string_view RUN_ENDED_KEY = "world.run_ended";           // ADR-170: "true" once the client saw the run end
constexpr std::string_view PACE_KEY = "world.pace";                     // ADR-191: its pace's name, from M4
constexpr std::string_view TRUE_TEXT = "true";
constexpr std::uint64_t SITE_COUNT = 4;          // WLD-040: the region's settlement sites
constexpr std::uint32_t WORLD_NUMBER_MAX = 9999; // the most that WORLD_NUMBER_DIGITS digits hold

// The number of a world folder's name, or none when the name is not one.
std::optional<std::uint32_t> NumberOf(const std::filesystem::path& _folder)
{
  const std::wstring name = _folder.filename().wstring();
  if (name.size() != WORLD_FOLDER_PREFIX.size() + WORLD_NUMBER_DIGITS || !name.starts_with(WORLD_FOLDER_PREFIX))
  {
    return std::nullopt;
  }
  std::uint32_t number = 0;
  for (const wchar_t digit : std::wstring_view(name).substr(WORLD_FOLDER_PREFIX.size()))
  {
    if (digit < L'0' || digit > L'9')
    {
      return std::nullopt;
    }
    number = number * 10 + static_cast<std::uint32_t>(digit - L'0');
  }
  return number;
}

template <class T> std::optional<T> ParseNumber(std::string_view _text)
{
  T value{};
  const auto [end, error] = std::from_chars(_text.data(), _text.data() + _text.size(), value);
  return error == std::errc{} && end == _text.data() + _text.size() ? std::optional<T>(value) : std::nullopt;
}

// The world's description written through atomic replacement (PLT-089).
bool WriteDescription(const SavedWorld& _world)
{
  std::vector<Neuron::SettingsEntryText> entries{Neuron::SettingsEntryText{std::string(SITE_KEY), std::to_string(_world.site)}};
  if (_world.seed.has_value())
  {
    entries.push_back({std::string(SEED_KEY), std::to_string(*_world.seed)});
  }
  entries.push_back({std::string(AI_SETTLEMENTS_KEY), std::to_string(_world.aiSettlements)});
  if (_world.ended)
  {
    entries.push_back({std::string(RUN_ENDED_KEY), std::string(TRUE_TEXT)});
  }
  entries.push_back({std::string(PACE_KEY), std::string(PaceName(_world.pace))});
  const std::string text = Neuron::WriteSettingsText({}, entries);
  if (!Neuron::ReplaceFileAtomically(_world.folder / WORLD_DESCRIPTION_FILE, std::as_bytes(std::span(text))))
  {
    Neuron::Log(Neuron::LogCategory::Platform, Neuron::LogLevel::Error, "the description of {} could not be written",
                Neuron::ToUtf8(_world.folder.wstring()));
    return false;
  }
  return true;
}

// A world folder's description, or none when it does not read: a site within the region's four, and a seed that is a
// number and AI settlements at most AI_SETTLEMENTS_FULL when they are there.
std::optional<SavedWorld> ReadWorld(const std::filesystem::path& _folder, std::uint32_t _number)
{
  const auto bytes = Neuron::ReadWholeFile(_folder / WORLD_DESCRIPTION_FILE);
  if (!bytes)
  {
    return std::nullopt;
  }
  SavedWorld world;
  world.folder = _folder;
  world.number = _number;
  bool site = false;
  for (const Neuron::SettingsLine& line : Neuron::ReadSettingsLines(*bytes))
  {
    if (line.kind != Neuron::SettingsLine::Kind::Entry)
    {
      continue;
    }
    if (line.key == SITE_KEY)
    {
      const std::optional<std::uint64_t> value = ParseNumber<std::uint64_t>(line.value);
      if (!value || *value >= SITE_COUNT)
      {
        return std::nullopt;
      }
      world.site = static_cast<std::uint8_t>(*value);
      site = true;
    }
    else if (line.key == SEED_KEY)
    {
      world.seed = ParseNumber<std::uint64_t>(line.value);
      if (!world.seed)
      {
        return std::nullopt;
      }
    }
    else if (line.key == AI_SETTLEMENTS_KEY)
    {
      const std::optional<std::uint64_t> value = ParseNumber<std::uint64_t>(line.value);
      if (!value || *value > AI_SETTLEMENTS_FULL)
      {
        return std::nullopt;
      }
      world.aiSettlements = static_cast<std::uint8_t>(*value);
    }
    else if (line.key == RUN_ENDED_KEY)
    {
      world.ended = line.value == TRUE_TEXT;
    }
    else if (line.key == PACE_KEY)
    {
      const std::optional<WorldPace> pace = PaceNamed(line.value);
      if (!pace)
      {
        return std::nullopt;
      }
      world.pace = *pace;
    }
  }
  return site ? std::optional<SavedWorld>(std::move(world)) : std::nullopt;
}

} // namespace

std::optional<SavedWorld> DescribedWorld(const std::filesystem::path& _folder)
{
  return ReadWorld(_folder, NumberOf(_folder).value_or(0));
}

std::vector<SavedWorld> ListWorlds(const std::filesystem::path& _saves)
{
  std::vector<SavedWorld> worlds;
  std::error_code error;
  for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(_saves, error))
  {
    const std::optional<std::uint32_t> number = NumberOf(entry.path());
    if (!number || !entry.is_directory(error))
    {
      continue;
    }
    if (std::optional<SavedWorld> world = ReadWorld(entry.path(), *number))
    {
      worlds.push_back(std::move(*world));
    }
    else
    {
      Neuron::Log(Neuron::LogCategory::Platform, Neuron::LogLevel::Warning, "the world folder {} has no description that reads",
                  Neuron::ToUtf8(entry.path().wstring()));
    }
  }
  std::ranges::sort(worlds, std::ranges::greater{}, &SavedWorld::number);
  return worlds;
}

std::optional<SavedWorld> LatestWorld(const std::filesystem::path& _saves)
{
  for (SavedWorld& world : ListWorlds(_saves))
  {
    // ADR-191: an always-on world whose server runs can be continued before its first snapshot.
    const std::optional<ServerListing> listing = world.pace == WorldPace::AlwaysOn ? ReadServerListing(world.folder) : std::nullopt;
    if (!Neuron::ListFiles(world.folder, WORLD_SNAPSHOT_SUFFIX).empty() ||
        (listing.has_value() && Neuron::ProcessRunning(listing->processId)))
    {
      return std::move(world);
    }
  }
  return std::nullopt;
}

std::optional<SavedWorld> CreateWorld(const std::filesystem::path& _saves, std::optional<std::uint64_t> _seed, std::uint8_t _site,
                                      std::uint8_t _aiSettlements, WorldPace _pace)
{
  // Numbered above every folder of the form, a world or not, so that no folder is ever reused.
  std::uint32_t highest = 0;
  std::error_code error;
  for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(_saves, error))
  {
    highest = std::max(highest, NumberOf(entry.path()).value_or(0));
  }
  if (highest >= WORLD_NUMBER_MAX)
  {
    Neuron::Log(Neuron::LogCategory::Platform, Neuron::LogLevel::Error, "the saves folder holds world {}, the last that can be numbered",
                highest);
    return std::nullopt;
  }
  SavedWorld world;
  world.number = highest + 1;
  world.folder = _saves / std::format(L"{}{:0{}}", WORLD_FOLDER_PREFIX, world.number, WORLD_NUMBER_DIGITS);
  world.seed = _seed;
  world.site = _site;
  world.aiSettlements = _aiSettlements;
  world.pace = _pace;
  std::filesystem::create_directories(world.folder, error);
  if (error)
  {
    Neuron::Log(Neuron::LogCategory::Platform, Neuron::LogLevel::Error, "the world folder {} could not be made: {}",
                Neuron::ToUtf8(world.folder.wstring()), error.message());
    return std::nullopt;
  }
  return WriteDescription(world) ? std::optional<SavedWorld>(std::move(world)) : std::nullopt;
}

bool RecordWorldSeed(SavedWorld& _world, std::uint64_t _seed)
{
  _world.seed = _seed;
  return WriteDescription(_world);
}

bool RecordRunEnded(SavedWorld& _world)
{
  _world.ended = true;
  return WriteDescription(_world);
}

std::vector<SavedWorld> EndedWorlds(const std::filesystem::path& _saves)
{
  std::vector<SavedWorld> worlds = ListWorlds(_saves);
  std::erase_if(worlds,
                [](const SavedWorld& _world) { return !_world.ended || Neuron::ListFiles(_world.folder, WORLD_SNAPSHOT_SUFFIX).empty(); });
  return worlds;
}

} // namespace Species
