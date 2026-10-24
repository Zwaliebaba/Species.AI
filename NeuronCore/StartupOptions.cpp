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

constexpr std::wstring_view USER_DATA_OPTION = L"--user-data";
constexpr std::wstring_view ALWAYS_ON_WORLD_OPTION = L"--always-on-world"; // ADR-190
#if ENGINE_DEVELOPMENT
constexpr std::wstring_view LOOSE_CONTENT_OPTION = L"--loose-content";
constexpr std::wstring_view SEED_OPTION = L"--seed";
constexpr std::wstring_view RENDER_CHECK_OPTION = L"--render-check";
constexpr std::wstring_view WATCH_OPTION = L"--watch";
constexpr std::wstring_view WORLD_OPTION = L"--world";
constexpr std::wstring_view SANDBOX_OPTION = L"--sandbox"; // ADR-155
constexpr std::wstring_view UI_TEST_OPTION = L"--ui-test";
constexpr std::wstring_view GPU_VALIDATION_OPTION = L"--gpu-validation"; // ADR-174
constexpr std::wstring_view LINK_PORT_OPTION = L"--link-port";           // ADR-192

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
    if (_arguments[i] == USER_DATA_OPTION && i + 1 < _arguments.size())
    {
      options.userDataDirectory = std::filesystem::path(_arguments[i + 1]);
      ++i;
      continue;
    }
    if (_arguments[i] == ALWAYS_ON_WORLD_OPTION && i + 1 < _arguments.size())
    {
      options.alwaysOnWorld = std::filesystem::path(_arguments[i + 1]);
      ++i;
      continue;
    }
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
    if (_arguments[i] == WATCH_OPTION)
    {
      options.watch = true;
      continue;
    }
    if (_arguments[i] == WORLD_OPTION)
    {
      options.world = true;
      continue;
    }
    if (_arguments[i] == SANDBOX_OPTION)
    {
      options.sandbox = true;
      continue;
    }
    if (_arguments[i] == UI_TEST_OPTION)
    {
      options.uiTest = true;
      continue;
    }
    if (_arguments[i] == GPU_VALIDATION_OPTION)
    {
      options.gpuValidation = true;
      continue;
    }
    if (_arguments[i] == RENDER_CHECK_OPTION && i + 1 < _arguments.size())
    {
      const std::optional<std::uint64_t> frames = DecimalOf(_arguments[i + 1]);
      if (frames.has_value() && *frames >= 1 && *frames <= RENDER_CHECK_FRAMES_MAX)
      {
        options.renderCheckFrames = static_cast<std::uint32_t>(*frames);
        ++i;
        continue;
      }
    }
    if (_arguments[i] == LINK_PORT_OPTION && i + 1 < _arguments.size())
    {
      const std::optional<std::uint64_t> port = DecimalOf(_arguments[i + 1]);
      if (port.has_value() && *port >= 1 && *port <= std::numeric_limits<std::uint16_t>::max())
      {
        options.linkPort = static_cast<std::uint16_t>(*port);
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
