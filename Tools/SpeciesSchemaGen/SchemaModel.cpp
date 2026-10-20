#include "pch.h"

#include "SchemaModel.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <climits>
#include <cstdint>
#include <format>
#include <set>
#include <string_view>
#include <utility>

namespace SchemaGen
{

namespace
{

constexpr std::string_view SCHEMA_DRAFT = "https://json-schema.org/draft/2020-12/schema"; // dat.schema.draft

// A tool project's folder, whose generated code takes the namespace of the project's name after it (R9).
constexpr std::string_view TOOL_PROJECT_PREFIX = "Tools/Species";

// AGENTS.md R11: the stems an identifier never spells, as Build/CheckProjectFiles.py lists them.
constexpr std::array<std::string_view, 11> BRITISH_STEMS{"colour",    "initialis", "serialis", "normalis", "quantis", "synchronis",
                                                         "behaviour", "neighbour", "centre",   "grey",     "cancell"};

// The C++ keywords, which no generated identifier may be.
constexpr auto CPP_KEYWORDS = std::to_array<std::string_view>(
  {"alignas",  "alignof",  "and",       "and_eq",       "asm",       "auto",       "bitand",        "bitor",       "bool",
   "break",    "case",     "catch",     "char",         "char8_t",   "char16_t",   "char32_t",      "class",       "compl",
   "concept",  "const",    "consteval", "constexpr",    "constinit", "const_cast", "continue",      "co_await",    "co_return",
   "co_yield", "decltype", "default",   "delete",       "do",        "double",     "dynamic_cast",  "else",        "enum",
   "explicit", "export",   "extern",    "false",        "float",     "for",        "friend",        "goto",        "if",
   "inline",   "int",      "long",      "mutable",      "namespace", "new",        "noexcept",      "not",         "not_eq",
   "nullptr",  "operator", "or",        "or_eq",        "private",   "protected",  "public",        "register",    "reinterpret_cast",
   "requires", "return",   "short",     "signed",       "sizeof",    "static",     "static_assert", "static_cast", "struct",
   "switch",   "template", "this",      "thread_local", "throw",     "true",       "try",           "typedef",     "typeid",
   "typename", "union",    "unsigned",  "using",        "virtual",   "void",       "volatile",      "wchar_t",     "while",
   "xor",      "xor_eq"});

/// The checks of one schema document; every finding is collected.
class Reader
{
public:
  explicit Reader(std::string_view _file)
    : m_file(_file)
  {
  }

  void Fail(const Neuron::JsonValue& _at, std::string_view _path, std::string_view _message)
  {
    Neuron::ContentDiagnostic diagnostic;
    diagnostic.diagnosticClass = Neuron::DiagnosticClass::FamilyRule;
    diagnostic.at = Neuron::SourceLocation{m_file, _at.line, _at.column, std::string(_path)};
    diagnostic.message = std::string(_message);
    m_diagnostics.push_back(std::move(diagnostic));
  }

  // Every key of the object is one of the allowed ones (DAT-015: any other keyword or annotation is refused).
  void OnlyKeys(const Neuron::JsonValue& _object, std::string_view _path, std::initializer_list<std::string_view> _allowed)
  {
    for (const Neuron::JsonMember& member : _object.members)
    {
      if (std::ranges::find(_allowed, std::string_view{member.key}) == _allowed.end())
      {
        Neuron::JsonValue at;
        at.line = member.line;
        at.column = member.column;
        Fail(at, _path, std::format("the keyword {} is outside the schema subset of ADR-062", member.key));
      }
    }
  }

  const Neuron::JsonValue* Member(const Neuron::JsonValue& _object, std::string_view _key) const
  {
    const auto found = std::ranges::find_if(_object.members, [_key](const Neuron::JsonMember& _member) { return _member.key == _key; });
    return found == _object.members.end() ? nullptr : &found->value;
  }

  std::string Text(const Neuron::JsonValue& _object, std::string_view _key, std::string_view _path, bool _required)
  {
    const Neuron::JsonValue* value = Member(_object, _key);
    if (value == nullptr)
    {
      if (_required)
      {
        Fail(_object, _path, std::format("{} is required", _key));
      }
      return {};
    }
    if (value->kind != Neuron::JsonKind::String)
    {
      Fail(*value, _path, std::format("{} is text", _key));
      return {};
    }
    return value->text;
  }

