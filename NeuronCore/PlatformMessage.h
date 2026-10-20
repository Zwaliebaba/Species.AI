#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace Neuron
{

enum class MessageSeverity : std::uint8_t
{
  Notice,
  Error,
  Fatal
};

struct MessageArgument
{
  std::string_view name;
  std::string value;
};

// The base-language text of a string id, compiled into the executable for the messages that can appear before the
// string table is loaded (PLT-039, table 6.4-A). The game supplies the texts, so the engine names no game text
// (AGENTS.md R9).
struct CompiledText
{
  std::string_view stringId;
  std::string_view text;
};

// What a pre-UI message takes from the composition root: the dialog title, the run id that ui.error.internal shows
// (PLT-127) and the compiled-in texts. The root sets it at start-up before anything can show a message, keeps it alive
// and clears it with nullptr at shutdown.
struct PlatformMessageContext
{
  std::string_view title;
  std::uint64_t runId = 0;
  std::span<const CompiledText> texts;
};

void SetPlatformMessageContext(const PlatformMessageContext* _context) noexcept;
// The run id of the context, 0 while none is set.
[[nodiscard]] std::uint64_t PlatformMessageRunId() noexcept;

// A message's text: the compiled-in text of the id with every {name} replaced by the argument of that name, or the id
// and its arguments when the id has no compiled-in text. Named arguments are the part of ICU MessageFormat that the
// compiled-in texts use (DAT-208); the string table's texts replace them once the language is loaded (§4.6).
[[nodiscard]] std::string ComposePlatformMessage(std::span<const CompiledText> _texts, std::string_view _stringId,
                                                 std::span<const MessageArgument> _arguments);

// A pre-UI message (PLT-039): written to the log through the log sink, then shown as a modal task dialog owned by the
// window when one exists. A run without an interactive desktop, such as a test or a service, writes the log only.
void ShowPlatformMessage(std::string_view _stringId, std::span<const MessageArgument> _arguments, MessageSeverity _severity);

// The platform's part of a fatal exit (PLT-044, DBG-074): the message, then the end of the process with
// plt.lifecycle.exit_code.fatal and no crash report.
[[noreturn]] void EndWithMessage(std::string_view _stringId, std::span<const MessageArgument> _arguments);

inline constexpr int EXIT_CODE_NORMAL = 0; // plt.lifecycle.exit_code.normal
inline constexpr int EXIT_CODE_FATAL = -1; // plt.lifecycle.exit_code.fatal

} // namespace Neuron
