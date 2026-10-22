#include "pch.h"

#include "MessagePattern.h"
#include "TestSupport.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

namespace
{

Neuron::MessagePattern Parsed(std::string_view _text)
{
  auto pattern = Neuron::ParseMessage(_text);
  Assert::IsTrue(pattern.has_value(), L"a message of the subset");
  return pattern.value_or(Neuron::MessagePattern{});
}

bool Refused(std::string_view _text, std::string_view _what)
{
  const auto pattern = Neuron::ParseMessage(_text);
  return !pattern.has_value() && pattern.error().what.find(_what) != std::string::npos;
}

} // namespace

TEST_CLASS(MessagePatternTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Adr063PlainTextIsOneTextPart)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-227")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr063PlainTextIsOneTextPart)
  {
    const Neuron::MessagePattern pattern = Parsed("Stores full: 50% of the Workers wait.");
    Assert::AreEqual(std::size_t{1}, pattern.parts.size());
    Assert::IsTrue(pattern.parts[0].kind == Neuron::MessagePartKind::Text &&
                     pattern.parts[0].text == "Stores full: 50% of the Workers wait.",
                   L"a percent sign is text (DEC-102)");
    Assert::IsTrue(Parsed("").parts.empty(), L"the empty text has no parts");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr063ApostrophesQuoteAsIcuDefaultDoes)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-227")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr063ApostrophesQuoteAsIcuDefaultDoes)
  {
    Assert::IsTrue(Parsed("The steward's site").parts[0].text == "The steward's site", L"a lone apostrophe is text");
    Assert::IsTrue(Parsed("It''s").parts[0].text == "It's", L"a doubled apostrophe is one");
    Assert::IsTrue(Parsed("'{braces}' here").parts[0].text == "{braces} here", L"an apostrophe before { quotes to the next one");
    Assert::IsTrue(Parsed("'{it''s}'").parts[0].text == "{it's}", L"a doubled apostrophe inside quotation");
    Assert::IsTrue(Refused("'{open", "quotation"), L"a quotation no apostrophe ends");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ui227ArgumentsAreNamedAndOfTheSubsetsTypes)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-227")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DAT-208")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui227ArgumentsAreNamedAndOfTheSubsetsTypes)
  {
    const Neuron::MessagePattern pattern = Parsed("Stores {stores} of { capacity , number }, {share, number, integer}%");
    Assert::AreEqual(std::size_t{7}, pattern.parts.size(), L"three arguments among four texts");
    Assert::IsTrue(pattern.parts[1].kind == Neuron::MessagePartKind::Argument && pattern.parts[1].text == "stores", L"a plain argument");
    Assert::IsTrue(pattern.parts[3].kind == Neuron::MessagePartKind::Number && !pattern.parts[3].integer, L"spaces around its parts");
    Assert::IsTrue(pattern.parts[5].kind == Neuron::MessagePartKind::Number && pattern.parts[5].integer, L"the integer style");
    Assert::IsTrue(Refused("{0} Workers", "numbered argument"), L"a numbered argument");
    Assert::IsTrue(Refused("{when, date}", "argument type 'date'"), L"a type the string tables do not use");
    Assert::IsTrue(Refused("{share, number, percent}", "number style 'percent'"), L"a style they do not use");
    Assert::IsTrue(Refused("{}", "without a name"), L"an argument without a name");
    Assert::IsTrue(Refused("Stores {stores", "neither , nor }"), L"an argument no } ends");
    Assert::IsTrue(Refused("Stores}", "closes no argument"), L"a stray }");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr063APluralHoldsItsCasesOffsetAndPound)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-227")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr063APluralHoldsItsCasesOffsetAndPound)
  {
    const Neuron::MessagePattern pattern = Parsed("{count, plural, offset:1 =0 {none} one {# Worker} other {# Workers '#'}}");
    Assert::AreEqual(std::size_t{1}, pattern.parts.size());
    const Neuron::MessagePart& plural = pattern.parts[0];
    Assert::IsTrue(plural.kind == Neuron::MessagePartKind::Plural && plural.text == "count", L"a plural of count");
    Assert::AreEqual(1.0, plural.offset, L"its offset");
    Assert::AreEqual(std::size_t{3}, plural.cases.size());
    Assert::IsTrue(plural.cases[0].selector == "=0" && plural.cases[1].selector == "one" && plural.cases[2].selector == "other");
    Assert::IsTrue(plural.cases[1].parts[0].kind == Neuron::MessagePartKind::Pound, L"# is the number");
    const std::vector<Neuron::MessagePart>& other = plural.cases[2].parts;
    Assert::IsTrue(other.size() == 2 && other[1].kind == Neuron::MessagePartKind::Text && other[1].text == " Workers #",
                   L"a quoted # is text");
    Assert::IsTrue(Parsed("#1 site").parts[0].text == "#1 site", L"outside a plural, # is text");
    Assert::IsTrue(Refused("{count, plural, one {#}}", "lack other"), L"a plural without other");
    Assert::IsTrue(Refused("{count, plural, single {#} other {#}}", "no plural category"), L"a keyword no category");
    Assert::IsTrue(Refused("{count, plural, one {a} one {b} other {c}}", "given twice"), L"a case given twice");
    Assert::IsTrue(Refused("{count, plural, one {a} other {b}", "no } ends"), L"cases no } ends");
    Assert::IsTrue(Refused("{count, plural, one {a other {b}}", "no } ends"), L"a case no } ends");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr063SelectsAndOrdinalsNest)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-227")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr063SelectsAndOrdinalsNest)
  {
    const Neuron::MessagePattern pattern =
      Parsed("{kind, select, dwelling {{count, selectordinal, one {#st} other {#th}} dwelling} other {a structure #}}");
    const Neuron::MessagePart& select = pattern.parts[0];
    Assert::IsTrue(select.kind == Neuron::MessagePartKind::Select && select.cases.size() == 2, L"a select of two cases");
    Assert::IsTrue(select.cases[0].parts[0].kind == Neuron::MessagePartKind::SelectOrdinal, L"an ordinal inside it");
    Assert::IsTrue(select.cases[1].parts[0].text == "a structure #", L"# in a select's case is text");
    Assert::IsTrue(Refused("{kind, select, dwelling {a}}", "lack other"), L"a select without other");
    std::string deep;
    for (int n = 0; n < 20; ++n)
    {
      deep += "{k, select, other {";
    }
    deep += 'x';
    for (int n = 0; n < 20; ++n)
    {
      deep += "}}";
    }
    Assert::IsTrue(Refused(deep, "nested too deeply"), L"nesting is bounded");
    const std::vector<std::string> names = Neuron::MessageArgumentNames(pattern);
    Assert::IsTrue(names == std::vector<std::string>{"kind", "count"}, L"its arguments, each once, in order");
  }
};

} // namespace NeuronCoreTests
