#include "pch.h"

#include <shellapi.h>

#include "Development.h"
#include "Files.h"
#include "StartupOptions.h"
#include "TextEncoding.h"

#include <limits>

#pragma comment(lib, "shell32.lib")

namespace Neuron
{

namespace
{

#if ENGINE_DEVELOPMENT
constexpr std::wstring_view LOOSE_CONTENT_OPTION = L"--loose-content";
constexpr std::wstring_view SEED_OPTION = L"--seed";

// A decimal number of digits only that fits 64 bits, or none.
std::optional<std::uint64_t> DecimalOf(std::wstring_view _text) noexcept
{
  if (_text.empty())
  {
    return std::nullopt;
  }
  std::uint64_t value = 0;
  for (const wchar_t digit : _text)
  {
    if (digit < L'0' || digit > L'9')
    {
      return std::nullopt;
    }
    const auto next = static_cast<std::uint64_t>(digit - L'0');
    if (value > (std::numeric_limits<std::uint64_t>::max() - next) / 10)
    {
      return std::nullopt;
    }
    value = value * 10 + next;
  }
  return value;
}
#endif

// Frees the array CommandLineToArgvW returns.
struct ArgumentArray
{
  wchar_t** values = nullptr;
  int count = 0;

  ArgumentArray() = default;
  ArgumentArray(const ArgumentArray&) = delete;
  ArgumentArray& operator=(const ArgumentArray&) = delete;
  ~ArgumentArray()
  {
    if (values != nullptr)
    {
      LocalFree(reinterpret_cast<HLOCAL>(values));
    }
  }
};

} // namespace

StartupOptions ParseArguments(std::span<const std::wstring> _arguments)
{
  StartupOptions options;
  for (std::size_t i = 0; i < _arguments.size(); ++i)
  {
#if ENGINE_DEVELOPMENT
    if (_arguments[i] == LOOSE_CONTENT_OPTION && i + 1 < _arguments.size())
    {
      options.looseContentDirectory = std::filesystem::path(_arguments[i + 1]);
      ++i;
      continue;
    }
    if (_arguments[i] == SEED_OPTION && i + 1 < _arguments.size())
    {
      if (const std::optional<std::uint64_t> seed = DecimalOf(_arguments[i + 1]))
      {
        options.worldSeed = seed;
        ++i;
        continue;
      }
    }
#endif
    options.ignoredArguments.push_back(ToUtf8(_arguments[i]));
  }
  return options;
}

StartupOptions ParseCommandLine()
{
  ArgumentArray array;
  array.values = CommandLineToArgvW(GetCommandLineW(), &array.count);
  std::vector<std::wstring> arguments;
  for (int i = 1; array.values != nullptr && i < array.count; ++i)
  {
    arguments.emplace_back(array.values[i]);
  }
  return ParseArguments(arguments);
}

std::filesystem::path ContentDirectory(const StartupOptions& _options)
{
  return _options.looseContentDirectory.value_or(ExecutableFolder() / CONTENT_FOLDER);
}

} // namespace Neuron
