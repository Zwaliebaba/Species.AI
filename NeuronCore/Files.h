#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <span>
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

} // namespace Neuron
