#include "pch.h"

#include "AuthoringParser.h"

#include <charconv>
#include <format>
#include <map>
#include <optional>
#include <system_error>
#include <utility>

namespace Neuron
{

namespace
{

constexpr std::string_view BYTE_ORDER_MARK = "\xEF\xBB\xBF";

// RFC 3629: the offset of the first byte that starts no well-formed sequence, or npos. No overlong form, no surrogate
// and nothing above U+10FFFF.
std::size_t FirstInvalidUtf8(std::string_view _text) noexcept
{
  const auto byteAt = [&_text](std::size_t _i) { return static_cast<unsigned char>(_text[_i]); };
  std::size_t i = 0;
  while (i < _text.size())
  {
    const unsigned char lead = byteAt(i);
    if (lead < 0x80)
    {
      ++i;
      continue;
    }
    std::size_t length = 0;
    unsigned char low = 0x80;
    unsigned char high = 0xBF;
    if (lead >= 0xC2 && lead <= 0xDF)
    {
      length = 2;
    }
    else if (lead >= 0xE0 && lead <= 0xEF)
    {
      length = 3;
      low = lead == 0xE0 ? 0xA0 : 0x80;
      high = lead == 0xED ? 0x9F : 0xBF;
    }
    else if (lead >= 0xF0 && lead <= 0xF4)
    {
      length = 4;
      low = lead == 0xF0 ? 0x90 : 0x80;
      high = lead == 0xF4 ? 0x8F : 0xBF;
    }
    else
    {
      return i;
    }
    if (i + length > _text.size())
    {
      return i;
    }
    for (std::size_t k = 1; k < length; ++k)
    {
      const unsigned char next = byteAt(i + k);
      const unsigned char minimum = k == 1 ? low : 0x80;
      const unsigned char maximum = k == 1 ? high : 0xBF;
      if (next < minimum || next > maximum)
      {
        return i;
      }
    }
    i += length;
  }
  return std::string_view::npos;
}

void AppendUtf8(std::string& _out, std::uint32_t _codePoint)
{
  const auto put = [&_out](std::uint32_t _byte) { _out.push_back(static_cast<char>(static_cast<unsigned char>(_byte))); };
  if (_codePoint < 0x80)
  {
    put(_codePoint);
  }
  else if (_codePoint < 0x800)
  {
    put(0xC0 | (_codePoint >> 6));
    put(0x80 | (_codePoint & 0x3F));
  }
  else if (_codePoint < 0x10000)
  {
    put(0xE0 | (_codePoint >> 12));
    put(0x80 | ((_codePoint >> 6) & 0x3F));
    put(0x80 | (_codePoint & 0x3F));
  }
  else
  {
    put(0xF0 | (_codePoint >> 18));
    put(0x80 | ((_codePoint >> 12) & 0x3F));
    put(0x80 | ((_codePoint >> 6) & 0x3F));
    put(0x80 | (_codePoint & 0x3F));
  }
}

bool IsDigit(char _c) noexcept
{
  return _c >= '0' && _c <= '9';
}

/// One parse of one file. The first error stops it (DAT-021); recursion is bounded by AUTHORING_MAX_DEPTH.
class Parser
{
public:
  Parser(std::string_view _text, std::string_view _file)
    : m_text(_text),
      m_file(_file)
  {
  }

  std::expected<JsonValue, ContentDiagnostic> Run()
  {
    if (m_text.starts_with(BYTE_ORDER_MARK))
    {
      m_position = BYTE_ORDER_MARK.size();
    }
    if (const std::size_t invalid = FirstInvalidUtf8(m_text.substr(m_position)); invalid != std::string_view::npos)
    {
      Move(m_position + invalid);
      return std::unexpected(Error(DiagnosticClass::Encoding, "the text is not well-formed UTF-8"));
    }
    SkipWhitespace();
    if (AtEnd())
    {
      return std::unexpected(Error(DiagnosticClass::Syntax, "the file holds no value"));
    }
    JsonValue root;
    if (!ParseValue(root, 0))
    {
      return std::unexpected(m_error.value_or(Error(DiagnosticClass::Syntax, "the value does not parse")));
    }
    SkipWhitespace();
    if (!AtEnd())
    {
      return std::unexpected(Error(DiagnosticClass::Syntax, "a character after the top-level value"));
    }
    return root;
  }

private:
  [[nodiscard]] bool AtEnd() const noexcept
  {
    return m_position >= m_text.size();
  }
  [[nodiscard]] char Peek() const noexcept
  {
    return AtEnd() ? '\0' : m_text[m_position];
  }

