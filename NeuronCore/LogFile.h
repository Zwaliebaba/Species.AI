#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string_view>

namespace Neuron
{

/// The log file of one run (DBG-049): UTF-8, in the logs folder, which keeps the newest dbg.log.files_kept files.
class LogFile
{
public:
  LogFile() = default;
  ~LogFile();

  LogFile(const LogFile&) = delete;
  LogFile& operator=(const LogFile&) = delete;

  // Creates the folder if needed, deletes the oldest log files so that at most _filesKept remain with the new one, and
  // creates the new file. Returns the Windows error code, 0 on success.
  [[nodiscard]] std::uint32_t Open(const std::filesystem::path& _directory, const std::filesystem::path& _fileName, std::size_t _filesKept);

  void Write(std::string_view _text) noexcept;
  // Flushes the operating system's buffers to the device: the fatal path only.
  void Flush() noexcept;

  [[nodiscard]] bool IsOpen() const noexcept
  {
    return m_handle != nullptr;
  }
  [[nodiscard]] const std::filesystem::path& Path() const noexcept
  {
    return m_path;
  }

private:
  void* m_handle = nullptr;
  std::filesystem::path m_path;
};

// Deletes the oldest `.log` files of the folder, by name, until at most _keep remain. Log file names begin with the
// UTC start time, so name order is age order.
void PruneLogFiles(const std::filesystem::path& _directory, std::size_t _keep);

} // namespace Neuron
