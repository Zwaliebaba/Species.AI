#include "pch.h"

#include "SchemaModel.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <format>
#include <set>
#include <string_view>
#include <utility>

namespace SchemaGen
{

namespace
{

constexpr std::string_view SCHEMA_DRAFT = "https://json-schema.org/draft/2020-12/schema"; // dat.schema.draft

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

  // An object's properties, each listed in `required`, with no other property allowed.
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
      if (!listed.contains(member.key))
      {
        Fail(member.value, _path, std::format("{} is not required; M0's subset holds required fields only (ADR-098)", member.key));
      }
      if (_record && (member.key == "schema" || member.key == "version"))
      {
        continue; // the record's header, read by SoleEnumerator
      }
      const std::string path = _path.empty() ? member.key : std::format("{}.{}", _path, member.key);
      if (CheckIdentifier(member.value, path, member.key))
      {
        fields.push_back(Field(member.key, member.value, path));
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

  FieldModel Field(const std::string& _key, const Neuron::JsonValue& _property, const std::string& _path)
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
    const std::string presence = Text(_property, "x-species-presence", _path, true);
    if (presence != "required")
    {
      Fail(_property, _path, "x-species-presence is required: M0's subset reads required fields only (ADR-098)");
    }
    if (logical == "object")
    {
      OnlyKeys(_property, _path,
               {"type", "description", "properties", "required", "additionalProperties", "x-species-type", "x-species-presence"});
      if (type != "object")
      {
        Fail(_property, _path, "an object field has type object");
      }
      field.kind = FieldKind::Object;
      field.fields = Properties(_property, _path, false);
      return field;
    }
    OnlyKeys(_property, _path,
             {"type", "description", "minimum", "exclusiveMinimum", "maximum", "x-species-type", "x-species-unit", "x-species-precision",
              "x-species-presence"});
    field.unit = Text(_property, "x-species-unit", _path, true);
    field.minimum = Bound(_property, "minimum", _path);
    field.exclusiveMinimum = Bound(_property, "exclusiveMinimum", _path);
    field.maximum = Bound(_property, "maximum", _path);
    if (logical == "integer" || logical == "count" || logical == "ticks")
    {
      field.kind = logical == "integer" ? FieldKind::Integer : (logical == "count" ? FieldKind::Count : FieldKind::Ticks);
      if (type != "integer")
      {
        Fail(_property, _path, std::format("a {} field has type integer", logical));
      }
      if (field.exclusiveMinimum || field.maximum || Member(_property, "x-species-precision") != nullptr)
      {
        Fail(_property, _path, "an integer field of M0's subset declares a minimum only (ADR-098)");
      }
      // Table 3.1-A: a count and a duration in ticks are integers of at least 0.
      const double minimum = field.minimum.value_or(field.kind == FieldKind::Integer ? -1.0 : 0.0);
      if ((field.kind != FieldKind::Integer && minimum != 0.0) || (field.kind == FieldKind::Integer && !field.minimum))
      {
        Fail(_property, _path,
             field.kind == FieldKind::Integer ? "an integer field declares its minimum" : "a count or ticks field has minimum 0");
      }
      field.integerMinimum = static_cast<std::int64_t>(minimum);
      return field;
    }
    if (logical == "sim_decimal")
    {
      field.kind = FieldKind::SimDecimal;
      if (type != "number")
      {
        Fail(_property, _path, "a simulation decimal has type number");
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
    Fail(_property, _path, std::format("the logical type {} is outside M0's subset (ADR-098)", logical));
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
  model.nameSpace = model.project.starts_with("Neuron") ? "Neuron" : "Species";

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
  if (reader.Failed())
  {
    return std::unexpected(reader.TakeDiagnostics());
  }
  return model;
}

} // namespace SchemaGen
