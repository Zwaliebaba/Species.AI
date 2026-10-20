#include "pch.h"

#include "HeaderWriter.h"

#include <array>
#include <charconv>
#include <format>
#include <string_view>
#include <vector>

namespace SchemaGen
{

namespace
{

// The shortest text that reads back as the same binary64 value (DAT-041), always a floating-point literal.
std::string DoubleLiteral(double _value)
{
  std::array<char, 32> buffer{};
  const std::to_chars_result written = std::to_chars(buffer.data(), buffer.data() + buffer.size(), _value);
  std::string text(buffer.data(), written.ptr);
  if (text.find_first_of(".e") == std::string::npos)
  {
    text += ".0";
  }
  return text;
}

std::string RangeLiteral(const FieldModel& _field)
{
  std::vector<std::string> bounds;
  if (_field.minimum)
  {
    bounds.push_back(std::format(".minimum = {}", DoubleLiteral(_field.minimum.value_or(0.0))));
  }
  if (_field.exclusiveMinimum)
  {
    bounds.push_back(std::format(".exclusiveMinimum = {}", DoubleLiteral(_field.exclusiveMinimum.value_or(0.0))));
  }
  if (_field.maximum)
  {
    bounds.push_back(std::format(".maximum = {}", DoubleLiteral(_field.maximum.value_or(0.0))));
  }
  std::string text = "{";
  for (std::size_t i = 0; i < bounds.size(); ++i)
  {
    text += (i == 0 ? "" : ", ") + bounds[i];
  }
  return text + "}";
}

std::string IntegerMaximum(const FieldModel& _field)
{
  return _field.integerMaximum ? std::to_string(_field.integerMaximum.value_or(0)) : std::string("Neuron::INTEGER_NO_MAXIMUM");
}

// The C++ type of a field that is not an object.
std::string ValueType(const FieldModel& _field)
{
  switch (_field.kind)
  {
  case FieldKind::SimDecimal:
    return "double";
  case FieldKind::AssetRef:
    return "std::string";
  case FieldKind::List:
    return std::format("std::vector<{}>", _field.item.front().kind == FieldKind::Object ? _field.item.front().typeName : "std::int64_t");
  case FieldKind::Object:
  case FieldKind::Integer:
  case FieldKind::Count:
  case FieldKind::Ticks:
    break;
  }
  return "std::int64_t";
}

// What a field is initialised with, so a struct that no reader filled holds its defaults.
std::string Initializer(const FieldModel& _field)
{
  switch (_field.kind)
  {
  case FieldKind::SimDecimal:
    return " = 0.0";
  case FieldKind::AssetRef:
  case FieldKind::List:
  case FieldKind::Object:
    return {};
  case FieldKind::Integer:
  case FieldKind::Count:
  case FieldKind::Ticks:
    break;
  }
  return std::format(" = {}", _field.defaulted ? _field.integerDefault : 0);
}

// The structs that a field's own struct needs first: an object's, and a list's object element's.
const FieldModel* NestedStruct(const FieldModel& _field) noexcept
{
  if (_field.kind == FieldKind::Object)
  {
    return &_field;
  }
  if (_field.kind == FieldKind::List && _field.item.front().kind == FieldKind::Object)
  {
    return &_field.item.front();
  }
  return nullptr;
}

class Writer
{
public:
  void Line(std::size_t _indent, std::string_view _text)
  {
    std::string line = _text.empty() ? std::string{} : std::string(_indent * 2, ' ') + std::string(_text);
    if (line.size() > GENERATED_COLUMN_LIMIT && m_tooLong.empty())
    {
      m_tooLong = line;
    }
    m_text += line + "\n";
  }

  // An assignment past the column limit breaks after its = and continues one level in, as clang-format lays it out.
  void Assignment(std::size_t _indent, std::string_view _target, std::string_view _value)
  {
    const std::string whole = std::format("{} = {};", _target, _value);
    if (_indent * 2 + whole.size() <= GENERATED_COLUMN_LIMIT)
    {
      Line(_indent, whole);
      return;
    }
    Line(_indent, std::format("{} =", _target));
    Line(_indent + 1, std::format("{};", _value));
  }

  // A signature of two or three parameters: past the column limit it breaks before its last parameter, aligned after
  // the open bracket, as clang-format lays it out.
  void Signature(std::string_view _head, std::string_view _last)
  {
    if (_head.size() + 1 + _last.size() <= GENERATED_COLUMN_LIMIT)
    {
      Line(0, std::format("{} {}", _head, _last));
      return;
    }
    Line(0, _head);
    Line(0, std::string(_head.find('(') + 1, ' ') + std::string(_last));
  }

