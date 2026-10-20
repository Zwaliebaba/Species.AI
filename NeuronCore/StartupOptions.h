#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Neuron
{

// The loose-content folder beside the executable, where the solution build's content build writes its output
// (ADR-108, owner decision O73).
inline constexpr std::wstring_view CONTENT_FOLDER = L"content";
// The most world frames a render check draws (ADR-113).
inline constexpr std::uint32_t RENDER_CHECK_FRAMES_MAX = 10'000;

/// The start-up options of PLT table 3.3-A that exist so far. `--loose-content <directory>` names the loose-content
/// folder to mount (PLT-026, AST-105), `--seed <number>` a new world's seed (MVP-005, ADR-109), and `--render-check
/// <frames>` the number of world frames a render check draws (ADR-113); all are compiled into Debug builds only, so a
/// Release build ignores them as unknown arguments (PLT-107). The other options of the table arrive with the services
/// they configure.
struct StartupOptions
{
  std::optional<std::filesystem::path> looseContentDirectory;
  std::optional<std::uint64_t> worldSeed;         // a decimal number of at most 64 bits
  std::optional<std::uint32_t> renderCheckFrames; // a decimal number from 1 to RENDER_CHECK_FRAMES_MAX
  std::vector<std::string> ignoredArguments;      // PLT-022: logged and ignored, as UTF-8
};

// PLT-022: the options of the arguments after the program's name. An option is a whole argument, in any position; one
// that takes a value takes the next argument, and one whose value is not valid is ignored with it; anything else is
// ignored.
[[nodiscard]] StartupOptions ParseArguments(std::span<const std::wstring> _arguments);
// The options of this process's command line, split by the Windows rules (CommandLineToArgvW).
[[nodiscard]] StartupOptions ParseCommandLine();

// The loose-content folder to mount: the option's, or CONTENT_FOLDER beside the executable.
[[nodiscard]] std::filesystem::path ContentDirectory(const StartupOptions& _options);

} // namespace Neuron
