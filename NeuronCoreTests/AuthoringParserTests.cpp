#include "pch.h"

#include "AuthoringParser.h"
#include "RecordReader.h"
#include "TestSupport.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

namespace
{

Neuron::JsonDocument Parsed(std::string_view _text)
{
  auto document = Neuron::AuthoringParser{}.Parse(_text, "test.json");
  if (!document)
  {
    Assert::Fail(L"the text did not parse");
  }
  return std::move(document).value_or(Neuron::JsonDocument{});
}

// The diagnostic of a text that must not parse.
Neuron::ContentDiagnostic Refused(std::string_view _text)
{
  auto document = Neuron::AuthoringParser{}.Parse(_text, "test.json");
  Assert::IsFalse(document.has_value(), L"the text is refused");
  return document.has_value() ? Neuron::ContentDiagnostic{} : document.error();
}

void ExpectRefused(std::string_view _text, Neuron::DiagnosticClass _class, std::uint32_t _line, std::uint32_t _column,
                   std::wstring_view _what)
{
  const Neuron::ContentDiagnostic diagnostic = Refused(_text);
  Assert::IsTrue(diagnostic.diagnosticClass == _class, std::format(L"{}: the class", _what).c_str());
  ExpectEqual(_line, diagnostic.at.line, std::format(L"{}: the line", _what));
  ExpectEqual(_column, diagnostic.at.column, std::format(L"{}: the column", _what));
  Assert::IsTrue(diagnostic.at.file == "test.json", L"the file");
}

} // namespace

