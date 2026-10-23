#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string_view>
#include <vector>

namespace Species
{

// PLT-017 (ADR-155): the user-data root's folder of saved worlds, made on first use, and in it one folder per world.
inline constexpr std::wstring_view SAVES_FOLDER = L"saves";
// A world's folder is this prefix and its number in WORLD_NUMBER_DIGITS digits, so that the names sort by number.
inline constexpr std::wstring_view WORLD_FOLDER_PREFIX = L"world-";
inline constexpr std::size_t WORLD_NUMBER_DIGITS = 4;
// A world's description, in DAT-196's form: the seed it was made from, once known, and its steward's site.
inline constexpr std::wstring_view WORLD_DESCRIPTION_FILE = L"world.txt";
// The suffix of a world's snapshots (ADR-099), which make a world one that can be continued.
inline constexpr std::string_view WORLD_SNAPSHOT_SUFFIX = ".snapshot";

/// A world of the saves folder (MVP-103, ADR-155): its folder, its number, and what its description holds.
struct SavedWorld
{
  std::filesystem::path folder;
  std::uint32_t number = 0;
  std::optional<std::uint64_t> seed; // MVP-005: none until the world's terrain gave it
  std::uint8_t site = 0;             // MVP-012: the steward's site
};

// The worlds of _saves, the highest number first. A folder whose name or description does not read is no world; it is
// logged and left as it is.
[[nodiscard]] std::vector<SavedWorld> ListWorlds(const std::filesystem::path& _saves);
// MVP-103: the world Continue opens, the highest-numbered one that holds a snapshot; none when no world does.
[[nodiscard]] std::optional<SavedWorld> LatestWorld(const std::filesystem::path& _saves);
// A new world's folder in _saves, numbered one above the highest, with its description; none, logged, when it cannot be
// made. _seed is the steward's, or none while the server draws it.
[[nodiscard]] std::optional<SavedWorld> CreateWorld(const std::filesystem::path& _saves, std::optional<std::uint64_t> _seed,
                                                    std::uint8_t _site);
// MVP-005: the world's seed, as its terrain gave it, written into its description; false, logged, when it is not written.
bool RecordWorldSeed(SavedWorld& _world, std::uint64_t _seed);

} // namespace Species
