#pragma once

#include "InputRouter.h"
#include "LooseContent.h"
#include "MessageFormat.h"
#include "MessageLocale.h"
#include "MessagePattern.h"
#include "StringTableAsset.h"

#include <memory>
#include <mutex>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Neuron
{

/// One language's strings for the UI (UI-218 to UI-221, ADR-146): the cooked table, each text's message parsed once. A
/// table never changes once made, so any thread may look up at once; the misses it logs are its one shared state, held
/// behind a lock.
class StringTable
{
public:
  // A table with no strings, in which every look-up misses.
  StringTable() = default;
  explicit StringTable(StringTableAsset _asset);

  StringTable(const StringTable&) = delete;
  StringTable& operator=(const StringTable&) = delete;

  [[nodiscard]] const std::string& Language() const noexcept
  {
    return m_asset.language;
  }

  // UI-218: the text of _id in _mode (UI-223), valid while the table is held (UI-221). An id the table lacks, or whose
  // entry is embed-only (DAT-211), is a miss, logged once per id (UI-219): a Debug build returns the id itself, so a
  // string not yet written shows where it belongs (the M2 plan's T14), and a Release build the empty text.
  [[nodiscard]] std::string_view Lookup(std::string_view _id, InputMode _mode = InputMode::KeyboardMouse) const;
  // UI-220: whether the table holds _id as a text a look-up returns.
  [[nodiscard]] bool Exists(std::string_view _id) const noexcept;
  // UI-227: the text of _id in _mode with _arguments inserted, in _locale's forms (ADR-063). A miss gives the look-up's
  // text, unformatted.
  [[nodiscard]] std::string Format(std::string_view _id, std::span<const FormatArgument> _arguments, const MessageLocale& _locale,
                                   InputMode _mode = InputMode::KeyboardMouse) const;

private:
  struct Messages
  {
    MessagePattern keyboardMouse;
    MessagePattern gamepad;
  };

  // The index of _id's entry, or the entries' count when a look-up misses.
  [[nodiscard]] std::size_t IndexOf(std::string_view _id) const noexcept;
  [[nodiscard]] std::string_view Miss(std::string_view _id) const;

  StringTableAsset m_asset;
  std::vector<Messages> m_messages; // by entry index
  mutable std::mutex m_missLock;
  mutable std::set<std::string, std::less<>> m_missed;
};

// The content key of a language's table: strings/en_us for en-US (DAT-206, AST-014).
[[nodiscard]] std::string StringTableKey(std::string_view _languageTag);
// The table of _languageTag from the content, or an empty one, every look-up a miss, when the content lacks it or it is
// damaged, which is logged.
[[nodiscard]] std::shared_ptr<const StringTable> LoadStringTable(const LooseContent& _content, std::string_view _languageTag);

} // namespace Neuron