TEST_CLASS(AuthoringParserTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Dat020EveryValueHasItsLineAndColumn)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DAT-020")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dat020EveryValueHasItsLineAndColumn)
  {
    // A byte-order mark is skipped, CR LF reads as LF, and a column counts code points.
    const Neuron::JsonDocument document =
      Parsed("\xEF\xBB\xBF{\r\n  \"na\xC3\xAFve\": [1, -2.5e1, true, null],\r\n  \"s\": \"\\u00e9\\ud83d\\ude00\"\r\n}");
    const Neuron::JsonValue& root = document.root;
    Assert::IsTrue(root.kind == Neuron::JsonKind::Object && root.members.size() == 2, L"an object of two members");
    const Neuron::JsonMember& list = root.members[0];
    Assert::IsTrue(list.key == "na\xC3\xAFve", L"a key keeps its UTF-8");
    ExpectEqual(2, list.line, L"the key's line");
    ExpectEqual(3, list.column, L"the key's column");
    ExpectEqual(4, static_cast<std::int64_t>(list.value.items.size()), L"the array's items");
    const Neuron::JsonValue& number = list.value.items[1];
    Assert::IsTrue(number.kind == Neuron::JsonKind::Number && number.number == -25.0 && number.text == "-2.5e1",
                   L"a number keeps its text");
    ExpectEqual(16, number.column, L"a column after a two-byte character counts it once");
    Assert::IsTrue(list.value.items[2].kind == Neuron::JsonKind::Boolean && list.value.items[2].boolean, L"true");
    Assert::IsTrue(list.value.items[3].kind == Neuron::JsonKind::Null, L"null");
    Assert::IsTrue(root.members[1].value.text == "\xC3\xA9\xF0\x9F\x98\x80", L"escapes and a surrogate pair decode to UTF-8");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dat021MalformedTextIsRefusedWhereItBreaks)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DAT-021")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dat021MalformedTextIsRefusedWhereItBreaks)
  {
    using Neuron::DiagnosticClass;
    ExpectRefused("", DiagnosticClass::Syntax, 1, 1, L"an empty file");
    ExpectRefused("{\"a\": 1", DiagnosticClass::Syntax, 1, 8, L"an unterminated object");
    ExpectRefused("[1, 2", DiagnosticClass::Syntax, 1, 6, L"an unterminated array");
    ExpectRefused("\"abc", DiagnosticClass::Syntax, 1, 5, L"an unterminated string");
    ExpectRefused("{\"a\":}", DiagnosticClass::Syntax, 1, 6, L"a missing value");
    ExpectRefused("1 2", DiagnosticClass::Syntax, 1, 3, L"a stray value after the top-level one");
    ExpectRefused("\n\n  tru", DiagnosticClass::Syntax, 3, 3, L"a broken literal");
    ExpectRefused("[01]", DiagnosticClass::Syntax, 1, 3, L"a leading zero");
    ExpectRefused("[1.]", DiagnosticClass::Syntax, 1, 4, L"no digits after the point");
    ExpectRefused("[1e]", DiagnosticClass::Syntax, 1, 4, L"no digits in the exponent");
    ExpectRefused("[-]", DiagnosticClass::Syntax, 1, 3, L"a sign without digits");
    ExpectRefused("[+1]", DiagnosticClass::Syntax, 1, 2, L"a plus sign");
    ExpectRefused("[.5]", DiagnosticClass::Syntax, 1, 2, L"no leading digit");
    ExpectRefused("[\"a\tb\"]", DiagnosticClass::Syntax, 1, 4, L"a control character in a string");
    ExpectRefused("[\"\\x\"]", DiagnosticClass::Syntax, 1, 5, L"an undefined escape");
    ExpectRefused("[1e400]", DiagnosticClass::OutOfRange, 1, 2, L"beyond binary64");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dat019InvalidUtf8AndUnpairedSurrogatesAreRefused)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DAT-019")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dat019InvalidUtf8AndUnpairedSurrogatesAreRefused)
  {
    using Neuron::DiagnosticClass;
    ExpectRefused("[\"ok\",\n \"\xC0\xAF\"]", DiagnosticClass::Encoding, 2, 3, L"an overlong form");
    ExpectRefused("\"\xED\xA0\x80\"", DiagnosticClass::Encoding, 1, 2, L"an encoded surrogate");
    ExpectRefused("\"\xF4\x90\x80\x80\"", DiagnosticClass::Encoding, 1, 2, L"above U+10FFFF");
    ExpectRefused("\"\xE2\x82\"", DiagnosticClass::Encoding, 1, 2, L"a truncated sequence");
    ExpectRefused("\"\\ud800\"", DiagnosticClass::Encoding, 1, 8, L"a high surrogate alone");
    ExpectRefused("\"\\udc00\"", DiagnosticClass::Encoding, 1, 8, L"a low surrogate alone");
    ExpectRefused("\"\\ud800\\u0041\"", DiagnosticClass::Encoding, 1, 14, L"a high surrogate before no low one");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dat022AKeyTwiceNamesBothPlaces)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DAT-022")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dat022AKeyTwiceNamesBothPlaces)
  {
    const Neuron::ContentDiagnostic diagnostic = Refused("{\n  \"a\": {\"b\": 1,\n    \"b\": 2}\n}");
    Assert::IsTrue(diagnostic.diagnosticClass == Neuron::DiagnosticClass::DuplicateKey, L"duplicate_key");
    ExpectEqual(3, diagnostic.at.line, L"the second occurrence's line");
    ExpectEqual(5, diagnostic.at.column, L"its column");
    Assert::IsTrue(diagnostic.at.fieldPath == "a.b", L"the field path");
    Assert::IsTrue(diagnostic.also.has_value(), L"the first occurrence");
    const Neuron::SourceLocation first = diagnostic.also.value_or(Neuron::SourceLocation{});
    ExpectEqual(2, first.line, L"the first occurrence's line");
    ExpectEqual(9, first.column, L"its column");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dat021NestingDeeperThanTheLimitIsRefused)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DAT-021")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dat021NestingDeeperThanTheLimitIsRefused)
  {
    const auto nested = [](std::size_t _depth) { return std::string(_depth, '[') + std::string(_depth, ']'); };
    static_cast<void>(Parsed(nested(Neuron::AUTHORING_MAX_DEPTH)));
    const Neuron::ContentDiagnostic diagnostic = Refused(nested(Neuron::AUTHORING_MAX_DEPTH + 1));
    Assert::IsTrue(diagnostic.diagnosticClass == Neuron::DiagnosticClass::Depth, L"depth");
    ExpectEqual(Neuron::AUTHORING_MAX_DEPTH + 1, diagnostic.at.column, L"at the bracket that goes too deep");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dat021NoInputHangsCrashesOrReadsOutside)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DAT-021")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dat021NoInputHangsCrashesOrReadsOutside)
  {
    // Every prefix and every single-byte change of a valid file ends with a document or one diagnostic.
    const std::string valid = "{\"schema\": \"x.y\", \"list\": [1, 2.5, -3e2, \"t\\u00e9\", {\"k\": [true, false, null]}]}";
    std::size_t refused = 0;
    for (std::size_t length = 0; length <= valid.size(); ++length)
    {
      refused += Neuron::AuthoringParser{}.Parse(std::string_view(valid).substr(0, length), "t").has_value() ? 0 : 1;
    }
    for (std::size_t at = 0; at < valid.size(); ++at)
    {
      for (const char replacement : {'\0', '"', '\\', '{', ']', ',', '\xFF', '9', 'e', ' '})
      {
        std::string mutated = valid;
        mutated[at] = replacement;
        refused += Neuron::AuthoringParser{}.Parse(mutated, "t").has_value() ? 0 : 1;
      }
    }
    Assert::IsTrue(refused > valid.size(), L"truncations and most changes are refused, each with its diagnostic");
  }
};

