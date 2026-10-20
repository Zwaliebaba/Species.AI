#include "pch.h"

#include "LogFile.h"

#include <algorithm>
#include <string>
#include <system_error>
#include <vector>

namespace Neuron
{

LogFile::~LogFile()
{
  if (m_handle != nullptr)
  {
    CloseHandle(m_handle);
  }
}

std::uint32_t LogFile::Open(const std::filesystem::path& _directory, const std::filesystem::path& _fileName, std::size_t _filesKept)
{
  std::error_code error;
  (void)std::filesystem::create_directories(_directory, error);
  PruneLogFiles(_directory, _filesKept > 0 ? _filesKept - 1 : 0);
  m_path = _directory / _fileName;
  // Others may read the file while it is written, so a developer can follow it.
  HANDLE handle = CreateFileW(m_path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (handle == INVALID_HANDLE_VALUE)
  {
    return GetLastError();
  }
  m_handle = handle;
  return 0;
}

void LogFile::Write(std::string_view _text) noexcept
{
  while (m_handle != nullptr && !_text.empty())
  {
    const DWORD partBytes = _text.size() < 0x40000000 ? static_cast<DWORD>(_text.size()) : 0x40000000;
    DWORD writtenBytes = 0;
    if (!WriteFile(m_handle, _text.data(), partBytes, &writtenBytes, nullptr) || writtenBytes == 0)
    {
      return;
    }
    _text.remove_prefix(writtenBytes);
  }
}

void LogFile::Flush() noexcept
{
  if (m_handle != nullptr)
  {
    FlushFileBuffers(m_handle);
  }
}

void PruneLogFiles(const std::filesystem::path& _directory, std::size_t _keep)
{
  std::error_code error;
  std::vector<std::filesystem::path> files;
  for (std::filesystem::directory_iterator entry(_directory, error), end; !error && entry != end; entry.increment(error))
  {
    if (entry->is_regular_file(error) && entry->path().extension() == L".log")
    {
      files.push_back(entry->path());
    }
  }
  if (files.size() <= _keep)
  {
    return;
  }
  std::sort(files.begin(), files.end());
  for (std::size_t index = 0; index < files.size() - _keep; ++index)
  {
    (void)std::filesystem::remove(files[index], error);
  }
}

} // namespace Neuron