  // The only value of a one-member text enumeration: how `schema` and `version` state the record's id and version.
  std::string SoleEnumerator(const Neuron::JsonValue& _properties, std::string_view _key)
  {
    const Neuron::JsonValue* property = Member(_properties, _key);
    if (property == nullptr || property->kind != Neuron::JsonKind::Object)
    {
      Fail(_properties, _key, std::format("the record has no {} field", _key));
      return {};
    }
    OnlyKeys(*property, _key, {"type", "description", "enum"});
    const Neuron::JsonValue* values = Member(*property, "enum");
    if (Text(*property, "type", _key, true) != "string" || values == nullptr || values->kind != Neuron::JsonKind::Array ||
        values->items.size() != 1 || values->items[0].kind != Neuron::JsonKind::String)
    {
      Fail(*property, _key, std::format("{} is a text enumeration of one value", _key));
      return {};
    }
    return values->items[0].text;
  }

  bool CheckIdentifier(const Neuron::JsonValue& _at, std::string_view _path, std::string_view _key)
  {
    const bool snakeCase =
      !_key.empty() && _key[0] >= 'a' && _key[0] <= 'z' &&
      std::ranges::all_of(_key, [](char _c) { return (_c >= 'a' && _c <= 'z') || (_c >= '0' && _c <= '9') || _c == '_'; });
    if (!snakeCase || _key.ends_with('_') || _key.find("__") != std::string_view::npos)
    {
      Fail(_at, _path, "a field key is lower-case snake_case (DAT-002)");
      return false;
    }
    for (const std::string_view stem : BRITISH_STEMS)
    {
      if (_key.find(stem) != std::string_view::npos)
      {
        Fail(_at, _path, std::format("the key spells {}, which AGENTS.md R11 spells otherwise", stem));
        return false;
      }
    }
    if (std::ranges::find(CPP_KEYWORDS, CamelCase(_key)) != CPP_KEYWORDS.end())
    {
      Fail(_at, _path, "the key generates a C++ keyword");
      return false;
    }
    return true;
  }

  std::optional<double> Bound(const Neuron::JsonValue& _property, std::string_view _key, std::string_view _path)
  {
    const Neuron::JsonValue* value = Member(_property, _key);
    if (value == nullptr)
    {
      return std::nullopt;
    }
    if (value->kind != Neuron::JsonKind::Number)
    {
      Fail(*value, _path, std::format("{} is a number", _key));
      return std::nullopt;
    }
    return value->number;
  }

  // An integer bound or default, taken from its digits exactly (DAT-023).
  std::optional<std::int64_t> IntegerValue(const Neuron::JsonValue& _property, std::string_view _key, std::string_view _path)
  {
    const Neuron::JsonValue* value = Member(_property, _key);
    if (value == nullptr)
    {
      return std::nullopt;
    }
    std::int64_t result = 0;
    const std::string& token = value->text;
    if (value->kind != Neuron::JsonKind::Number ||
        std::from_chars(token.data(), token.data() + token.size(), result).ptr != token.data() + token.size())
    {
      Fail(*value, _path, std::format("{} is an integer", _key));
      return std::nullopt;
    }
    return result;
  }

  // An object's properties: a required field listed in `required` and a defaulted one not, with no other property
  // allowed.
  std::vector<FieldModel> Properties(const Neuron::JsonValue& _object, std::string_view _path, bool _record)
  {
    std::vector<FieldModel> fields;
    const Neuron::JsonValue* properties = Member(_object, "properties");
    const Neuron::JsonValue* required = Member(_object, "required");
    const Neuron::JsonValue* additional = Member(_object, "additionalProperties");
    if (properties == nullptr || properties->kind != Neuron::JsonKind::Object)
    {
      Fail(_object, _path, "an object declares its properties");
      return fields;
    }
    if (additional == nullptr || additional->kind != Neuron::JsonKind::Boolean || additional->boolean)
    {
      Fail(_object, _path, "an object declares additionalProperties false: nothing undeclared is read (DAT-026)");
    }
    std::set<std::string> listed;
    if (required == nullptr || required->kind != Neuron::JsonKind::Array)
    {
      Fail(_object, _path, "an object lists its required fields");
    }
    else
    {
      for (const Neuron::JsonValue& item : required->items)
      {
        if (item.kind == Neuron::JsonKind::String)
        {
          listed.insert(item.text);
        }
      }
    }
    for (const Neuron::JsonMember& member : properties->members)
    {
      if (_record && (member.key == "schema" || member.key == "version"))
      {
        if (!listed.contains(member.key))
        {
          Fail(member.value, _path, std::format("{} is required: it heads every record (DAT-016)", member.key));
        }
        continue; // the record's header, read by SoleEnumerator
      }
      const std::string path = _path.empty() ? member.key : std::format("{}.{}", _path, member.key);
      if (CheckIdentifier(member.value, path, member.key))
      {
        FieldModel field = Field(member.key, member.value, path, false);
        if (listed.contains(member.key) == field.defaulted)
        {
          Fail(member.value, _path,
               field.defaulted ? std::format("{} is defaulted, so required does not list it (DAT-011)", member.key)
                               : std::format("{} is required, so required lists it (DAT-011)", member.key));
        }
        fields.push_back(std::move(field));
      }
    }
    for (const std::string& key : listed)
    {
      if (Member(*properties, key) == nullptr)
      {
        Fail(*required, _path, std::format("{} is required but not declared", key));
      }
    }
    return fields;
  }

