#include "pch.h"

#include "Development.h"
#include "MessageFormat.h"
#include "MessageLocale.h"
#include "StringTable.h"
#include "TestSupport.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <string_view>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronClientTests
{

namespace
{

Neuron::StringTableAsset SmallTable()
{
  Neuron::StringTableAsset table;
  table.language = "en-GB";
  table.entries = {
    {"ui.hint.order", "Click to order", "Press A to order", false},
    {"ui.hud.workers", "{count, plural, one {# Worker} other {# Workers}}", "{count, plural, one {# Worker} other {# Workers}}", false},
    {"ui.title.game_name", "Species", "Species", true},
  };
  std::ranges::sort(table.entries, {}, &Neuron::StringEntry::id);
  return table;
}

} // namespace

TEST_CLASS(StringTableTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Ui218ALookUpReturnsTheTextOfItsMode)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-218")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-223")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui218ALookUpReturnsTheTextOfItsMode)
  {
    const Neuron::StringTable table(SmallTable());
    Assert::IsTrue(table.Language() == "en-GB");
    Assert::IsTrue(table.Lookup("ui.hint.order") == "Click to order", L"keyboard and mouse by default");
    Assert::IsTrue(table.Lookup("ui.hint.order", Neuron::InputMode::Gamepad) == "Press A to order", L"the gamepad's variant");
    Assert::IsTrue(table.Exists("ui.hint.order") && table.Exists("ui.hud.workers"), L"UI-220");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ui219AMissShowsItsIdOnlyInADebugBuild)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-219")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DAT-211")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui219AMissShowsItsIdOnlyInADebugBuild)
  {
    // ADR-146: the M2 plan's T14 shows a string not yet written as its id in a Debug build; a player sees nothing.
    const Neuron::StringTable table(SmallTable());
#if ENGINE_DEVELOPMENT
    const std::string_view expected = "ui.hud.stores";
#else
    const std::string_view expected;
#endif
    Assert::IsTrue(table.Lookup("ui.hud.stores") == expected, L"an id the table lacks");
    Assert::IsTrue(table.Lookup("ui.hud.stores") == expected, L"again, logged once");
    Assert::IsFalse(table.Exists("ui.hud.stores"));
    Assert::IsFalse(table.Exists("ui.title.game_name"), L"an embed-only string is never looked up");
    const Neuron::StringTable empty;
    Assert::IsTrue(empty.Lookup("ui.hint.order") == (expected.empty() ? std::string_view() : std::string_view("ui.hint.order")),
                   L"an empty table misses every id");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ui227AStringIsFormattedWithItsArguments)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-227")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui227AStringIsFormattedWithItsArguments)
  {
    const Neuron::StringTable table(SmallTable());
    const Neuron::MessageLocale locale("en-GB");
    const std::array one{Neuron::FormatArgument{"count", std::int64_t{1}}};
    const std::array many{Neuron::FormatArgument{"count", std::int64_t{2400}}};
    Assert::AreEqual(std::string("1 Worker"), table.Format("ui.hud.workers", one, locale));
    Assert::AreEqual(std::string("2,400 Workers"), table.Format("ui.hud.workers", many, locale));
    Assert::AreEqual(std::string(table.Lookup("ui.hud.idle")), table.Format("ui.hud.idle", one, locale), L"a miss is the look-up's");
    Assert::AreEqual(std::string("strings/en_gb"), Neuron::StringTableKey("en-GB"), L"the table's content key");
  }
};

} // namespace NeuronClientTests
