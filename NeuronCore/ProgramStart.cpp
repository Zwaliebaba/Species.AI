#include "pch.h"

#include "ProgramStart.h"

#include <format>
#include <vector>

namespace Neuron
{

namespace
{

// One argument as CommandLineToArgvW reads it: in quotes, each run of backslashes doubled before a quote or the end,
// and each quote escaped.
void AppendQuoted(std::wstring& _line, std::wstring_view _argument)
{
  _line += L'"';
  std::size_t backslashes = 0;
  for (const wchar_t letter : _argument)
  {
    if (letter == L'\\')
    {
      ++backslashes;
      continue;
    }
    _line.append(letter == L'"' ? 2 * backslashes + 1 : backslashes, L'\\');
    backslashes = 0;
    _line += letter;
  }
  _line.append(2 * backslashes, L'\\');
  _line += L'"';
}

} // namespace

std::wstring CommandLineOf(const std::filesystem::path& _program, std::span<const std::wstring> _arguments)
{
  std::wstring line;
  AppendQuoted(line, _program.native());
  for (const std::wstring& argument : _arguments)
  {
    line += L' ';
    AppendQuoted(line, argument);
  }
  return line;
}

std::expected<std::uint32_t, std::string> StartInOwnConsole(const std::filesystem::path& _program, std::span<const std::wstring> _arguments)
{
  // CreateProcessW may write into the command line, so it is a buffer of its own.
  const std::wstring line = CommandLineOf(_program, _arguments);
  std::vector<wchar_t> buffer(line.begin(), line.end());
  buffer.push_back(L'\0');
  const std::filesystem::path folder = _program.parent_path();
  STARTUPINFOW startup{};
  startup.cb = sizeof(startup);
  PROCESS_INFORMATION started{};
  if (CreateProcessW(_program.c_str(), buffer.data(), nullptr, nullptr, FALSE, CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT, nullptr,
                     folder.c_str(), &startup, &started) == FALSE)
  {
    return std::unexpected(std::format("CreateProcessW failed with error {}", GetLastError()));
  }
  static_cast<void>(CloseHandle(started.hThread));
  static_cast<void>(CloseHandle(started.hProcess));
  return static_cast<std::uint32_t>(started.dwProcessId);
}

bool ProcessRunning(std::uint32_t _processId)
{
  HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, _processId);
  if (process == nullptr)
  {
    return false;
  }
  DWORD exitCode = 0;
  const bool running = GetExitCodeProcess(process, &exitCode) != FALSE && exitCode == STILL_ACTIVE;
  static_cast<void>(CloseHandle(process));
  return running;
}

std::uint32_t CurrentProcessId() noexcept
{
  return static_cast<std::uint32_t>(GetCurrentProcessId());
}

} // namespace Neuron
