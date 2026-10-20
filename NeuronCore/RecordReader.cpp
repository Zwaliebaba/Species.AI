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
  return IntegerValue(Take(_object, _key), Joined(_object.path, _key), _minimum, INTEGER_NO_MAXIMUM);
}

std::int64_t RecordReader::Integer(RecordObject& _object, std::string_view _key, std::int64_t _minimum, std::int64_t _maximum)
{
  return IntegerValue(Take(_object, _key), Joined(_object.path, _key), _minimum, _maximum);
}

std::int64_t RecordReader::DefaultedInteger(RecordObject& _object, std::string_view _key, std::int64_t _minimum, std::int64_t _maximum,
                                            std::int64_t _default)
{
  const JsonValue* value = TakeIfPresent(_object, _key);
  return value == nullptr ? _default : IntegerValue(value, Joined(_object.path, _key), _minimum, _maximum);
}

std::string RecordReader::Text(RecordObject& _object, std::string_view _key, std::size_t _maxChars)
{
  const JsonValue* value = Take(_object, _key);
  if (value == nullptr)
  {
    return {};
  }
  const std::string path = Joined(_object.path, _key);
  if (value->kind != JsonKind::String)
  {
    Report(DiagnosticClass::WrongType, value->line, value->column, path, "a text is expected");
    return {};
  }
  if (CodePoints(value->text) > _maxChars)
  {
    Report(DiagnosticClass::TooLong, value->line, value->column, path, std::format("a text longer than {} characters", _maxChars));
    return {};
  }
  return value->text;
}

RecordList RecordReader::List(RecordObject& _object, std::string_view _key, std::size_t _minItems, std::size_t _maxItems)
{
  return ListValue(Take(_object, _key), Joined(_object.path, _key), _minItems, _maxItems);
}

RecordList RecordReader::DefaultedList(RecordObject& _object, std::string_view _key, std::size_t _minItems, std::size_t _maxItems)
{
  const JsonValue* value = TakeIfPresent(_object, _key);
  std::string path = Joined(_object.path, _key);
  return value == nullptr ? RecordList{nullptr, std::move(path)} : ListValue(value, std::move(path), _minItems, _maxItems);
}

RecordObject RecordReader::Element(const RecordList& _list, std::size_t _index)
{
  return Open(_index < _list.Size() ? &_list.value->items[_index] : nullptr, std::format("{}[{}]", _list.path, _index));
}

std::int64_t RecordReader::ElementInteger(const RecordList& _list, std::size_t _index, std::int64_t _minimum, std::int64_t _maximum)
{
  return IntegerValue(_index < _list.Size() ? &_list.value->items[_index] : nullptr, std::format("{}[{}]", _list.path, _index), _minimum,
                      _maximum);
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

const JsonValue* RecordReader::TakeIfPresent(RecordObject& _object, std::string_view _key)
{
  if (_object.value == nullptr)
  {
    return nullptr;
  }
  const auto& members = _object.value->members;
  const auto found = std::ranges::find_if(members, [_key](const JsonMember& _member) { return _member.key == _key; });
  if (found == members.end())
  {
    return nullptr;
  }
  _object.taken[static_cast<std::size_t>(found - members.begin())] = true;
  return &found->value;
}

std::int64_t RecordReader::IntegerValue(const JsonValue* _value, const std::string& _path, std::int64_t _minimum, std::int64_t _maximum)
{
  if (_value == nullptr)
  {
    return 0;
  }
  const std::string& token = _value->text;
  if (_value->kind != JsonKind::Number || token.find_first_of(".eE") != std::string::npos)
  {
    Report(DiagnosticClass::WrongType, _value->line, _value->column, _path, "an integer is expected");
    return 0;
  }
  std::int64_t result = 0;
  const std::from_chars_result converted = std::from_chars(token.data(), token.data() + token.size(), result);
  if (converted.ec != std::errc{} || converted.ptr != token.data() + token.size() || result < _minimum || result > _maximum)
  {
    Report(DiagnosticClass::OutOfRange, _value->line, _value->column, _path,
           _maximum == INTEGER_NO_MAXIMUM ? std::format("a value below {} or beyond 64 bits", _minimum)
                                          : std::format("a value outside {} to {}", _minimum, _maximum));
    return 0;
  }
  return result;
}

RecordList RecordReader::ListValue(const JsonValue* _value, std::string _path, std::size_t _minItems, std::size_t _maxItems)
{
  RecordList list;
  list.path = std::move(_path);
  if (_value == nullptr)
  {
    return list;
  }
  if (_value->kind != JsonKind::Array)
  {
    Report(DiagnosticClass::WrongType, _value->line, _value->column, list.path, "a list is expected");
    return list;
  }
  if (_value->items.size() < _minItems || _value->items.size() > _maxItems)
  {
    Report(DiagnosticClass::OutOfRange, _value->line, _value->column, list.path,
           _maxItems == LIST_NO_MAXIMUM ? std::format("a list of fewer than {} elements", _minItems)
                                        : std::format("a list of other than {} to {} elements", _minItems, _maxItems));
    return list;
  }
  list.value = _value;
  return list;
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
