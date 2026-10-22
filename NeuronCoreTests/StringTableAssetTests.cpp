#include "pch.h"

#include "StringTableAsset.h"
#include "TestSupport.h"

#include <algorithm>
#include <cstddef>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

namespace
{

Neuron::StringTableAsset SmallTable()
{
  Neuron::StringTableAsset table;
  table.language = "en-GB";
  table.entries = {
    {"ui.hud.stores", "Stores {stores}", "Stores {stores}", false},
    {"ui.hint.order", "Click to order", "Press A to order", false},
    {"ui.title.game_name", "Species", "Species", true},
  };
  std::ranges::sort(table.entries, {}, &Neuron::StringEntry::id);
  return table;
}

} // namespace

TEST_CLASS(StringTableAssetTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Dat217ACookedTableReadsBackAsWritten)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DAT-217")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AST-091")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dat217ACookedTableReadsBackAsWritten)
  {
    const Neuron::StringTableAsset table = SmallTable();
    const std::vector<std::byte> bytes = Neuron::WriteStringTableAsset(table);
    const auto read = Neuron::ReadStringTableAsset(bytes);
    Assert::IsTrue(read.has_value(), L"it reads");
    const Neuron::StringTableAsset back = read.value_or(Neuron::StringTableAsset{});
    Assert::IsTrue(back.language == "en-GB" && back.entries.size() == 3, L"its language and strings");
    const Neuron::StringEntry* hint = Neuron::FindString(back, "ui.hint.order");
    Assert::IsTrue(hint != nullptr && hint->keyboardMouse == "Click to order" && hint->gamepad == "Press A to order", L"both modes");
    const Neuron::StringEntry* name = Neuron::FindString(back, "ui.title.game_name");
    Assert::IsTrue(name != nullptr && name->embedOnly, L"the embed-only flag");
    Assert::IsTrue(Neuron::FindString(back, "ui.hud") == nullptr && Neuron::FindString(back, "ui.title.game_name.x") == nullptr,
                   L"ids compare exactly");
    Assert::IsTrue(Neuron::WriteStringTableAsset(back) == bytes, L"the same bytes again");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dat217ADamagedTableIsRefused)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DAT-050")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dat217ADamagedTableIsRefused)
  {
    const std::vector<std::byte> bytes = Neuron::WriteStringTableAsset(SmallTable());
    std::vector<std::byte> version = bytes;
    version[0] = std::byte{9};
    Assert::IsFalse(Neuron::ReadStringTableAsset(version).has_value(), L"another version");
    const std::vector<std::byte> shortened(bytes.begin(), bytes.end() - 3);
    Assert::IsFalse(Neuron::ReadStringTableAsset(shortened).has_value(), L"an entry cut short");
    std::vector<std::byte> longer = bytes;
    longer.push_back(std::byte{0});
    Assert::IsFalse(Neuron::ReadStringTableAsset(longer).has_value(), L"bytes after the last string");
    Neuron::StringTableAsset unsorted = SmallTable();
    std::ranges::reverse(unsorted.entries);
    Assert::IsFalse(Neuron::ReadStringTableAsset(Neuron::WriteStringTableAsset(unsorted)).has_value(), L"ids out of order");
  }
};

} // namespace NeuronCoreTests