  // A struct and, before its members, the structs of its object fields and of its lists' object elements.
  void Struct(const std::string& _name, const std::vector<FieldModel>& _fields, std::size_t _indent)
  {
    Line(_indent, std::format("struct {}", _name));
    Line(_indent, "{");
    bool first = true;
    for (const FieldModel& field : _fields)
    {
      if (const FieldModel* nested = NestedStruct(field); nested != nullptr)
      {
        if (!first)
        {
          Line(0, "");
        }
        Struct(nested->typeName, nested->fields, _indent + 1);
        first = false;
      }
    }
    for (const FieldModel& field : _fields)
    {
      if (field.kind == FieldKind::Object)
      {
        if (!first)
        {
          Line(0, "");
          first = true;
        }
        Line(_indent + 1, std::format("{} {};", field.typeName, field.identifier));
        continue;
      }
      if (!first)
      {
        Line(0, "");
      }
      first = true;
      Line(_indent + 1, Comment(field));
      Line(_indent + 1, std::format("{} {}{};", ValueType(field), field.identifier, Initializer(field)));
      first = false;
    }
    Line(_indent, "};");
  }

  // One reader per struct, the nested ones first so that each is declared before its caller.
  void Readers(const std::string& _type, const std::vector<FieldModel>& _fields)
  {
    for (const FieldModel& field : _fields)
    {
      if (const FieldModel* nested = NestedStruct(field); nested != nullptr)
      {
        Readers(std::format("{}::{}", _type, nested->typeName), nested->fields);
      }
    }
    Signature("inline void ReadFields(Neuron::RecordReader& _reader, Neuron::RecordObject _object,", std::format("{}& _out)", _type));
    Line(0, "{");
    for (const FieldModel& field : _fields)
    {
      Read(field);
    }
    Line(1, "_reader.Close(_object);");
    Line(0, "}");
    Line(0, "");
  }

  // The cooked record's writer and reader per struct (DAT-014): every field in schema order, an integer as 64 bits and
  // a decimal as its binary64 bits, both little-endian.
  void Cookers(const std::string& _type, const std::vector<FieldModel>& _fields)
  {
    for (const FieldModel& field : _fields)
    {
      if (field.kind == FieldKind::Object)
      {
        Cookers(std::format("{}::{}", _type, field.typeName), field.fields);
      }
    }
    Signature("inline void CookFields(Neuron::CookedWriter& _writer,", std::format("const {}& _in)", _type));
    Line(0, "{");
    for (const FieldModel& field : _fields)
    {
      if (field.kind == FieldKind::Object)
      {
        Line(1, std::format("CookFields(_writer, _in.{});", field.identifier));
      }
      else
      {
        Line(1, std::format("_writer.{}(_in.{});", field.kind == FieldKind::SimDecimal ? "F64" : "I64", field.identifier));
      }
    }
    Line(0, "}");
    Line(0, "");
    Signature("inline void ReadCookedFields(Neuron::CookedReader& _reader,", std::format("{}& _out)", _type));
    Line(0, "{");
    for (const FieldModel& field : _fields)
    {
      if (field.kind == FieldKind::Object)
      {
        Line(1, std::format("ReadCookedFields(_reader, _out.{});", field.identifier));
      }
      else
      {
        Line(1, std::format("_out.{} = _reader.{}();", field.identifier, field.kind == FieldKind::SimDecimal ? "F64" : "I64"));
      }
    }
    Line(0, "}");
    Line(0, "");
  }

  [[nodiscard]] const std::string& Text() const noexcept
  {
    return m_text;
  }
  [[nodiscard]] const std::string& TooLong() const noexcept
  {
    return m_tooLong;
  }

private:
  static std::string Comment(const FieldModel& _field)
  {
    if (_field.kind == FieldKind::AssetRef)
    {
      return std::format("// {}, an asset of {}", _field.path, _field.refKind);
    }
    if (_field.kind == FieldKind::List)
    {
      const FieldModel& element = _field.item.front();
      const std::string order = _field.orderMeaningful ? "in its authored order" : "in no meaningful order";
      return element.kind == FieldKind::Object || element.unit.empty() ? std::format("// {}, {}", _field.path, order)
                                                                       : std::format("// {}, in {}, {}", _field.path, element.unit, order);
    }
    return _field.unit.empty() ? std::format("// {}", _field.path) : std::format("// {}, in {}", _field.path, _field.unit);
  }