  // Moves to an offset at or after the current one, counting lines and code points (DAT-020).
  void Move(std::size_t _to) noexcept
  {
    for (; m_position < _to && m_position < m_text.size(); ++m_position)
    {
      const auto byte = static_cast<unsigned char>(m_text[m_position]);
      if (byte == '\n')
      {
        ++m_line;
        m_column = 1;
      }
      else if ((byte & 0xC0) != 0x80)
      {
        ++m_column;
      }
    }
  }
  void Next() noexcept
  {
    Move(m_position + 1);
  }

  void SkipWhitespace() noexcept
  {
    while (!AtEnd() && (Peek() == ' ' || Peek() == '\t' || Peek() == '\n' || Peek() == '\r'))
    {
      Next();
    }
  }

  ContentDiagnostic Error(DiagnosticClass _class, std::string_view _message) const
  {
    return ErrorAt(_class, _message, m_line, m_column);
  }
  ContentDiagnostic ErrorAt(DiagnosticClass _class, std::string_view _message, std::uint32_t _line, std::uint32_t _column) const
  {
    ContentDiagnostic diagnostic;
    diagnostic.diagnosticClass = _class;
    diagnostic.at = SourceLocation{std::string(m_file), _line, _column, Path()};
    diagnostic.message = std::string(_message);
    return diagnostic;
  }
  bool Fail(DiagnosticClass _class, std::string_view _message)
  {
    m_error = Error(_class, _message);
    return false;
  }

  [[nodiscard]] std::string Path() const
  {
    std::string path;
    for (const std::string& part : m_path)
    {
      if (!path.empty() && !part.starts_with('['))
      {
        path += '.';
      }
      path += part;
    }
    return path;
  }

  bool ParseValue(JsonValue& _out, std::uint32_t _depth)
  {
    _out.line = m_line;
    _out.column = m_column;
    switch (Peek())
    {
    case '{':
      return ParseObject(_out, _depth + 1);
    case '[':
      return ParseArray(_out, _depth + 1);
    case '"':
      _out.kind = JsonKind::String;
      return ParseString(_out.text);
    case 't':
      return ParseLiteral(_out, "true", JsonKind::Boolean, true);
    case 'f':
      return ParseLiteral(_out, "false", JsonKind::Boolean, false);
    case 'n':
      return ParseLiteral(_out, "null", JsonKind::Null, false);
    default:
      if (Peek() == '-' || IsDigit(Peek()))
      {
        return ParseNumber(_out);
      }
      return Fail(DiagnosticClass::Syntax, AtEnd() ? "a value is missing at the end of the file" : "a character that starts no value");
    }
  }

  bool ParseLiteral(JsonValue& _out, std::string_view _word, JsonKind _kind, bool _boolean)
  {
    if (m_text.substr(m_position, _word.size()) != _word)
    {
      return Fail(DiagnosticClass::Syntax, "a character that starts no value");
    }
    Move(m_position + _word.size());
    _out.kind = _kind;
    _out.boolean = _boolean;
    return true;
  }