TEST_CLASS(RecordReaderTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Dat026EveryFindingOfARecordIsReported)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DAT-026")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dat026EveryFindingOfARecordIsReported)
  {
    const Neuron::JsonDocument document = Parsed(R"({
  "schema": "test.record",
  "version": "1.0",
  "notes": "a note",
  "group": {"count": 3, "ratio": 0.25, "extra": 1},
  "other": {"count": -1, "ratio": "x", "whole": 1.5}
})");
    Neuron::RecordReader reader("test.json");
    Neuron::RecordObject record = reader.Record(document.root, "test.record", {1, 0});
    Neuron::RecordObject group = reader.Object(record, "group");
    ExpectEqual(3, reader.Integer(group, "count", 0), L"an integer");
    Assert::IsTrue(reader.Decimal(group, "ratio", {.minimum = 0.0, .maximum = 1.0}, Neuron::SimPrecision::Binary64) == 0.25, L"a decimal");
    reader.Close(group);
    const std::vector<Neuron::ContentDiagnostic> extra = reader.TakeDiagnostics();
    Assert::IsTrue(extra.size() == 1 && extra[0].diagnosticClass == Neuron::DiagnosticClass::UnknownField &&
                     extra[0].at.fieldPath == "group.extra",
                   L"so far one finding: the undeclared field");
    Neuron::RecordObject other = reader.Object(record, "other");
    static_cast<void>(reader.Integer(other, "count", 0));
    static_cast<void>(reader.Decimal(other, "ratio", {}, Neuron::SimPrecision::Binary64));
    static_cast<void>(reader.Integer(other, "whole", 0));
    static_cast<void>(reader.Integer(other, "absent", 0));
    reader.Close(other);
    static_cast<void>(reader.Object(record, "missing"));
    reader.Close(record);
    const std::vector<Neuron::ContentDiagnostic> found = reader.TakeDiagnostics();
    const auto has = [&found](Neuron::DiagnosticClass _class, std::string_view _path)
    {
      return std::ranges::any_of(found, [&](const Neuron::ContentDiagnostic& _d)
                                 { return _d.diagnosticClass == _class && _d.at.fieldPath == _path; });
    };
    Assert::IsTrue(has(Neuron::DiagnosticClass::OutOfRange, "other.count"), L"below the minimum");
    Assert::IsTrue(has(Neuron::DiagnosticClass::WrongType, "other.ratio"), L"text for a number");
    Assert::IsTrue(has(Neuron::DiagnosticClass::WrongType, "other.whole"), L"a fraction for an integer");
    Assert::IsTrue(has(Neuron::DiagnosticClass::MissingField, "other.absent"), L"an absent field");
    Assert::IsTrue(has(Neuron::DiagnosticClass::MissingField, "missing"), L"an absent object");
    ExpectEqual(5, static_cast<std::int64_t>(found.size()), L"each finding once, and notes accepted");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dat016TheHeaderNamesTheSchemaAndVersion)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DAT-016")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dat016TheHeaderNamesTheSchemaAndVersion)
  {
    for (const auto& [text, expected] :
         {std::pair{R"({"schema": "other.record", "version": "1.0"})", Neuron::DiagnosticClass::UnknownValue},
          std::pair{R"({"schema": "test.record", "version": "2.0"})", Neuron::DiagnosticClass::Version},
          std::pair{R"({"version": "1.0"})", Neuron::DiagnosticClass::MissingField},
          std::pair{R"({"schema": "test.record", "version": "1.0", "notes": 5})", Neuron::DiagnosticClass::WrongType}})
    {
      const Neuron::JsonDocument document = Parsed(text);
      Neuron::RecordReader reader("test.json");
      const Neuron::RecordObject record = reader.Record(document.root, "test.record", {1, 0});
      reader.Close(record);
      const std::vector<Neuron::ContentDiagnostic> found = reader.TakeDiagnostics();
      Assert::IsTrue(found.size() == 1 && found[0].diagnosticClass == expected, L"one finding of its class");
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dat010ADecimalIsHeldAtItsPrecision)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DAT-010")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dat010ADecimalIsHeldAtItsPrecision)
  {
    const Neuron::JsonDocument document =
      Parsed(R"({"schema": "t.r", "version": "1.0", "a": 0.1, "b": 0.1, "c": 0, "d": 9223372036854775808})");
    Neuron::RecordReader reader("test.json");
    Neuron::RecordObject record = reader.Record(document.root, "t.r", {1, 0});
    Assert::IsTrue(reader.Decimal(record, "a", {}, Neuron::SimPrecision::Binary64) == 0.1, L"binary64 keeps the value");
    Assert::IsTrue(reader.Decimal(record, "b", {}, Neuron::SimPrecision::Binary32Rounded) == static_cast<double>(0.1F), L"rounded once");
    static_cast<void>(reader.Decimal(record, "c", {.exclusiveMinimum = 0.0}, Neuron::SimPrecision::Binary64));
    static_cast<void>(reader.Integer(record, "d", 0));
    reader.Close(record);
    const std::vector<Neuron::ContentDiagnostic> found = reader.TakeDiagnostics();
    ExpectEqual(2, static_cast<std::int64_t>(found.size()), L"0 is not above an exclusive 0, and 2^63 is beyond 64 bits");
  }
};

} // namespace NeuronCoreTests
