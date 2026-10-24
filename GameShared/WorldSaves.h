#pragma once

#include "WorldPace.h"

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
// O103: a new world's AI settlements, one at each other site (MVP-013), unless the steward chose none.
inline constexpr std::uint8_t AI_SETTLEMENTS_FULL = 3;
// A world's folder is this prefix and its number in WORLD_NUMBER_DIGITS digits, so that the names sort by number.
inline constexpr std::wstring_view WORLD_FOLDER_PREFIX = L"world-";
inline constexpr std::size_t WORLD_NUMBER_DIGITS = 4;
// A world's description, in DAT-196's form: the seed it was made from, once known, its steward's site, and from M3 its AI
// settlements (O103) and whether its run has ended (ADR-170); from M4 its pace (ADR-190).
inline constexpr std::wstring_view WORLD_DESCRIPTION_FILE = L"world.txt";
// The suffix of a world's snapshots (ADR-099), which make a world one that can be continued.
inline constexpr std::string_view WORLD_SNAPSHOT_SUFFIX = ".snapshot";

/// A world of the saves folder (MVP-103, ADR-155): its folder, its number, and what its description holds.
struct SavedWorld
{
  std::filesystem::path folder;
  std::uint32_t number = 0;
  std::optional<std::uint64_t> seed;   // MVP-005: none until the world's terrain gave it
  std::uint8_t site = 0;               // MVP-012: the steward's site
  std::uint8_t aiSettlements = 0;      // MVP-013 (O103): its AI settlements, none for a world described before M3
  bool ended = false;                  // MVP-063 (ADR-170): its run has ended, which the client recorded when it saw it
  WorldPace pace = WorldPace::Session; // MVP-071 (ADR-190): the session pace for a world described before M4
};

// ADR-190: the world of one folder, as its description reads, numbered by its name when it has a world folder's name and
// 0 otherwise; none when the description does not read.
[[nodiscard]] std::optional<SavedWorld> DescribedWorld(const std::filesystem::path& _folder);
// The worlds of _saves, the highest number first. A folder whose name or description does not read is no world; it is
// logged and left as it is.
[[nodiscard]] std::vector<SavedWorld> ListWorlds(const std::filesystem::path& _saves);
// MVP-103: the world Continue opens, the highest-numbered one that holds a snapshot or, at the always-on pace, whose
// server runs (ADR-190); none when no world does.
[[nodiscard]] std::optional<SavedWorld> LatestWorld(const std::filesystem::path& _saves);
// A new world's folder in _saves, numbered one above the highest, with its description; none, logged, when it cannot be
// made. _seed is the steward's, or none while the server draws it; _aiSettlements its AI settlements (O103); _pace its
// pace (MVP-071).
[[nodiscard]] std::optional<SavedWorld> CreateWorld(const std::filesystem::path& _saves, std::optional<std::uint64_t> _seed,
                                                    std::uint8_t _site, std::uint8_t _aiSettlements, WorldPace _pace = WorldPace::Session);
// MVP-005: the world's seed, as its terrain gave it, written into its description; false, logged, when it is not written.
bool RecordWorldSeed(SavedWorld& _world, std::uint64_t _seed);
// MVP-063 (ADR-170): the world's run has ended, written into its description; false, logged, when it is not written.
bool RecordRunEnded(SavedWorld& _world);
// MVP-063: the worlds whose run has ended and that hold a snapshot, the highest number first, whose epilogues the front
// end opens.
[[nodiscard]] std::vector<SavedWorld> EndedWorlds(const std::filesystem::path& _saves);

} // namespace Species
