#pragma once

#include <cstdint>
#include <expected>
#include <filesystem>
#include <optional>
#include <string_view>

namespace Neuron
{

// PLT-017, PLT-031: the settings folder of the user-data root and the settings file in it.
inline constexpr std::wstring_view SETTINGS_FOLDER = L"settings";
inline constexpr std::wstring_view SETTINGS_FILE = L"settings.txt";

enum class UserDataFault : std::uint8_t
{
  Unavailable,  // PLT-018: the known folder cannot be resolved, or the root cannot be created or written
  InsideInstall // PLT-025: `--user-data` names a directory inside the install directory
};

// PLT-016, PLT-018, PLT-025: the user-data root, created when it is missing: the directory of `--user-data` when the
// option is given, else the Local AppData known folder with the product's folder (ADR-150, DEC-1004). A root that is
// created and written is the root; one that is not is Unavailable, and the client keeps its settings in memory.
[[nodiscard]] std::expected<std::filesystem::path, UserDataFault> ResolveUserDataRoot(const std::optional<std::filesystem::path>& _option,
                                                                                      std::wstring_view _product);

// Whether _path is _folder or lies inside it, compared without regard to case, after both are made absolute and
// normal.
[[nodiscard]] bool IsInsideFolder(const std::filesystem::path& _path, const std::filesystem::path& _folder);

// PLT-017: a folder of the user-data root, created on first use; none when it cannot be created.
[[nodiscard]] std::optional<std::filesystem::path> UserDataFolder(const std::filesystem::path& _root, std::wstring_view _name);

} // namespace Neuron
