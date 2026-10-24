#pragma once

#include "Sha256.h"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Neuron
{

enum class FileError : std::uint8_t
{
  NotFound,
  AccessDenied,
  DiskFull,
  Other
};

// The whole file, or why it could not be read. Safe on any thread for distinct files (PLT-088).
[[nodiscard]] std::expected<std::vector<std::byte>, FileError> ReadWholeFile(const std::filesystem::path& _path);

// Writes the bytes to a new temporary file in the target's folder, flushes it to the device and replaces the target
// with it in one step; on any failure the target keeps its previous content and the temporary file is deleted
// (PLT-089). Every writer of settings, custom maps and development outputs uses it.
[[nodiscard]] std::expected<void, FileError> ReplaceFileAtomically(const std::filesystem::path& _path, std::span<const std::byte> _bytes);

// The files in a folder whose names end with the suffix, an ASCII one, in the order of their names' UTF-16 code units;
// none when the folder does not exist.
[[nodiscard]] std::vector<std::filesystem::path> ListFiles(const std::filesystem::path& _folder, std::string_view _suffix);

// Removes a file; a file that does not exist is already removed.
[[nodiscard]] std::expected<void, FileError> RemoveFile(const std::filesystem::path& _path);

// The folder of the running executable.
[[nodiscard]] std::filesystem::path ExecutableFolder();

// The stamp of the named files in a folder: the SHA-256 over each one's name, size and last write time, in the order
// given, a missing file adding its name alone. Any build that writes one of them changes it (ADR-189).
[[nodiscard]] Sha256Digest FilesStamp(const std::filesystem::path& _folder, std::span<const std::wstring_view> _names);

} // namespace Neuron