  // One property, or a list's element, which has no key and no presence of its own.
  FieldModel Field(const std::string& _key, const Neuron::JsonValue& _property, const std::string& _path, bool _element)
  {
    FieldModel field;
    field.key = _key;
    field.identifier = CamelCase(_key);
    field.typeName = PascalCase(_key);
    field.path = _path;
    if (_property.kind != Neuron::JsonKind::Object)
    {
      Fail(_property, _path, "a property is a schema object");
      return field;
    }
    const std::string type = Text(_property, "type", _path, true);
    const std::string logical = Text(_property, "x-species-type", _path, true);
    if (_element)
    {
      if (Member(_property, "x-species-presence") != nullptr)
      {
        Fail(_property, _path, "a list's element has no presence of its own");
      }
    }
    else if (const std::string presence = Text(_property, "x-species-presence", _path, true); presence == "defaulted")
    {
      field.defaulted = true;
    }
    else if (presence != "required")
    {
      Fail(_property, _path, "x-species-presence is required or defaulted in the generator's subset (ADR-108)");
    }
    if (logical == "object")
    {
      OnlyKeys(_property, _path,
               {"type", "description", "properties", "required", "additionalProperties", "x-species-type", "x-species-presence"});
      if (type != "object" || field.defaulted)
      {
        Fail(_property, _path, "an object field has type object and is required");
      }
      field.kind = FieldKind::Object;
      field.fields = Properties(_property, _path, false);
      return field;
    }
    if (logical == "asset_ref")
    {
      OnlyKeys(_property, _path, {"type", "description", "x-species-type", "x-species-presence", "x-species-ref", "x-species-max-chars"});
      field.kind = FieldKind::AssetRef;
      field.refKind = Text(_property, "x-species-ref", _path, true);
      const std::optional<std::int64_t> maxChars = IntegerValue(_property, "x-species-max-chars", _path);
      if (type != "string" || field.defaulted || _element || maxChars.value_or(0) <= 0)
      {
        Fail(_property, _path, "an asset reference is a required text field with its x-species-max-chars (DAT-009)");
      }
      field.maxChars = static_cast<std::size_t>(std::max<std::int64_t>(0, maxChars.value_or(0)));
      return field;
    }
    if (logical == "list")
    {
      OnlyKeys(
        _property, _path,
        {"type", "description", "items", "minItems", "maxItems", "default", "x-species-type", "x-species-presence", "x-species-order"});
      field.kind = FieldKind::List;
      const std::string order = Text(_property, "x-species-order", _path, true);
      field.orderMeaningful = order == "meaningful";
      const std::optional<std::int64_t> minItems = IntegerValue(_property, "minItems", _path);
      const std::optional<std::int64_t> maxItems = IntegerValue(_property, "maxItems", _path);
      const Neuron::JsonValue* fallback = Member(_property, "default");
      const Neuron::JsonValue* items = Member(_property, "items");
      if (type != "array" || _element || (order != "meaningful" && order != "none") || minItems.value_or(0) < 0 || maxItems.value_or(0) < 0)
      {
        Fail(_property, _path, "a list has type array, is no list's element and states x-species-order (DAT-013)");
      }
      if (field.defaulted != (fallback != nullptr) ||
          (fallback != nullptr && (fallback->kind != Neuron::JsonKind::Array || !fallback->items.empty())))
      {
        Fail(_property, _path, "a defaulted list, and only one, declares the default [], the empty list");
      }
      field.minItems = static_cast<std::size_t>(std::max<std::int64_t>(0, minItems.value_or(0)));
      if (maxItems)
      {
        field.maxItems = static_cast<std::size_t>(std::max<std::int64_t>(0, maxItems.value_or(0)));
      }
      if (items == nullptr)
      {
        Fail(_property, _path, "a list declares its items");
        return field;
      }
      FieldModel element = Field({}, *items, _path + "[]", true);
      element.typeName = PascalCase(_key) + "Item";
      if (element.kind != FieldKind::Object && element.kind != FieldKind::Integer && element.kind != FieldKind::Count &&
          element.kind != FieldKind::Ticks)
      {
        Fail(*items, _path, "a list's element is an object or an integer in the generator's subset (ADR-108)");
      }
      field.item.push_back(std::move(element));
      return field;
    }
    OnlyKeys(_property, _path,
             {"type", "description", "minimum", "exclusiveMinimum", "maximum", "default", "x-species-type", "x-species-unit",
              "x-species-precision", "x-species-presence"});
    field.unit = Text(_property, "x-species-unit", _path, true);
    if (logical == "integer" || logical == "count" || logical == "ticks")
    {
      field.kind = logical == "integer" ? FieldKind::Integer : (logical == "count" ? FieldKind::Count : FieldKind::Ticks);
      if (type != "integer")
      {
        Fail(_property, _path, std::format("a {} field has type integer", logical));
      }
      if (Member(_property, "exclusiveMinimum") != nullptr || Member(_property, "x-species-precision") != nullptr)
      {
        Fail(_property, _path, "an integer field declares a minimum and a maximum only (ADR-098)");
      }
      const std::optional<std::int64_t> minimum = IntegerValue(_property, "minimum", _path);
      // Table 3.1-A: a count and a duration in ticks are integers of at least 0.
      if ((field.kind != FieldKind::Integer && minimum.value_or(0) != 0) || (field.kind == FieldKind::Integer && !minimum))
      {
        Fail(_property, _path,
             field.kind == FieldKind::Integer ? "an integer field declares its minimum" : "a count or ticks field has minimum 0");
      }
      field.integerMinimum = minimum.value_or(0);
      field.integerMaximum = IntegerValue(_property, "maximum", _path);
      const std::optional<std::int64_t> fallback = IntegerValue(_property, "default", _path);
      if (field.defaulted != fallback.has_value() || fallback.value_or(field.integerMinimum) < field.integerMinimum ||
          fallback.value_or(field.integerMinimum) > field.integerMaximum.value_or(INT64_MAX))
      {
        Fail(_property, _path, "a defaulted integer, and only one, declares a default within its range (DAT-012)");
      }
      field.integerDefault = fallback.value_or(0);
      return field;
    }
    if (logical == "sim_decimal")
    {
      field.kind = FieldKind::SimDecimal;
      field.minimum = Bound(_property, "minimum", _path);
      field.exclusiveMinimum = Bound(_property, "exclusiveMinimum", _path);
      field.maximum = Bound(_property, "maximum", _path);
      if (type != "number" || field.defaulted || _element || Member(_property, "default") != nullptr)
      {
        Fail(_property, _path, "a simulation decimal has type number and is a required field");
      }
      const std::string precision = Text(_property, "x-species-precision", _path, true);
      if (precision == "binary64")
      {
        field.precision = Neuron::SimPrecision::Binary64;
      }
      else if (precision == "binary32-rounded")
      {
        field.precision = Neuron::SimPrecision::Binary32Rounded;
      }
      else
      {
        Fail(_property, _path, "x-species-precision is binary64 or binary32-rounded (DAT-010)");
      }
      return field;
    }
    Fail(_property, _path, std::format("the logical type {} is outside the generator's subset (ADR-108)", logical));
    return field;
  }