  void Read(const FieldModel& _field)
  {
    const std::string target = std::format("_out.{}", _field.identifier);
    switch (_field.kind)
    {
    case FieldKind::Object:
      Line(1, std::format("ReadFields(_reader, _reader.Object(_object, \"{}\"), _out.{});", _field.key, _field.identifier));
      return;
    case FieldKind::Integer:
    case FieldKind::Count:
    case FieldKind::Ticks:
      if (_field.defaulted)
      {
        Assignment(1, target,
                   std::format("_reader.DefaultedInteger(_object, \"{}\", {}, {}, {})", _field.key, _field.integerMinimum,
                               IntegerMaximum(_field), _field.integerDefault));
      }
      else if (_field.integerMaximum)
      {
        Assignment(1, target,
                   std::format("_reader.Integer(_object, \"{}\", {}, {})", _field.key, _field.integerMinimum, IntegerMaximum(_field)));
      }
      else
      {
        Assignment(1, target, std::format("_reader.Integer(_object, \"{}\", {})", _field.key, _field.integerMinimum));
      }
      return;
    case FieldKind::SimDecimal:
      Assignment(1, target,
                 std::format("_reader.Decimal(_object, \"{}\", {}, Neuron::SimPrecision::{})", _field.key, RangeLiteral(_field),
                             _field.precision == Neuron::SimPrecision::Binary64 ? "Binary64" : "Binary32Rounded"));
      return;
    case FieldKind::AssetRef:
      Assignment(1, target, std::format("_reader.Text(_object, \"{}\", {})", _field.key, _field.maxChars));
      return;
    case FieldKind::List:
      ReadList(_field);
      return;
    }
  }

  // A list: its elements in their authored order, each an object read by its own reader or an integer.
  void ReadList(const FieldModel& _field)
  {
    const FieldModel& element = _field.item.front();
    const std::string maxItems = _field.maxItems ? std::to_string(_field.maxItems.value_or(0)) : std::string("Neuron::LIST_NO_MAXIMUM");
    Assignment(1, std::format("const Neuron::RecordList {}", _field.identifier),
               std::format("_reader.{}(_object, \"{}\", {}, {})", _field.defaulted ? "DefaultedList" : "List", _field.key, _field.minItems,
                           maxItems));
    Line(1, std::format("_out.{0}.resize({0}.Size());", _field.identifier));
    Line(1, std::format("for (std::size_t i = 0; i < {}.Size(); ++i)", _field.identifier));
    Line(1, "{");
    if (element.kind == FieldKind::Object)
    {
      Line(2, std::format("ReadFields(_reader, _reader.Element({0}, i), _out.{0}[i]);", _field.identifier));
    }
    else
    {
      Assignment(2, std::format("_out.{}[i]", _field.identifier),
                 std::format("_reader.ElementInteger({}, i, {}, {})", _field.identifier, element.integerMinimum, IntegerMaximum(element)));
    }
    Line(1, "}");
  }

