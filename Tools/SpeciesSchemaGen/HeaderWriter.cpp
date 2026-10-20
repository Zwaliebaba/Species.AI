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

  // A struct and, before its members, the structs of its object fields.
  void Struct(const std::string& _name, const std::vector<FieldModel>& _fields, std::size_t _indent)
  {
    Line(_indent, std::format("struct {}", _name));
    Line(_indent, "{");
    bool first = true;
    for (const FieldModel& field : _fields)
    {
      if (field.kind == FieldKind::Object)
      {
        if (!first)
        {
          Line(0, "");
        }
        Struct(field.typeName, field.fields, _indent + 1);
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
      Line(_indent + 1, field.unit.empty() ? std::format("// {}", field.path) : std::format("// {}, in {}", field.path, field.unit));
      Line(_indent + 1, field.kind == FieldKind::SimDecimal ? std::format("double {} = 0.0;", field.identifier)
                                                            : std::format("std::int64_t {} = 0;", field.identifier));
      first = false;
    }
    Line(_indent, "};");
  }

  // One reader per struct, the nested ones first so that each is declared before its caller.
  void Readers(const std::string& _type, const std::vector<FieldModel>& _fields)
  {
    for (const FieldModel& field : _fields)
    {
      if (field.kind == FieldKind::Object)
      {
        Readers(std::format("{}::{}", _type, field.typeName), field.fields);
      }
    }
    // A signature past the column limit breaks after the second parameter, aligned after the open bracket, as
    // clang-format lays it out.
    const std::string head = "inline void ReadFields(Neuron::RecordReader& _reader, Neuron::RecordObject _object,";
    const std::string last = std::format("{}& _out)", _type);
    if (head.size() + 1 + last.size() <= GENERATED_COLUMN_LIMIT)
    {
      Line(0, head + " " + last);
    }
    else
    {
      Line(0, head);
      Line(0, std::string(head.find('(') + 1, ' ') + last);
    }
    Line(0, "{");
    for (const FieldModel& field : _fields)
    {
      switch (field.kind)
      {
      case FieldKind::Object:
        Line(1, std::format("ReadFields(_reader, _reader.Object(_object, \"{}\"), _out.{});", field.key, field.identifier));
        break;
      case FieldKind::Integer:
      case FieldKind::Count:
      case FieldKind::Ticks:
        Assignment(1, std::format("_out.{}", field.identifier),
                   std::format("_reader.Integer(_object, \"{}\", {})", field.key, field.integerMinimum));
        break;
      case FieldKind::SimDecimal:
        Assignment(1, std::format("_out.{}", field.identifier),
                   std::format("_reader.Decimal(_object, \"{}\", {}, Neuron::SimPrecision::{})", field.key, RangeLiteral(field),
                               field.precision == Neuron::SimPrecision::Binary64 ? "Binary64" : "Binary32Rounded"));
        break;
      }
    }
    Line(1, "_reader.Close(_object);");
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
  std::string m_text;
  std::string m_tooLong;
};

} // namespace

std::string HeaderPath(const SchemaModel& _model)
{
  return std::format("{}/{}.h", _model.project, _model.typeName);
}

std::expected<std::string, std::string> WriteHeader(const SchemaModel& _model)
{
  Writer out;
  out.Line(0, std::format("// Generated by SpeciesSchemaGen from {}, schema {} version {}.{}. Never edit it by hand:", _model.file,
                          _model.id, _model.version.major, _model.version.minor));
  out.Line(0, "// change the schema and run the generator (ADR-062, AGENTS.md §2).");
  out.Line(0, "#pragma once");
  out.Line(0, "");
  out.Line(0, "#include \"RecordReader.h\"");
  out.Line(0, "");
  out.Line(0, "#include <cstdint>");
  out.Line(0, "#include <expected>");
  out.Line(0, "#include <string_view>");
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
  out.Line(0, "");
  out.Line(0, std::format("}} // namespace {}", _model.nameSpace));
  if (!out.TooLong().empty())
  {
    return std::unexpected(std::format("a generated line passes column {}: {}", GENERATED_COLUMN_LIMIT, out.TooLong()));
  }
  return out.Text();
}

} // namespace SchemaGen