  std::vector<Neuron::ContentDiagnostic> TakeDiagnostics()
  {
    return std::exchange(m_diagnostics, {});
  }
  [[nodiscard]] bool Failed() const noexcept
  {
    return !m_diagnostics.empty();
  }

private:
  std::string m_file;
  std::vector<Neuron::ContentDiagnostic> m_diagnostics;
};

// A field whose cooked form has a fixed size: no text and no list, at any depth (DAT-014).
bool HasFixedSize(const FieldModel& _field)
{
  return _field.kind != FieldKind::AssetRef && _field.kind != FieldKind::List && std::ranges::all_of(_field.fields, HasFixedSize);
}

std::vector<std::string_view> Words(std::string_view _name)
{
  std::vector<std::string_view> words;
  std::size_t start = 0;
  for (std::size_t i = 0; i <= _name.size(); ++i)
  {
    if (i == _name.size() || _name[i] == '_' || _name[i] == '.')
    {
      if (i > start)
      {
        words.push_back(_name.substr(start, i - start));
      }
      start = i + 1;
    }
  }
  return words;
}

std::string Capitalized(std::string_view _word)
{
  std::string word(_word);
  if (!word.empty() && word[0] >= 'a' && word[0] <= 'z')
  {
    word[0] = static_cast<char>(word[0] - 'a' + 'A');
  }
  return word;
}

std::string UpperCase(std::string_view _name)
{
  std::string upper;
  for (const char c : _name)
  {
    upper.push_back(c == '.' ? '_' : (c >= 'a' && c <= 'z' ? static_cast<char>(c - 'a' + 'A') : c));
  }
  return upper;
}

} // namespace

std::string PascalCase(std::string_view _name)
{
  std::string result;
  for (const std::string_view word : Words(_name))
  {
    result += Capitalized(word);
  }
  return result;
}

std::string CamelCase(std::string_view _name)
{
  std::string result;
  for (const std::string_view word : Words(_name))
  {
    result += result.empty() ? std::string(word) : Capitalized(word);
  }
  return result;
}

std::expected<SchemaModel, std::vector<Neuron::ContentDiagnostic>> ReadSchema(const Neuron::JsonDocument& _document)
{
  Reader reader(_document.file);
  SchemaModel model;
  model.file = _document.file;
  const Neuron::JsonValue& root = _document.root;
  if (root.kind != Neuron::JsonKind::Object)
  {
    reader.Fail(root, "", "a schema is an object");
    return std::unexpected(reader.TakeDiagnostics());
  }
  reader.OnlyKeys(root, "",
                  {"$schema", "$id", "title", "description", "type", "properties", "required", "additionalProperties", "x-species-sim",
                   "x-species-project"});
  if (reader.Text(root, "$schema", "", true) != SCHEMA_DRAFT)
  {
    reader.Fail(root, "$schema", "$schema pins the draft dat.schema.draft, 2020-12 (DAT-015)");
  }
  if (reader.Text(root, "type", "", true) != "object")
  {
    reader.Fail(root, "type", "a record is an object");
  }
  model.id = reader.Text(root, "$id", "", true);
  model.title = reader.Text(root, "title", "", true);
  model.project = reader.Text(root, "x-species-project", "", true);
  const Neuron::JsonValue* simulation = reader.Member(root, "x-species-sim");
  if (simulation == nullptr || simulation->kind != Neuron::JsonKind::Boolean)
  {
    reader.Fail(root, "x-species-sim", "x-species-sim states whether the record type is simulation-relevant");
  }
  else
  {
    model.simulationRelevant = simulation->boolean;
  }
  model.typeName = PascalCase(model.id);
  model.constantName = UpperCase(model.id);
  if (model.project.starts_with("Neuron"))
  {
    model.nameSpace = "Neuron";
  }
  else if (model.project.starts_with(TOOL_PROJECT_PREFIX))
  {
    model.nameSpace = model.project.substr(TOOL_PROJECT_PREFIX.size());
  }
  else
  {
    model.nameSpace = "Species";
  }

  if (const Neuron::JsonValue* properties = reader.Member(root, "properties");
      properties != nullptr && properties->kind == Neuron::JsonKind::Object)
  {
    if (reader.SoleEnumerator(*properties, "schema") != model.id)
    {
      reader.Fail(*properties, "schema", "the schema field names the schema's own id");
    }
    const std::string version = reader.SoleEnumerator(*properties, "version");
    const std::size_t dot = version.find('.');
    std::uint16_t major = 0;
    std::uint16_t minor = 0;
    const bool parsed =
      dot != std::string::npos && std::from_chars(version.data(), version.data() + dot, major).ptr == version.data() + dot &&
      std::from_chars(version.data() + dot + 1, version.data() + version.size(), minor).ptr == version.data() + version.size();
    if (!parsed || major == 0)
    {
      reader.Fail(*properties, "version", "the version field's value is \"<major>.<minor>\", from 1.0 (DAT-017)");
    }
    model.version = Neuron::SchemaVersion{major, minor};
  }
  model.fields = reader.Properties(root, "", true);
  model.cooked = std::ranges::all_of(model.fields, HasFixedSize);
  if (reader.Failed())
  {
    return std::unexpected(reader.TakeDiagnostics());
  }
  return model;
}

} // namespace SchemaGen
