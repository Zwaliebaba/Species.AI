#include "pch.h"

#include "MessagePattern.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <format>
#include <utility>

namespace Neuron
{

namespace
{

// The deepest nesting of cases a message may hold; a string table's texts need two or three (ADR-146).
constexpr std::uint32_t MESSAGE_MAX_DEPTH = 16;

// CLDR's plural categories, the only keywords a plural's or a select-ordinal's case may name.
constexpr std::array<std::string_view, 6> PLURAL_CATEGORIES{"zero", "one", "two", "few", "many", "other"};

bool IsSpace(char _c) noexcept
{
  return _c == ' ' || _c == '\t' || _c == '\r' || _c == '\n';
}

bool IsNameStart(char _c) noexcept
{
  return (_c >= 'a' && _c <= 'z') || (_c >= 'A' && _c <= 'Z') || _c == '_';
}

bool IsNameChar(char _c) noexcept
{
  return IsNameStart(_c) || (_c >= '0' && _c <= '9');
}

bool IsSelectorChar(char _c) noexcept
{
  return IsNameChar(_c) || _c == '-';
}

class Parser
{
public:
  explicit Parser(std::string_view _text) noexcept
    : m_text(_text)
  {
  }

  // A message up to the end of the text, or, nested, up to the } that ends its case, which is left for the caller.
  std::expected<std::vector<MessagePart>, MessageError> Message(bool _pluralCase, bool _nested, std::uint32_t _depth)
  {
    std::vector<MessagePart> parts;
    std::string literal;
    const auto flush = [&]
    {
      if (!literal.empty())
      {
        parts.push_back(MessagePart{MessagePartKind::Text, std::exchange(literal, {}), false, 0.0, {}});
      }
    };
    while (m_at < m_text.size())
    {
      const char c = m_text[m_at];
      if (c == '\'')
      {
        const char next = m_at + 1 < m_text.size() ? m_text[m_at + 1] : '\0';
        if (next == '\'')
        {
          literal += '\'';
          m_at += 2;
        }
        else if (next == '{' || next == '}' || (_pluralCase && next == '#'))
        {
          if (auto quoted = Quoted(); !quoted)
          {
            return std::unexpected(std::move(quoted.error()));
          }
          else
          {
            literal += *quoted;
          }
        }
        else
        {
          literal += '\'';
          ++m_at;
        }
      }
      else if (c == '{')
      {
        flush();
        auto argument = Argument(_depth);
        if (!argument)
        {
          return std::unexpected(std::move(argument.error()));
        }
        parts.push_back(std::move(*argument));
      }
      else if (c == '}')
      {
        if (!_nested)
        {
          return Fail("a } that closes no argument");
        }
        flush();
        return parts;
      }
      else if (c == '#' && _pluralCase)
      {
        flush();
        parts.push_back(MessagePart{MessagePartKind::Pound, {}, false, 0.0, {}});
        ++m_at;
      }
      else
      {
        literal += c;
        ++m_at;
      }
    }
    if (_nested)
    {
      return Fail("a case's message that no } ends");
    }
    flush();
    return parts;
  }

  [[nodiscard]] bool AtEnd() const noexcept
  {
    return m_at == m_text.size();
  }

private:
  std::unexpected<MessageError> Fail(std::string _what) const
  {
    return std::unexpected(MessageError{m_at, std::move(_what)});
  }

  void SkipSpaces() noexcept
  {
    while (m_at < m_text.size() && IsSpace(m_text[m_at]))
    {
      ++m_at;
    }
  }

  bool Take(char _c) noexcept
  {
    if (m_at < m_text.size() && m_text[m_at] == _c)
    {
      ++m_at;
      return true;
    }
    return false;
  }

  std::string_view Word(bool (*_accepts)(char) noexcept) noexcept
  {
    const std::size_t start = m_at;
    while (m_at < m_text.size() && _accepts(m_text[m_at]))
    {
      ++m_at;
    }
    return m_text.substr(start, m_at - start);
  }

  // Quoted text from the apostrophe at the cursor to the next single one; '' inside it is an apostrophe.
  std::expected<std::string, MessageError> Quoted()
  {
    const std::size_t start = m_at;
    ++m_at;
    std::string text;
    while (m_at < m_text.size())
    {
      if (m_text[m_at] == '\'')
      {
        if (m_at + 1 < m_text.size() && m_text[m_at + 1] == '\'')
        {
          text += '\'';
          m_at += 2;
          continue;
        }
        ++m_at;
        return text;
      }
      text += m_text[m_at];
      ++m_at;
    }
    m_at = start;
    return Fail("a quotation that no apostrophe ends");
  }

  // A number of an offset or an exact selector: an optional minus, digits, and an optional fraction.
  std::expected<double, MessageError> Number()
  {
    const std::size_t start = m_at;
    Take('-');
    if (Word([](char _c) noexcept { return _c >= '0' && _c <= '9'; }).empty())
    {
      m_at = start;
      return Fail("a number is expected");
    }
    if (Take('.') && Word([](char _c) noexcept { return _c >= '0' && _c <= '9'; }).empty())
    {
      m_at = start;
      return Fail("a number's fraction has no digits");
    }
    double value = 0.0;
    const std::string_view token = m_text.substr(start, m_at - start);
    if (std::from_chars(token.data(), token.data() + token.size(), value).ec != std::errc{})
    {
      m_at = start;
      return Fail("a number out of range");
    }
    return value;
  }

