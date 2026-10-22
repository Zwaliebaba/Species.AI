#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace Neuron
{

// What a part of a message is (ADR-146): the subset of ICU MessageFormat that a string table's texts may use (ADR-063,
// UI-227).
enum class MessagePartKind : std::uint8_t
{
  Text,          // literal text, its quoting already removed
  Argument,      // {name}: the argument's value as text, a number formatted for the language
  Number,        // {name, number} or {name, number, integer}
  Plural,        // {name, plural, [offset:n] =n {...} keyword {...} ... other {...}}
  SelectOrdinal, // {name, selectordinal, ...}, the ordinal rules' categories
  Select,        // {name, select, key {...} ... other {...}}
  Pound          // # in a plural's or a select-ordinal's case: its number less the offset, formatted
};

struct MessagePart;

/// One case of a plural, select-ordinal or select argument: its selector, either `=n` for an exact number or a keyword,
/// and its message.
struct MessageCase
{
  std::string selector;
  std::vector<MessagePart> parts;
};

/// One part of a parsed message.
struct MessagePart
{
  MessagePartKind kind = MessagePartKind::Text;
  std::string text;     // Text: the literal text; every other kind but Pound: the argument's name
  bool integer = false; // Number: the integer style
  double offset = 0.0;  // Plural and SelectOrdinal: the offset subtracted before the categories and #
  std::vector<MessageCase> cases;
};

/// A parsed message: its parts in order.
struct MessagePattern
{
  std::vector<MessagePart> parts;
};

/// Why a text is not a message of the subset: the byte offset where the parse stopped and what is wrong there.
struct MessageError
{
  std::size_t offset = 0;
  std::string what;
};

// UI-227 and ADR-146: a text in ICU MessageFormat's syntax, apostrophe quoting as ICU's default has it ('' is an
// apostrophe, and an apostrophe before {, } or, in a plural's case, # starts quoted text up to the next one). An
// argument has a name of ASCII letters, digits and underscores that starts with a letter or an underscore, never a
// number, and is plain, number (with no style or the integer style), plural, selectordinal or select; a plural's or a
// select's cases end with other. Any other form fails, naming where.
[[nodiscard]] std::expected<MessagePattern, MessageError> ParseMessage(std::string_view _text);

// The names of the arguments a parsed message reads, each once, in the order they first appear.
[[nodiscard]] std::vector<std::string> MessageArgumentNames(const MessagePattern& _pattern);

} // namespace Neuron