  std::string m_text;
  std::string m_tooLong;
};

bool HasKind(const std::vector<FieldModel>& _fields, FieldKind _kind)
{
  for (const FieldModel& field : _fields)
  {
    if (field.kind == _kind || HasKind(field.fields, _kind) || HasKind(field.item, _kind))
    {
      return true;
    }
  }
  return false;
}

} // namespace

std::string HeaderPath(const SchemaModel& _model)
{
  return std::format("{}/{}.h", _model.project, _model.typeName);
}

std::expected<std::string, std::string> WriteHeader(const SchemaModel& _model)
{
  const bool lists = HasKind(_model.fields, FieldKind::List);
  const bool texts = HasKind(_model.fields, FieldKind::AssetRef);
  Writer out;
  // The mark that Build/CheckGeneratedSchemas.py finds opens the first line; a long schema name moves the instruction to
  // the second.
  const std::string source = std::format("// Generated by SpeciesSchemaGen from {}, schema {} version {}.{}.", _model.file, _model.id,
                                         _model.version.major, _model.version.minor);
  if (source.size() + std::string_view{" Never edit it by hand:"}.size() <= GENERATED_COLUMN_LIMIT)
  {
    out.Line(0, source + " Never edit it by hand:");
    out.Line(0, "// change the schema and run the generator (ADR-062, AGENTS.md §2).");
  }
  else
  {
    out.Line(0, source);
    out.Line(0, "// Never edit it by hand: change the schema and run the generator (ADR-062, AGENTS.md §2).");
  }
  out.Line(0, "#pragma once");
  out.Line(0, "");
  if (_model.cooked)
  {
    out.Line(0, "#include \"CookedRecord.h\"");
  }
  out.Line(0, "#include \"RecordReader.h\"");
  out.Line(0, "");
  if (_model.cooked || lists)
  {
    out.Line(0, "#include <cstddef>");
  }
  out.Line(0, "#include <cstdint>");
  out.Line(0, "#include <expected>");
  if (_model.cooked)
  {
    out.Line(0, "#include <span>");
  }
  if (_model.cooked || texts)
  {
    out.Line(0, "#include <string>");
  }
  out.Line(0, "#include <string_view>");
  if (_model.cooked || lists)
  {
    out.Line(0, "#include <vector>");
  }
  out.Line(0, "");
  out.Line(0, std::format("namespace {}", _model.nameSpace));
  out.Line(0, "{");
  out.Line(0, "");
  out.Line(0, std::format("inline constexpr std::string_view {}_SCHEMA_ID = \"{}\";", _model.constantName, _model.id));
  out.Line(0, std::format("inline constexpr Neuron::SchemaVersion {}_VERSION{{{}, {}}};", _model.constantName, _model.version.major,
                          _model.version.minor));
  out.Line(0, std::format("inline constexpr bool {}_SIMULATION_RELEVANT = {};", _model.constantName, _model.simulationRelevant));
  out.Line(0, "");
  out.Line(0, std::format("/// {}", _model.title));
  out.Struct(_model.typeName, _model.fields, 0);
  out.Line(0, "");
  out.Readers(_model.typeName, _model.fields);
  out.Line(0, std::format("// Reads a {} authoring file: the record, or every finding about it (DAT-024).", _model.id));
  out.Line(0,
           std::format("[[nodiscard]] inline Neuron::RecordResult<{0}> Read{0}(const Neuron::JsonDocument& _document)", _model.typeName));
  out.Line(0, "{");
  out.Line(1, "Neuron::RecordReader reader(_document.file);");
  out.Line(1, std::format("{} record;", _model.typeName));
  out.Line(1, std::format("ReadFields(reader, reader.Record(_document.root, {0}_SCHEMA_ID, {0}_VERSION), record);", _model.constantName));
  out.Line(1, "if (reader.Failed())");
  out.Line(1, "{");
  out.Line(2, "return std::unexpected(reader.TakeDiagnostics());");
  out.Line(1, "}");
  out.Line(1, "return record;");
  out.Line(0, "}");
  if (_model.cooked)
  {
    out.Line(0, "");
    out.Cookers(_model.typeName, _model.fields);
    out.Line(0, std::format("// Cooks a {} record into the bytes of its entry (DAT-014): what the content build writes.", _model.id));
    out.Line(0, std::format("[[nodiscard]] inline std::vector<std::byte> Cook{0}(const {0}& _record)", _model.typeName));
    out.Line(0, "{");
    out.Line(1, "Neuron::CookedWriter payload;");
    out.Line(1, "CookFields(payload, _record);");
    out.Line(1, std::format("return Neuron::CookRecordEntry({0}_SCHEMA_ID, {0}_VERSION, 1, payload.View());", _model.constantName));
    out.Line(0, "}");
    out.Line(0, "");
    out.Line(0, std::format("// Reads a cooked {} entry with no parsing: the record, or why the bytes are not one (DAT-050).", _model.id));
    out.Line(0, std::format("[[nodiscard]] inline std::expected<{0}, std::string> ReadCooked{0}(std::span<const std::byte> _entry)",
                            _model.typeName));
    out.Line(0, "{");
    out.Line(1, std::format("const auto payload = Neuron::OpenCookedRecord(_entry, {0}_SCHEMA_ID, {0}_VERSION, 1);", _model.constantName));
    out.Line(1, "if (!payload)");
    out.Line(1, "{");
    out.Line(2, "return std::unexpected(payload.error());");
    out.Line(1, "}");
    out.Line(1, "Neuron::CookedReader reader(*payload);");
    out.Line(1, std::format("{} record;", _model.typeName));
    out.Line(1, "ReadCookedFields(reader, record);");
    out.Line(1, "if (!reader.Finished())");
    out.Line(1, "{");
    out.Line(2, std::format("return std::unexpected(std::string(\"the payload is not the size of a {} record\"));", _model.id));
    out.Line(1, "}");
    out.Line(1, "return record;");
    out.Line(0, "}");
  }
  out.Line(0, "");
  out.Line(0, std::format("}} // namespace {}", _model.nameSpace));
  if (!out.TooLong().empty())
  {
    return std::unexpected(std::format("a generated line passes column {}: {}", GENERATED_COLUMN_LIMIT, out.TooLong()));
  }
  return out.Text();
}

} // namespace SchemaGen
