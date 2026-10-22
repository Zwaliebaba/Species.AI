#include "pch.h"

#include "MessageFormat.h"
#include "MessageLocale.h"
#include "MessagePattern.h"
#include "TestSupport.h"

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronClientTests
{

namespace
{

// The message _text with _arguments inserted in en-US's forms, as the in-box ICU gives them.
std::string Formatted(std::string_view _text, std::span<const Neuron::FormatArgument> _arguments)
{
  static const Neuron::MessageLocale EN_US("en-US");
  const auto pattern = Neuron::ParseMessage(_text);
  Assert::IsTrue(pattern.has_value(), L"a message of the subset");
  return pattern ? Neuron::FormatPattern(*pattern, _arguments, EN_US) : std::string();
}

std::string Workers(std::int64_t _count)
{
  const std::array arguments{Neuron::FormatArgument{"count", _count}};
  return Formatted("{count, plural, =0 {No Workers} one {# Worker} other {# Workers}}", arguments);
}

} // namespace

TEST_CLASS(MessageFormatTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Adr063APluralTakesItsExactCaseThenItsCategory)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-227")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr063APluralTakesItsExactCaseThenItsCategory)
  {
    Assert::AreEqual(std::string("No Workers"), Workers(0), L"the exact case first");
    Assert::AreEqual(std::string("1 Worker"), Workers(1), L"en's one");
    Assert::AreEqual(std::string("2 Workers"), Workers(2), L"en's other");
    Assert::AreEqual(std::string("1,500 Workers"), Workers(1500), L"# in en-US's grouping");
    const std::array offset{Neuron::FormatArgument{"count", std::int64_t{2}}};
    Assert::AreEqual(std::string("you and 1 other"),
                     Formatted("{count, plural, offset:1 =1 {you} one {you and # other} other {you and # others}}", offset),
                     L"the category and # of the number less the offset");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr063NumbersTakeTheLanguagesForm)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-227")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr063NumbersTakeTheLanguagesForm)
  {
    const std::array arguments{Neuron::FormatArgument{"stores", std::int64_t{12345}}, Neuron::FormatArgument{"share", 2.5},
                               Neuron::FormatArgument{"rate", 1234.5678}};
    Assert::AreEqual(std::string("12,345 / 2.5 / 1,235 / 1,234.568"),
                     Formatted("{stores} / {share, number} / {rate, number, integer} / {rate}", arguments),
                     L"grouped, rounded in the integer style, and three fractional digits at most");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr063SelectAndOrdinalChooseTheirCases)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-227")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr063SelectAndOrdinalChooseTheirCases)
  {
    const std::string_view text = "{kind, select, dwelling {a dwelling} storehouse {a storehouse} other {a structure}}";
    const std::array dwelling{Neuron::FormatArgument{"kind", std::string_view("dwelling")}};
    const std::array turret{Neuron::FormatArgument{"kind", std::string_view("turret")}};
    Assert::AreEqual(std::string("a dwelling"), Formatted(text, dwelling));
    Assert::AreEqual(std::string("a structure"), Formatted(text, turret), L"other for a key no case names");
    const std::string_view ordinal = "{n, selectordinal, one {#st} two {#nd} few {#rd} other {#th}}";
    const auto nth = [&](std::int64_t _n)
    {
      const std::array arguments{Neuron::FormatArgument{"n", _n}};
      return Formatted(ordinal, arguments);
    };
    Assert::IsTrue(nth(1) == "1st" && nth(2) == "2nd" && nth(3) == "3rd" && nth(4) == "4th" && nth(11) == "11th" && nth(22) == "22nd",
                   L"en's ordinal categories");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ui227ATextArgumentIsInsertedLiterally)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-227")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-231")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui227ATextArgumentIsInsertedLiterally)
  {
    const std::array arguments{Neuron::FormatArgument{"file", std::string_view("{count} 100% 'x' #")}};
    Assert::AreEqual(std::string("Saved {count} 100% 'x' #."), Formatted("Saved {file}.", arguments), L"no character of it is syntax");
    Assert::AreEqual(std::string("Saved {file}."), Formatted("Saved {file}.", {}), L"an argument not given shows its name, as ICU does");
  }
};

} // namespace NeuronClientTests