  bool ParseNumber(JsonValue& _out)
  {
    // RFC 8259 §6: -? (0 | [1-9][0-9]*) (. [0-9]+)? ([eE] [+-]? [0-9]+)?
    const std::size_t start = m_position;
    std::size_t end = start;
    const auto digits = [this, &end]
    {
      const std::size_t first = end;
      while (end < m_text.size() && IsDigit(m_text[end]))
      {
        ++end;
      }
      return end - first;
    };
    if (end < m_text.size() && m_text[end] == '-')
    {
      ++end;
    }
    if (end < m_text.size() && m_text[end] == '0')
    {
      ++end;
      if (end < m_text.size() && IsDigit(m_text[end]))
      {
        Move(end);
        return Fail(DiagnosticClass::Syntax, "a number with a leading zero");
      }
    }
    else if (digits() == 0)
    {
      Move(end);
      return Fail(DiagnosticClass::Syntax, "a number without digits");
    }
    if (end < m_text.size() && m_text[end] == '.')
    {
      ++end;
      if (digits() == 0)
      {
        Move(end);
        return Fail(DiagnosticClass::Syntax, "a number without digits after its decimal point");
      }
    }
    if (end < m_text.size() && (m_text[end] == 'e' || m_text[end] == 'E'))
    {
      ++end;
      if (end < m_text.size() && (m_text[end] == '+' || m_text[end] == '-'))
      {
        ++end;
      }
      if (digits() == 0)
      {
        Move(end);
        return Fail(DiagnosticClass::Syntax, "a number without digits in its exponent");
      }
    }
    if (end < m_text.size() && (IsDigit(m_text[end]) || m_text[end] == '.' || m_text[end] == 'e' || m_text[end] == 'E'))
    {
      Move(end);
      return Fail(DiagnosticClass::Syntax, "a malformed number");
    }
    const std::string_view token = m_text.substr(start, end - start);
    // DAT-023: one correctly rounded conversion, the same on every build host.
    const std::from_chars_result converted = std::from_chars(token.data(), token.data() + token.size(), _out.number);
    if (converted.ec == std::errc::result_out_of_range)
    {
      return Fail(DiagnosticClass::OutOfRange, "a number outside the binary64 range");
    }
    if (converted.ec != std::errc{} || converted.ptr != token.data() + token.size())
    {
      return Fail(DiagnosticClass::Syntax, "a number that does not convert");
    }
    _out.kind = JsonKind::Number;
    _out.text = std::string(token);
    Move(end);
    return true;
  }

  bool ParseHex4(std::uint32_t& _out)
  {
    _out = 0;
    for (int i = 0; i < 4; ++i)
    {
      const char c = Peek();
      std::uint32_t digit = 0;
      if (IsDigit(c))
      {
        digit = static_cast<std::uint32_t>(c - '0');
      }
      else if (c >= 'a' && c <= 'f')
      {
        digit = static_cast<std::uint32_t>(c - 'a' + 10);
      }
      else if (c >= 'A' && c <= 'F')
      {
        digit = static_cast<std::uint32_t>(c - 'A' + 10);
      }
      else
      {
        return Fail(DiagnosticClass::Syntax, "a \\u escape without four hexadecimal digits");
      }
      _out = _out * 16 + digit;
      Next();
    }
    return true;
  }

  bool ParseString(std::string& _out)
  {
    Next(); // the opening quote
    while (true)
    {
      if (AtEnd())
      {
        return Fail(DiagnosticClass::Syntax, "an unterminated string");
      }
      const char c = Peek();
      if (c == '"')
      {
        Next();
        return true;
      }
      if (static_cast<unsigned char>(c) < 0x20)
      {
        return Fail(DiagnosticClass::Syntax, "a control character in a string");
      }
      if (c != '\\')
      {
        _out.push_back(c);
        Next();
        continue;
      }
      Next();
      const char escape = Peek();
      Next();
      switch (escape)
      {
      case '"':
      case '\\':
      case '/':
        _out.push_back(escape);
        break;
      case 'b':
        _out.push_back('\b');
        break;
      case 'f':
        _out.push_back('\f');
        break;
      case 'n':
        _out.push_back('\n');
        break;
      case 'r':
        _out.push_back('\r');
        break;
      case 't':
        _out.push_back('\t');
        break;
      case 'u':
      {
        std::uint32_t unit = 0;
        if (!ParseHex4(unit))
        {
          return false;
        }
        if (unit >= 0xDC00 && unit <= 0xDFFF)
        {
          return Fail(DiagnosticClass::Encoding, "an unpaired surrogate escape");
        }
        if (unit >= 0xD800 && unit <= 0xDBFF)
        {
          std::uint32_t low = 0;
          if (m_text.substr(m_position, 2) != "\\u")
          {
            return Fail(DiagnosticClass::Encoding, "an unpaired surrogate escape");
          }
          Move(m_position + 2);
          if (!ParseHex4(low))
          {
            return false;
          }
          if (low < 0xDC00 || low > 0xDFFF)
          {
            return Fail(DiagnosticClass::Encoding, "an unpaired surrogate escape");
          }
          unit = 0x10000 + ((unit - 0xD800) << 10) + (low - 0xDC00);
        }
        AppendUtf8(_out, unit);
        break;
      }
      default:
        return Fail(DiagnosticClass::Syntax, "an escape that JSON does not define");
      }
    }
  }

