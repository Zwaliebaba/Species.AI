#include "pch.h"

#include "AuthoringParser.h"
#include "MessagePattern.h"
#include "StringTableAsset.h"
#include "StringTableCooker.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <expected>
#include <format>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace ContentBuild
{

namespace
{

constexpr std::string_view SCHEMA_ID = "text.string_table";
constexpr std::string_view SCHEMA_VERSION = "1.0";
// ui.lang.max_id_length, ui.lang.max_marker_length and ui.lang.max_expanded_length, in characters (UI-225, DAT-212).
constexpr std::size_t MAX_ID_CHARACTERS = 96;
constexpr std::size_t MAX_MARKER_CHARACTERS = 64;
constexpr std::size_t MAX_EXPANDED_CHARACTERS = 1024;

// The input modes of table 3.5-B, in the order the cooked entry holds their texts.
enum class TextMode : std::uint8_t
{
  KeyboardMouse,
  Gamepad
};

constexpr std::array<TextMode, 2> MODES{TextMode::KeyboardMouse, TextMode::Gamepad};

std::string_view NameOf(TextMode _mode) noexcept
{
  return _mode == TextMode::KeyboardMouse ? "keyboard-and-mouse" : "gamepad";
}

/// One entry as authored (DAT-208): its plain text and its variants, each when given, and its embed-only flag.
struct Authored
{
  std::optional<std::string> text;
  std::optional<std::string> keyboardMouse;
  std::optional<std::string> gamepad;
  bool embedOnly = false;
};

// The code points of UTF-8 text that the authoring parser has already found valid.
std::vector<std::uint32_t> CodePoints(std::string_view _utf8)
{
  std::vector<std::uint32_t> points;
  for (std::size_t at = 0; at < _utf8.size();)
  {
    const auto lead = static_cast<std::uint8_t>(_utf8[at]);
    const std::size_t length = lead < 0x80 ? 1 : lead < 0xE0 ? 2 : lead < 0xF0 ? 3 : 4;
    const std::uint32_t bits = lead;
    std::uint32_t point = length == 1 ? bits : length == 2 ? (bits & 0x1Fu) : length == 3 ? (bits & 0x0Fu) : (bits & 0x07u);
    for (std::size_t n = 1; n < length && at + n < _utf8.size(); ++n)
    {
      point = (point << 6) | (static_cast<std::uint8_t>(_utf8[at + n]) & 0x3Fu);
    }
    points.push_back(point);
    at += length;
  }
  return points;
}

std::size_t CharacterCount(std::string_view _utf8)
{
  return static_cast<std::size_t>(std::ranges::count_if(_utf8, [](char _c) { return (static_cast<std::uint8_t>(_c) & 0xC0u) != 0x80u; }));
}

// UI-233 for the en-US table: the upper-cased form of Latin-1 text as ICU's en-US mapping gives it. A character outside
// Latin-1 is kept as it is, so its upper-cased form is checked only when a later language needs it (ADR-147).
std::vector<std::uint32_t> UpperCased(const std::vector<std::uint32_t>& _points)
{
  std::vector<std::uint32_t> upper;
  upper.reserve(_points.size());
  for (const std::uint32_t point : _points)
  {
    // a to z, and à to þ but ÷, lie 0x20 above their capitals
    if ((point >= 'a' && point <= 'z') || (point >= 0xE0 && point <= 0xFE && point != 0xF7))
    {
      upper.push_back(point - 0x20);
    }
    else if (point == 0xDF) // ß
    {
      upper.insert(upper.end(), {'S', 'S'});
    }
    else if (point == 0xFF) // ÿ
    {
      upper.push_back(0x178);
    }
    else if (point == 0xB5) // µ
    {
      upper.push_back(0x39C);
    }
    else
    {
      upper.push_back(point);
    }
  }
  return upper;
}

// DAT-206: the BCP 47 tag that a table's file name gives, en_us.json giving en-US.
std::string LanguageOfFile(std::string_view _file)
{
  std::string_view stem = _file.substr(_file.find_last_of('/') + 1);
  stem = stem.substr(0, stem.find('.'));
  std::string tag;
  bool region = false;
  for (const char c : stem)
  {
    if (c == '_')
    {
      tag += '-';
      region = true;
    }
    else
    {
      tag += region && c >= 'a' && c <= 'z' ? static_cast<char>(c - 'a' + 'A') : c;
    }
  }
  return tag;
}

// DAT-207 and CONVENTIONS §8: lower-case and dotted, two segments at least, each of a-z, 0-9 and _.
bool IsStringId(std::string_view _id) noexcept
{
  if (_id.empty() || _id.front() == '.' || _id.back() == '.' || _id.find("..") != std::string_view::npos ||
      _id.find('.') == std::string_view::npos)
  {
    return false;
  }
  return std::ranges::all_of(_id, [](char _c) { return (_c >= 'a' && _c <= 'z') || (_c >= '0' && _c <= '9') || _c == '_' || _c == '.'; });
}

// Table 3.5-B: the text an entry holds in a mode before its markers are expanded.
std::string RawText(const Authored& _entry, TextMode _mode)
{
  const std::optional<std::string>& variant = _mode == TextMode::KeyboardMouse ? _entry.keyboardMouse : _entry.gamepad;
  if (variant.has_value())
  {
    return *variant;
  }
  return _entry.text.value_or(std::string());
}

// DAT-213: an embedded string quoted for ICU MessageFormat, so that none of its characters is read as message syntax.
std::string QuotedForMessage(std::string_view _text)
{
  std::string quoted;
  for (const char c : _text)
  {
    if (c == '\'')
    {
      quoted += "''";
    }
    else if (c == '{' || c == '}')
    {
      quoted += '\'';
      quoted += c;
      quoted += '\'';
    }
    else
    {
      quoted += c;
    }
  }
  return quoted;
}

/// DAT-211 to DAT-213: expands one entry's markers in one input mode, embedded strings recursively; the first fault
/// found ends the expansion.
class Expander
{
public:
  explicit Expander(const std::map<std::string, Authored, std::less<>>& _entries) noexcept
    : m_entries(_entries)
  {
  }

  std::expected<std::string, std::string> Expand(std::string_view _id, TextMode _mode)
  {
    if (std::ranges::find(m_stack, _id) != m_stack.end())
    {
      std::string chain;
      for (const std::string& id : m_stack)
      {
        chain += id + " -> ";
      }
      return std::unexpected(std::format("a cycle of embedded strings: {}{} (DAT-211)", chain, _id));
    }
    const auto found = m_entries.find(_id);
    if (found == m_entries.end())
    {
      return std::unexpected(std::format("it embeds {}, which the table does not hold (DAT-211)", _id));
    }
    m_stack.emplace_back(_id);
    auto expanded = Text(RawText(found->second, _mode), _mode);
    m_stack.pop_back();
    return expanded;
  }

private:
  std::expected<std::string, std::string> Text(std::string_view _raw, TextMode _mode)
  {
    std::string out;
    std::optional<TextMode> passage; // the mode of the passage being read, if any
    bool alternated = false;         // the passage has switched to its alternative
    std::size_t at = 0;
    while (at < _raw.size())
    {
      const std::size_t open = _raw.find("[[", at);
      const std::string_view before = _raw.substr(at, open == std::string_view::npos ? std::string_view::npos : open - at);
      if (!passage.has_value() || *passage == _mode)
      {
        out += before;
      }
      if (open == std::string_view::npos)
      {
        break;
      }
      const std::size_t close = _raw.find("]]", open + 2);
      if (close == std::string_view::npos)
      {
        return std::unexpected(std::format("an unterminated marker at character {} (DAT-212)", CharacterCount(_raw.substr(0, open)) + 1));
      }
      const std::string_view marker = _raw.substr(open + 2, close - open - 2);
      if (CharacterCount(marker) + 4 > MAX_MARKER_CHARACTERS)
      {
        return std::unexpected(std::format("a marker of more than {} characters (UI-225)", MAX_MARKER_CHARACTERS));
      }
      at = close + 2;
      if (marker == "kbm" || marker == "pad")
      {
        const TextMode mode = marker == "kbm" ? TextMode::KeyboardMouse : TextMode::Gamepad;
        if (!passage.has_value())
        {
          passage = mode;
          alternated = false;
        }
        else if (*passage != mode && !alternated)
        {
          passage = mode;
          alternated = true;
        }
        else
        {
          return std::unexpected(std::format("a [[{}]] passage inside a mode passage (DAT-211)", marker));
        }
        continue;
      }
      if (marker == "end")
      {
        if (!passage.has_value())
        {
          return std::unexpected(std::string("an [[end]] without a mode passage (DAT-212)"));
        }
        passage.reset();
        continue;
      }
      if (passage.has_value() && *passage != _mode)
      {
        continue; // a marker inside the other mode's passage expands to nothing
      }
      if (marker == "open")
      {
        out += "[[";
      }
      else if (marker.starts_with("string:"))
      {
        auto embedded = Expand(marker.substr(7), _mode);
        if (!embedded)
        {
          return embedded;
        }
        if (embedded->find('#') != std::string::npos)
        {
          return std::unexpected(
            std::format("it embeds {}, whose # cannot be quoted for every place it may land (ADR-146)", marker.substr(7)));
        }
        out += QuotedForMessage(*embedded);
      }
      else if (marker.starts_with("binding:"))
      {
        return std::unexpected(std::format(
          "the binding argument [[{}]]: binding nouns arrive with the settings store's rebinding (M2 task T15, ADR-146)", marker));
      }
      else
      {
        return std::unexpected(std::format("the unknown marker [[{}]] (DAT-212)", marker));
      }
    }
    if (passage.has_value())
    {
      return std::unexpected(std::string("a mode passage without [[end]] (DAT-212)"));
    }
    return out;
  }

  const std::map<std::string, Authored, std::less<>>& m_entries;
  std::vector<std::string> m_stack;
};

const Neuron::JsonValue* Member(const Neuron::JsonValue& _object, std::string_view _key) noexcept
{
  const auto found = std::ranges::find(_object.members, _key, &Neuron::JsonMember::key);
  return found == _object.members.end() ? nullptr : &found->value;
}

} // namespace

std::optional<Neuron::CookedEntry> CookStringTable(const ManifestRow& _row, std::string_view _file, std::span<const std::byte> _bytes,
                                                   const FontCoverage& _coverage, BuildReport& _report)
{
  const std::size_t findings = _report.Findings().size();
  const auto fail = [&](std::string_view _message) { _report.Fail(_file, _message); };
  const auto document = Neuron::AuthoringParser{}.Parse(_bytes, _file);
  if (!document)
  {
    _report.Fail(document.error());
    return std::nullopt;
  }
  const Neuron::JsonValue& root = document->root;
  if (root.kind != Neuron::JsonKind::Object)
  {
    fail("a string table is a JSON object (DAT-206)");
    return std::nullopt;
  }
  // DAT-206 and DAT-026: the four fields, and nothing undeclared.
  for (const Neuron::JsonMember& member : root.members)
  {
    if (member.key != "schema" && member.key != "version" && member.key != "language" && member.key != "strings")
    {
      fail(std::format("{}:{}: the field {} is no field of {} (DAT-026)", member.line, member.column, member.key, SCHEMA_ID));
    }
  }
  const Neuron::JsonValue* schema = Member(root, "schema");
  const Neuron::JsonValue* version = Member(root, "version");
  const Neuron::JsonValue* language = Member(root, "language");
  const Neuron::JsonValue* strings = Member(root, "strings");
  if (schema == nullptr || schema->kind != Neuron::JsonKind::String || schema->text != SCHEMA_ID)
  {
    fail(std::format("its schema field is not {} (DAT-016)", SCHEMA_ID));
  }
  if (version == nullptr || version->kind != Neuron::JsonKind::String || version->text != SCHEMA_VERSION)
  {
    fail(std::format("its version field is not {}, the version this build reads (DAT-017)", SCHEMA_VERSION));
  }
  const std::string tag = LanguageOfFile(_row.speciesFilename);
  if (language == nullptr || language->kind != Neuron::JsonKind::String || language->text != tag)
  {
    fail(std::format("its language field is not {}, the tag its file name gives (DAT-206)", tag));
  }
  if (strings == nullptr || strings->kind != Neuron::JsonKind::Object)
  {
    fail("its strings field is not an object keyed by string id (DAT-206)");
    return std::nullopt;
  }

  // DAT-207 to DAT-209: the ids and the entries. The parser has already refused an id given twice.
  std::map<std::string, Authored, std::less<>> entries;
  for (const Neuron::JsonMember& member : strings->members)
  {
    const auto at = [&](std::string_view _what) { fail(std::format("{}:{}: {}: {}", member.line, member.column, member.key, _what)); };
    if (!IsStringId(member.key))
    {
      at("not a string id: lower-case, dotted, of a-z, 0-9 and _ (DAT-207)");
      continue;
    }
    if (CharacterCount(member.key) > MAX_ID_CHARACTERS)
    {
      at(std::format("an id of more than {} characters (UI-225)", MAX_ID_CHARACTERS));
      continue;
    }
    Authored entry;
    const Neuron::JsonValue& value = member.value;
    if (value.kind == Neuron::JsonKind::String)
    {
      entry.text = value.text;
    }
    else if (value.kind == Neuron::JsonKind::Object)
    {
      bool valid = true;
      for (const Neuron::JsonMember& field : value.members)
      {
        const bool text = field.key == "text" || field.key == "kbm" || field.key == "pad";
        if (text && field.value.kind == Neuron::JsonKind::String)
        {
          (field.key == "text" ? entry.text : field.key == "kbm" ? entry.keyboardMouse : entry.gamepad) = field.value.text;
        }
        else if (field.key == "embed_only" && field.value.kind == Neuron::JsonKind::Boolean)
        {
          entry.embedOnly = field.value.boolean;
        }
        else
        {
          at(std::format("the field {} is not one of table 3.5-A, of its type (DAT-208)", field.key));
          valid = false;
        }
      }
      if (!valid)
      {
        continue;
      }
      if (!entry.text && !entry.keyboardMouse && !entry.gamepad)
      {
        at("an entry holds at least one of text, kbm and pad (DAT-209)");
        continue;
      }
    }
    else
    {
      at("an entry is a text or an object of table 3.5-A (DAT-208)");
      continue;
    }
    entries.emplace(member.key, std::move(entry));
  }

  // DAT-211 to DAT-213, UI-225, ADR-063 and UI-199: each entry's text in each mode.
  Neuron::StringTableAsset table;
  table.language = tag;
  Expander expander(entries);
  for (const auto& [id, authored] : entries)
  {
    Neuron::StringEntry cooked;
    cooked.id = id;
    cooked.embedOnly = authored.embedOnly;
    bool valid = true;
    std::set<std::uint32_t> missing;
    for (const TextMode mode : MODES)
    {
      auto expanded = expander.Expand(id, mode);
      if (!expanded)
      {
        fail(std::format("{}: {}", id, expanded.error()));
        valid = false;
        break;
      }
      if (CharacterCount(*expanded) > MAX_EXPANDED_CHARACTERS)
      {
        fail(std::format("{}: its {} text expands to more than {} characters (UI-225)", id, NameOf(mode), MAX_EXPANDED_CHARACTERS));
        valid = false;
      }
      if (const auto pattern = Neuron::ParseMessage(*expanded); !pattern)
      {
        fail(std::format("{}: its {} text is not a message: {}, at byte {} of its expanded text (ADR-063)", id, NameOf(mode),
                         pattern.error().what, pattern.error().offset));
        valid = false;
      }
      const std::vector<std::uint32_t> points = CodePoints(*expanded);
      for (const std::vector<std::uint32_t>& form : {points, UpperCased(points)})
      {
        for (const std::uint32_t point : form)
        {
          if (!_coverage.Covers(point))
          {
            missing.insert(point);
          }
        }
      }
      (mode == TextMode::KeyboardMouse ? cooked.keyboardMouse : cooked.gamepad) = std::move(*expanded);
    }
    for (const std::uint32_t point : missing)
    {
      fail(std::format("{}: U+{:04X} is in its text or its upper-cased form, and no system font of {}'s font roles draws it "
                       "(UI-199, UI-233)",
                       id, point, tag));
      valid = false;
    }
    if (valid)
    {
      table.entries.push_back(std::move(cooked));
    }
  }
  if (_report.Findings().size() != findings)
  {
    return std::nullopt;
  }
  Neuron::CookedEntry entry;
  entry.entry.type = Neuron::ContentType::StringTable;
  entry.bytes = Neuron::WriteStringTableAsset(table);
  // AST-091: the entry reads back through the runtime's loader as what the cooker wrote.
  const auto readBack = Neuron::ReadStringTableAsset(entry.bytes);
  if (!readBack || Neuron::WriteStringTableAsset(*readBack) != entry.bytes)
  {
    fail(std::format("the cooked string table does not read back as written: {} (AST-091)", readBack ? "it differs" : readBack.error()));
    return std::nullopt;
  }
  return entry;
}

} // namespace ContentBuild
