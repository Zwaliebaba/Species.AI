#include "pch.h"

#include "RecordReader.h"

#include <algorithm>
#include <charconv>
#include <format>
#include <system_error>
#include <utility>

namespace Neuron
{

namespace
{

std::size_t CodePoints(std::string_view _utf8) noexcept
{
  return static_cast<std::size_t>(std::ranges::count_if(_utf8, [](char _c) { return (static_cast<unsigned char>(_c) & 0xC0) != 0x80; }));
}

std::string Joined(std::string_view _path, std::string_view _key)
{
  return _path.empty() ? std::string(_key) : std::format("{}.{}", _path, _key);
}

} // namespace

RecordReader::RecordReader(std::string_view _file)
  : m_file(_file)
{
}

RecordObject RecordReader::Record(const JsonValue& _root, std::string_view _schemaId, SchemaVersion _version)
{
  RecordObject record = Open(&_root, {});
  if (record.value == nullptr)
  {
    return record;
  }
  if (const JsonValue* schema = Take(record, "schema");
      schema != nullptr && (schema->kind != JsonKind::String || schema->text != _schemaId))
  {
    Report(DiagnosticClass::UnknownValue, schema->line, schema->column, "schema", std::format("the file is not of schema {}", _schemaId));
  }
  const std::string expected = std::format("{}.{}", _version.major, _version.minor);
  if (const JsonValue* version = Take(record, "version");
      version != nullptr && (version->kind != JsonKind::String || version->text != expected))
  {
    Report(DiagnosticClass::Version, version->line, version->column, "version", std::format("the reader reads version {} only", expected));
  }
  return record;
}

RecordObject RecordReader::Object(RecordObject& _parent, std::string_view _key)
{
  return Open(Take(_parent, _key), Joined(_parent.path, _key));
}

std::int64_t RecordReader::Integer(RecordObject& _object, std::string_view _key, std::int64_t _minimum)
{
  const JsonValue* value = Take(_object, _key);
  if (value == nullptr)
  {
    return 0;
  }
  const std::string path = Joined(_object.path, _key);
  const std::string& token = value->text;
  if (value->kind != JsonKind::Number || token.find_first_of(".eE") != std::string::npos)
  {
    Report(DiagnosticClass::WrongType, value->line, value->column, path, "an integer is expected");
    return 0;
  }
  std::int64_t result = 0;
  const std::from_chars_result converted = std::from_chars(token.data(), token.data() + token.size(), result);
  if (converted.ec != std::errc{} || converted.ptr != token.data() + token.size() || result < _minimum)
  {
    Report(DiagnosticClass::OutOfRange, value->line, value->column, path, std::format("a value below {} or beyond 64 bits", _minimum));
    return 0;
  }
  return result;
}

double RecordReader::Decimal(RecordObject& _object, std::string_view _key, const DecimalRange& _range, SimPrecision _precision)
{
  const JsonValue* value = Take(_object, _key);
  if (value == nullptr)
  {
    return 0.0;
  }
  const std::string path = Joined(_object.path, _key);
  if (value->kind != JsonKind::Number)
  {
    Report(DiagnosticClass::WrongType, value->line, value->column, path, "a number is expected");
    return 0.0;
  }
  const double number = value->number;
  const bool belowMinimum = _range.minimum.has_value() && number < _range.minimum.value_or(0.0);
  const bool atOrBelowExclusive = _range.exclusiveMinimum.has_value() && number <= _range.exclusiveMinimum.value_or(0.0);
  const bool aboveMaximum = _range.maximum.has_value() && number > _range.maximum.value_or(0.0);
  if (belowMinimum || atOrBelowExclusive || aboveMaximum)
  {
    Report(DiagnosticClass::OutOfRange, value->line, value->column, path, "a value outside the field's declared range");
    return 0.0;
  }
  // DAT-010: a binary32-rounded decimal is rounded once, here, and widened back.
  return _precision == SimPrecision::Binary32Rounded ? static_cast<double>(static_cast<float>(number)) : number;
}

void RecordReader::Close(const RecordObject& _object)
{
  if (_object.value == nullptr)
  {
    return;
  }
  for (std::size_t i = 0; i < _object.value->members.size(); ++i)
  {
    const JsonMember& member = _object.value->members[i];
    const std::string path = Joined(_object.path, member.key);
    if (member.key == "notes")
    {
      if (member.value.kind != JsonKind::String)
      {
        Report(DiagnosticClass::WrongType, member.value.line, member.value.column, path, "a note is text");
      }
      else if (CodePoints(member.value.text) > NOTES_MAX_CHARS)
      {
        Report(DiagnosticClass::TooLong, member.value.line, member.value.column, path, "a note longer than dat.notes.max_chars");
      }
    }
    else if (!_object.taken[i])
    {
      Report(DiagnosticClass::UnknownField, member.line, member.column, path, "a field the schema does not declare");
    }
  }
}

std::vector<ContentDiagnostic> RecordReader::TakeDiagnostics()
{
  return std::exchange(m_diagnostics, {});
}

const JsonValue* RecordReader::Take(RecordObject& _object, std::string_view _key)
{
  if (_object.value == nullptr)
  {
    return nullptr; // the object itself was reported
  }
  const auto& members = _object.value->members;
  const auto found = std::ranges::find_if(members, [_key](const JsonMember& _member) { return _member.key == _key; });
  if (found == members.end())
  {
    Report(DiagnosticClass::MissingField, _object.value->line, _object.value->column, Joined(_object.path, _key),
           "a required field is absent");
    return nullptr;
  }
  _object.taken[static_cast<std::size_t>(found - members.begin())] = true;
  return &found->value;
}

RecordObject RecordReader::Open(const JsonValue* _value, std::string _path)
{
  RecordObject object;
  object.path = std::move(_path);
  if (_value == nullptr)
  {
    return object;
  }
  if (_value->kind != JsonKind::Object)
  {
    Report(DiagnosticClass::WrongType, _value->line, _value->column, object.path, "an object is expected");
    return object;
  }
  object.value = _value;
  object.taken.assign(_value->members.size(), false);
  return object;
}

void RecordReader::Report(DiagnosticClass _class, std::uint32_t _line, std::uint32_t _column, std::string_view _path,
                          std::string_view _message)
{
  ContentDiagnostic diagnostic;
  diagnostic.diagnosticClass = _class;
  diagnostic.at = SourceLocation{m_file, _line, _column, std::string(_path)};
  diagnostic.message = std::string(_message);
  m_diagnostics.push_back(std::move(diagnostic));
}

} // namespace Neuron