  bool ParseArray(JsonValue& _out, std::uint32_t _depth)
  {
    if (_depth > AUTHORING_MAX_DEPTH)
    {
      return Fail(DiagnosticClass::Depth, "arrays and objects nested deeper than dat.parse.max_depth");
    }
    _out.kind = JsonKind::Array;
    Next();
    SkipWhitespace();
    if (Peek() == ']')
    {
      Next();
      return true;
    }
    while (true)
    {
      m_path.push_back(std::format("[{}]", _out.items.size()));
      JsonValue& item = _out.items.emplace_back();
      if (!ParseValue(item, _depth))
      {
        return false;
      }
      m_path.pop_back();
      SkipWhitespace();
      if (Peek() == ',')
      {
        Next();
        SkipWhitespace();
        continue;
      }
      if (Peek() == ']')
      {
        Next();
        return true;
      }
      return Fail(DiagnosticClass::Syntax, AtEnd() ? "an unterminated array" : "an array element not followed by , or ]");
    }
  }

  bool ParseObject(JsonValue& _out, std::uint32_t _depth)
  {
    if (_depth > AUTHORING_MAX_DEPTH)
    {
      return Fail(DiagnosticClass::Depth, "arrays and objects nested deeper than dat.parse.max_depth");
    }
    _out.kind = JsonKind::Object;
    Next();
    SkipWhitespace();
    if (Peek() == '}')
    {
      Next();
      return true;
    }
    std::map<std::string, std::size_t> seen;
    while (true)
    {
      if (Peek() != '"')
      {
        return Fail(DiagnosticClass::Syntax, AtEnd() ? "an unterminated object" : "an object member without a quoted key");
      }
      JsonMember member;
      member.line = m_line;
      member.column = m_column;
      if (!ParseString(member.key))
      {
        return false;
      }
      m_path.push_back(member.key);
      // DAT-022: a key twice is an error that names both places; neither occurrence is kept.
      if (const auto found = seen.find(member.key); found != seen.end())
      {
        const JsonMember& first = _out.members[found->second];
        ContentDiagnostic duplicate =
          ErrorAt(DiagnosticClass::DuplicateKey, "a key that this object already holds", member.line, member.column);
        duplicate.also = SourceLocation{std::string(m_file), first.line, first.column, Path()};
        m_error = std::move(duplicate);
        return false;
      }
      SkipWhitespace();
      if (Peek() != ':')
      {
        return Fail(DiagnosticClass::Syntax, "an object key not followed by :");
      }
      Next();
      SkipWhitespace();
      if (!ParseValue(member.value, _depth))
      {
        return false;
      }
      m_path.pop_back();
      seen.emplace(member.key, _out.members.size());
      _out.members.push_back(std::move(member));
      SkipWhitespace();
      if (Peek() == ',')
      {
        Next();
        SkipWhitespace();
        continue;
      }
      if (Peek() == '}')
      {
        Next();
        return true;
      }
      return Fail(DiagnosticClass::Syntax, AtEnd() ? "an unterminated object" : "an object member not followed by , or }");
    }
  }

  std::string_view m_text;
  std::string_view m_file;
  std::size_t m_position = 0;
  std::uint32_t m_line = 1;
  std::uint32_t m_column = 1;
  std::vector<std::string> m_path;
  std::optional<ContentDiagnostic> m_error;
};

} // namespace

std::expected<JsonDocument, ContentDiagnostic> AuthoringParser::Parse(std::span<const std::byte> _bytes, std::string_view _file) const
{
  return Parse(std::string_view(reinterpret_cast<const char*>(_bytes.data()), _bytes.size()), _file);
}

std::expected<JsonDocument, ContentDiagnostic> AuthoringParser::Parse(std::string_view _text, std::string_view _file) const
{
  Parser parser(_text, _file);
  std::expected<JsonValue, ContentDiagnostic> root = parser.Run();
  if (!root)
  {
    return std::unexpected(std::move(root.error()));
  }
  return JsonDocument{std::string(_file), std::move(*root)};
}

} // namespace Neuron
