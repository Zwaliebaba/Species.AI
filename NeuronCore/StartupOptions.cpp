#include "pch.h"

#include <shellapi.h>

#include "Development.h"
#include "Files.h"
#include "StartupOptions.h"
#include "TextEncoding.h"

#pragma comment(lib, "shell32.lib")

namespace Neuron
{

namespace
{

#if ENGINE_DEVELOPMENT
constexpr std::wstring_view LOOSE_CONTENT_OPTION = L"--loose-content";
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
      LocalFree(values);
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
