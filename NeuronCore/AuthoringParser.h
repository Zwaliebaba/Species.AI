#pragma once

#include "ContentDiagnostic.h"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Neuron
{

// dat.parse.max_depth: the deepest nesting of arrays and objects an authoring file may hold (DAT-021).
inline constexpr std::uint32_t AUTHORING_MAX_DEPTH = 64;

enum class JsonKind : std::uint8_t
{
  Null,
  Boolean,
  Number,
  String,
  Array,
  Object
};

struct JsonMember;

/// One value of an authoring file, with its line and column (DAT-020). A number keeps its text, so an integer field
/// takes its decimal digits exactly (DAT-023), and its binary64 value, converted once and correctly rounded.
struct JsonValue
{
  JsonKind kind = JsonKind::Null;
  std::uint32_t line = 0;
  std::uint32_t column = 0;
  bool boolean = false;
  double number = 0.0;
  std::string text;                // a string's decoded UTF-8, or a number's token
  std::vector<JsonValue> items;    // an array's elements
  std::vector<JsonMember> members; // an object's members, in the order they were written
};

struct JsonMember
{
  std::string key;
  std::uint32_t line = 0;
  std::uint32_t column = 0;
  JsonValue value;
};

/// A parsed authoring file: every key and value with its location (DAT-020).
struct JsonDocument
{
  std::string file;
  JsonValue root;
};

/// The strict authoring parser of DAT-019 to DAT-023: RFC 8259 JSON with the I-JSON restrictions of RFC 7493. The
/// text is UTF-8, a single leading byte-order mark is skipped, invalid UTF-8 and unpaired surrogate escapes are errors,
/// every number is within the binary64 range, and no object holds a key twice. Line endings and whitespace carry no
/// meaning. Malformed input ends the parse with one diagnostic that names its location, never a hang, a crash or a
/// shortened value (DAT-021). Hand-written, in a project that holds no generated code (ADR-062).
class AuthoringParser
{
public:
  [[nodiscard]] std::expected<JsonDocument, ContentDiagnostic> Parse(std::span<const std::byte> _bytes, std::string_view _file) const;
  [[nodiscard]] std::expected<JsonDocument, ContentDiagnostic> Parse(std::string_view _text, std::string_view _file) const;
};

} // namespace Neuron