  // The argument whose { is at the cursor, up to and with its }.
  std::expected<MessagePart, MessageError> Argument(std::uint32_t _depth)
  {
    ++m_at;
    SkipSpaces();
    MessagePart part;
    if (m_at < m_text.size() && m_text[m_at] >= '0' && m_text[m_at] <= '9')
    {
      return Fail("a numbered argument: a string's arguments are named (UI-227)");
    }
    part.text = std::string(Word(&IsNameChar));
    if (part.text.empty() || !IsNameStart(part.text.front()))
    {
      return Fail("an argument without a name");
    }
    SkipSpaces();
    if (Take('}'))
    {
      part.kind = MessagePartKind::Argument;
      return part;
    }
    if (!Take(','))
    {
      return Fail("an argument's name is followed by neither , nor }");
    }
    SkipSpaces();
    const std::size_t typeAt = m_at;
    const std::string_view type = Word(&IsNameChar);
    SkipSpaces();
    if (type == "number")
    {
      part.kind = MessagePartKind::Number;
      if (Take(','))
      {
        SkipSpaces();
        const std::size_t styleAt = m_at;
        const std::string_view style = Word(&IsNameChar);
        if (style != "integer")
        {
          m_at = styleAt;
          return Fail(std::format("the number style '{}' is not one the string tables use (ADR-146)", style));
        }
        part.integer = true;
        SkipSpaces();
      }
      if (!Take('}'))
      {
        return Fail("a number argument that no } ends");
      }
      return part;
    }
    if (type != "plural" && type != "selectordinal" && type != "select")
    {
      m_at = typeAt;
      return Fail(std::format("the argument type '{}' is not one the string tables use (ADR-146)", type));
    }
    part.kind = type == "plural"          ? MessagePartKind::Plural
                : type == "selectordinal" ? MessagePartKind::SelectOrdinal
                                          : MessagePartKind::Select;
    if (!Take(','))
    {
      return Fail(std::format("a {} argument without its cases", type));
    }
    if (_depth >= MESSAGE_MAX_DEPTH)
    {
      return Fail("cases nested too deeply");
    }
    if (auto cases = Cases(part, _depth); !cases)
    {
      return std::unexpected(std::move(cases.error()));
    }
    return part;
  }

  // The cases of a plural, select-ordinal or select argument, after its type's comma, up to and with its }.
  std::expected<void, MessageError> Cases(MessagePart& _part, std::uint32_t _depth)
  {
    const bool categories = _part.kind != MessagePartKind::Select;
    SkipSpaces();
    if (_part.kind == MessagePartKind::Plural && m_text.substr(m_at).starts_with("offset:"))
    {
      m_at += 7;
      SkipSpaces();
      auto offset = Number();
      if (!offset)
      {
        return std::unexpected(std::move(offset.error()));
      }
      _part.offset = *offset;
    }
    for (;;)
    {
      SkipSpaces();
      if (Take('}'))
      {
        break;
      }
      const std::size_t selectorAt = m_at;
      MessageCase entry;
      if (categories && Take('='))
      {
        auto exact = Number();
        if (!exact)
        {
          return std::unexpected(std::move(exact.error()));
        }
        entry.selector = std::string(m_text.substr(selectorAt, m_at - selectorAt));
      }
      else
      {
        entry.selector = std::string(Word(&IsSelectorChar));
        if (entry.selector.empty())
        {
          return Fail(m_at < m_text.size() ? "a case without its selector" : "an argument that no } ends");
        }
        if (categories && std::ranges::find(PLURAL_CATEGORIES, entry.selector) == PLURAL_CATEGORIES.end())
        {
          m_at = selectorAt;
          return Fail(std::format("'{}' is no plural category (zero, one, two, few, many or other) and no =number", entry.selector));
        }
      }
      if (std::ranges::any_of(_part.cases, [&](const MessageCase& _other) { return _other.selector == entry.selector; }))
      {
        m_at = selectorAt;
        return Fail(std::format("the case '{}' is given twice", entry.selector));
      }
      SkipSpaces();
      if (!Take('{'))
      {
        return Fail(std::format("the case '{}' has no {{message}}", entry.selector));
      }
      auto message = Message(categories, true, _depth + 1);
      if (!message)
      {
        return std::unexpected(std::move(message.error()));
      }
      entry.parts = std::move(*message);
      Take('}');
      _part.cases.push_back(std::move(entry));
    }
    if (std::ranges::none_of(_part.cases, [](const MessageCase& _case) { return _case.selector == "other"; }))
    {
      return Fail("an argument's cases lack other");
    }
    return {};
  }

  std::string_view m_text;
  std::size_t m_at = 0;
};

void CollectNames(const std::vector<MessagePart>& _parts, std::vector<std::string>& _names)
{
  for (const MessagePart& part : _parts)
  {
    if (part.kind == MessagePartKind::Text || part.kind == MessagePartKind::Pound)
    {
      continue;
    }
    if (std::ranges::find(_names, part.text) == _names.end())
    {
      _names.push_back(part.text);
    }
    for (const MessageCase& entry : part.cases)
    {
      CollectNames(entry.parts, _names);
    }
  }
}

} // namespace

std::expected<MessagePattern, MessageError> ParseMessage(std::string_view _text)
{
  Parser parser(_text);
  auto parts = parser.Message(false, false, 0);
  if (!parts)
  {
    return std::unexpected(std::move(parts.error()));
  }
  return MessagePattern{std::move(*parts)};
}

std::vector<std::string> MessageArgumentNames(const MessagePattern& _pattern)
{
  std::vector<std::string> names;
  CollectNames(_pattern.parts, names);
  return names;
}

} // namespace Neuron
