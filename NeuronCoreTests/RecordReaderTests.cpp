#include "pch.h"

#include "AuthoringParser.h"
#include "RecordReader.h"
#include "TestSupport.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

namespace
{

Neuron::JsonDocument Parsed(std::string_view _text)
{
  auto document = Neuron::AuthoringParser{}.Parse(_text, "test.json");
  Assert::IsTrue(document.has_value(), L"the text parses");
  return document.value_or(Neuron::JsonDocument{});
}

bool Has(const std::vector<Neuron::ContentDiagnostic>& _found, Neuron::DiagnosticClass _class, std::string_view _path)
{
  return std::ranges::any_of(_found,
                             [&](const Neuron::ContentDiagnostic& _d) { return _d.diagnosticClass == _class && _d.at.fieldPath == _path; });
}

} // namespace

TEST_CLASS(RecordReaderTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dat012ADefaultedFieldTakesItsDeclaredDefault)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DAT-012")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dat012ADefaultedFieldTakesItsDeclaredDefault)
  {
    const Neuron::JsonDocument document = Parsed(R"({"schema": "a.b", "version": "1.0", "given": 7})");
    Neuron::RecordReader reader(document.file);
    Neuron::RecordObject record = reader.Record(document.root, "a.b", {1, 0});
    ExpectEqual(7, reader.DefaultedInteger(record, "given", 0, 10, 63), L"a given value");
    ExpectEqual(63, reader.DefaultedInteger(record, "omitted", 0, 10, 63), L"an omitted value is its default");
    const Neuron::RecordList list = reader.DefaultedList(record, "pairs", 0, Neuron::LIST_NO_MAXIMUM);
    ExpectEqual(0, static_cast<std::int64_t>(list.Size()), L"an omitted list is empty");
    reader.Close(record);
    Assert::IsFalse(reader.Failed(), L"no finding");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dat013AListReadsItsElementsInTheirOrder)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DAT-013")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dat013AListReadsItsElementsInTheirOrder)
  {
    const Neuron::JsonDocument document =
      Parsed(R"({"schema": "a.b", "version": "1.0", "glyphs": [{"cp": 65}, {"cp": 66, "extra": 1}], "ns": [3, 4], "face": "font.x"})");
    Neuron::RecordReader reader(document.file);
    Neuron::RecordObject record = reader.Record(document.root, "a.b", {1, 0});
    const Neuron::RecordList glyphs = reader.List(record, "glyphs", 1, Neuron::LIST_NO_MAXIMUM);
    ExpectEqual(2, static_cast<std::int64_t>(glyphs.Size()), L"two elements");
    std::vector<std::int64_t> codePoints;
    for (std::size_t i = 0; i < glyphs.Size(); ++i)
    {
      Neuron::RecordObject glyph = reader.Element(glyphs, i);
      codePoints.push_back(reader.Integer(glyph, "cp", 32, 1114111));
      reader.Close(glyph);
    }
    Assert::IsTrue(codePoints == std::vector<std::int64_t>{65, 66}, L"in their authored order");
    const Neuron::RecordList numbers = reader.List(record, "ns", 2, 2);
    ExpectEqual(4, reader.ElementInteger(numbers, 1, 0, 9), L"an integer element");
    ExpectText("font.x", reader.Text(record, "face", 64), L"a text");
    reader.Close(record);
    const std::vector<Neuron::ContentDiagnostic> found = reader.TakeDiagnostics();
    Assert::IsTrue(found.size() == 1 && Has(found, Neuron::DiagnosticClass::UnknownField, "glyphs[1].extra"),
                   L"an element's unknown field is found at its path");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dat009AListTextOrIntegerOutsideItsBoundsIsRefused)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DAT-009")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dat009AListTextOrIntegerOutsideItsBoundsIsRefused)
  {
    const Neuron::JsonDocument document =
      Parsed(R"({"schema": "a.b", "version": "1.0", "empty": [], "long": "abcdef", "big": 11, "notList": 1, "word": 2})");
    Neuron::RecordReader reader(document.file);
    Neuron::RecordObject record = reader.Record(document.root, "a.b", {1, 0});
    (void)reader.List(record, "empty", 1, Neuron::LIST_NO_MAXIMUM);
    (void)reader.Text(record, "long", 5);
    (void)reader.Integer(record, "big", 0, 10);
    (void)reader.List(record, "notList", 0, Neuron::LIST_NO_MAXIMUM);
    (void)reader.Text(record, "word", 5);
    reader.Close(record);
    const std::vector<Neuron::ContentDiagnostic> found = reader.TakeDiagnostics();
    Assert::IsTrue(Has(found, Neuron::DiagnosticClass::OutOfRange, "empty"), L"a list below its minimum");
    Assert::IsTrue(Has(found, Neuron::DiagnosticClass::TooLong, "long"), L"a text past its maximum, refused and not shortened");
    Assert::IsTrue(Has(found, Neuron::DiagnosticClass::OutOfRange, "big"), L"an integer above its maximum");
    Assert::IsTrue(Has(found, Neuron::DiagnosticClass::WrongType, "notList"), L"a number for a list");
    Assert::IsTrue(Has(found, Neuron::DiagnosticClass::WrongType, "word"), L"a number for a text");
  }
};

} // namespace NeuronCoreTests
