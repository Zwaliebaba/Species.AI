#include "pch.h"

#include "Files.h"

#include <algorithm>
#include <atomic>
#include <format>
#include <string>
#include <system_error>
#include <vector>

namespace Neuron
{

namespace
{

// The largest single read or write: a DWORD count, rounded down to a whole mebibyte.
constexpr std::size_t IO_CHUNK_BYTES = 0x7FF00000;

std::atomic<std::uint64_t> g_temporaryFileCount{0};

FileError ToFileError(DWORD _error) noexcept
{
  switch (_error)
  {
  case ERROR_FILE_NOT_FOUND:
  case ERROR_PATH_NOT_FOUND:
    return FileError::NotFound;
  case ERROR_ACCESS_DENIED:
  case ERROR_SHARING_VIOLATION:
  case ERROR_LOCK_VIOLATION:
    return FileError::AccessDenied;
  case ERROR_DISK_FULL:
  case ERROR_HANDLE_DISK_FULL:
    return FileError::DiskFull;
  default:
    return FileError::Other;
  }
}

/// A file handle that is closed when it goes out of scope.
class FileHandle
{
public:
  explicit FileHandle(HANDLE _handle) noexcept
    : m_handle(_handle)
  {
  }

  ~FileHandle()
  {
    if (IsOpen())
    {
      CloseHandle(m_handle);
    }
  }

  FileHandle(const FileHandle&) = delete;
  FileHandle& operator=(const FileHandle&) = delete;

  [[nodiscard]] bool IsOpen() const noexcept
  {
    return m_handle != INVALID_HANDLE_VALUE && m_handle != nullptr;
  }
  [[nodiscard]] HANDLE Get() const noexcept
  {
    return m_handle;
  }

  // Closes the handle now; false when closing reported an error.
  bool Close() noexcept
  {
    const bool closed = IsOpen() && CloseHandle(m_handle) != FALSE;
    m_handle = INVALID_HANDLE_VALUE;
    return closed;
  }

private:
  HANDLE m_handle;
};

bool WriteAll(HANDLE _file, std::span<const std::byte> _bytes) noexcept
{
  while (!_bytes.empty())
  {
    const std::size_t partBytes = _bytes.size() < IO_CHUNK_BYTES ? _bytes.size() : IO_CHUNK_BYTES;
    DWORD writtenBytes = 0;
    if (!WriteFile(_file, _bytes.data(), static_cast<DWORD>(partBytes), &writtenBytes, nullptr) || writtenBytes == 0)
    {
      return false;
    }
    _bytes = _bytes.subspan(writtenBytes);
  }
  return true;
}

} // namespace

std::expected<std::vector<std::byte>, FileError> ReadWholeFile(const std::filesystem::path& _path)
{
  // Others may write or replace the file meanwhile, a log or a settings file among them; the read is a snapshot.
  FileHandle file(CreateFileW(_path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr));
  if (!file.IsOpen())
  {
    return std::unexpected(ToFileError(GetLastError()));
  }
  LARGE_INTEGER sizeBytes{};
  if (!GetFileSizeEx(file.Get(), &sizeBytes) || sizeBytes.QuadPart < 0)
  {
    return std::unexpected(ToFileError(GetLastError()));
  }
  std::vector<std::byte> bytes(static_cast<std::size_t>(sizeBytes.QuadPart));
  std::size_t readBytes = 0;
  while (readBytes < bytes.size())
  {
    const std::size_t remaining = bytes.size() - readBytes;
    const std::size_t partBytes = remaining < IO_CHUNK_BYTES ? remaining : IO_CHUNK_BYTES;
    DWORD partRead = 0;
    if (!ReadFile(file.Get(), bytes.data() + readBytes, static_cast<DWORD>(partBytes), &partRead, nullptr))
    {
      return std::unexpected(ToFileError(GetLastError()));
    }
    if (partRead == 0)
    {
      // The file shrank while it was read: what was read is the file.
      bytes.resize(readBytes);
      break;
    }
    readBytes += partRead;
  }
  return bytes;
}

std::expected<void, FileError> ReplaceFileAtomically(const std::filesystem::path& _path, std::span<const std::byte> _bytes)
{
  // Unique per process and per call, so two writers of one target never share a temporary file (PLT-041).
  std::filesystem::path temporary = _path;
  temporary += std::format(L".{}.{}.tmp", GetCurrentProcessId(), g_temporaryFileCount.fetch_add(1, std::memory_order_relaxed));

  FileHandle file(CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
  if (!file.IsOpen())
  {
    return std::unexpected(ToFileError(GetLastError()));
  }
  DWORD error = ERROR_SUCCESS;
  if (!WriteAll(file.Get(), _bytes) || !FlushFileBuffers(file.Get()))
  {
    error = GetLastError();
  }
  if (!file.Close() && error == ERROR_SUCCESS)
  {
    error = GetLastError();
  }
  if (error == ERROR_SUCCESS)
  {
    if (ReplaceFileW(_path.c_str(), temporary.c_str(), nullptr, REPLACEFILE_IGNORE_MERGE_ERRORS, nullptr, nullptr))
    {
      return {};
    }
    error = GetLastError();
    // No target yet: a write-through move puts the new file in place (PLT-089).
    if ((error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) &&
        MoveFileExW(temporary.c_str(), _path.c_str(), MOVEFILE_WRITE_THROUGH))
    {
      return {};
    }
    if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)
    {
      error = GetLastError();
    }
  }
  DeleteFileW(temporary.c_str());
  return std::unexpected(ToFileError(error == ERROR_SUCCESS ? ERROR_GEN_FAILURE : error));
}

std::vector<std::filesystem::path> ListFiles(const std::filesystem::path& _folder, std::string_view _suffix)
{
  std::vector<std::filesystem::path> files;
  std::error_code error;
  // The names are compared as the system holds them, UTF-16, so a name the narrow code page cannot spell is no error.
  const std::wstring suffix(_suffix.begin(), _suffix.end());
  for (std::filesystem::directory_iterator entry(_folder, error), end; !error && entry != end; entry.increment(error))
  {
    if (entry->is_regular_file(error) && entry->path().filename().native().ends_with(suffix))
    {
      files.push_back(entry->path());
    }
  }
  std::ranges::sort(files, [](const std::filesystem::path& _left, const std::filesystem::path& _right)
                    { return _left.filename().native() < _right.filename().native(); });
  return files;
}

std::expected<void, FileError> RemoveFile(const std::filesystem::path& _path)
{
  if (DeleteFileW(_path.c_str()))
  {
    return {};
  }
  const DWORD error = GetLastError();
  if (error == ERROR_FILE_NOT_FOUND)
  {
    return {};
  }
  return std::unexpected(ToFileError(error));
}

std::filesystem::path ExecutableFolder()
{
  std::vector<wchar_t> path(MAX_PATH);
  while (true)
  {
    const DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if (length == 0)
    {
      return {};
    }
    if (length < path.size())
    {
      return std::filesystem::path(std::wstring(path.data(), length)).parent_path();
    }
    path.resize(path.size() * 2);
  }
}

} // namespace Neuron
