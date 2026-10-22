#include "pch.h"

#include "Files.h"
#include "SettingsFile.h"
#include "SettingsStore.h"
#include "TestSupport.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

namespace
{

std::vector<Neuron::SettingSpec> Specs()
{
  std::vector<Neuron::SettingSpec> specs;
  specs.push_back({"render.low_latency", Neuron::SettingType::Boolean, false, 0.0, 0.0, {}});
  specs.push_back({"render.frame_rate_cap", Neuron::SettingType::Integer, std::int64_t{0}, 0.0, 1000.0, {}});
  specs.push_back({"interface.ui_scale", Neuron::SettingType::Decimal, 1.0, 0.5, 3.0, {}});
  specs.push_back({"render.window_mode",
                   Neuron::SettingType::Enumeration,
                   std::string("borderless_full_screen"),
                   0.0,
                   0.0,
                   {"windowed", "borderless_full_screen"}});
  specs.push_back({"render.render_width", Neuron::SettingType::Integer, std::nullopt, 1.0, 65535.0, {}});
  specs.push_back({"binding.", Neuron::SettingType::Binding, std::nullopt, 0.0, 0.0, {}});
  return specs;
}

void WriteText(const std::filesystem::path& _path, std::string_view _text)
{
  const auto written = Neuron::ReplaceFileAtomically(_path, std::span(reinterpret_cast<const std::byte*>(_text.data()), _text.size()));
  Assert::IsTrue(written.has_value(), L"the test file is written");
}

} // namespace

TEST_CLASS(SettingsStoreTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dat198AnInvalidEntryTakesItsDefaultAndTheRestKeepTheirs)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DAT-198")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DAT-199")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DAT-200")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-079")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dat198AnInvalidEntryTakesItsDefaultAndTheRestKeepTheirs)
  {
    TemporaryFolder folder;
    const std::filesystem::path file = folder.Path() / L"settings.txt";
    WriteText(file, "\xEF\xBB\xBFrender.low_latency = true\r\n"
                    "render.frame_rate_cap = 30.5\n"
                    "interface.ui_scale = 2\n"
                    "render.window_mode = windowed\n"
                    "render.window_mode = borderless_full_screen\n"
                    "render.render_width = 1920\n"
                    "binding.camera_pan_left = {\"key\":\"A\"}\n"
                    "no equals sign\n"
                    "render.low_latency_x = kept\n"
                    "render.frame_rate_cap");
    const Neuron::SettingsStore store(Specs(), file);
    Assert::IsTrue(store.LoadedFromFile(), L"the file was read");
    Assert::IsTrue(store.GetBool("render.low_latency"), L"a byte-order mark and a carriage return are accepted");
    ExpectEqual(0, store.GetInteger("render.frame_rate_cap"), L"an integer written with a point is malformed and takes its default");
    Assert::IsTrue(store.GetDecimal("interface.ui_scale") == 2.0, L"a decimal written as a whole number reads as that decimal");
    ExpectText("windowed", store.GetText("render.window_mode"), L"the first occurrence of a key wins");
    ExpectEqual(1920, store.GetInteger("render.render_width"), L"an absent default is replaced by the stored value");
    ExpectText("{\"key\":\"A\"}", store.GetText("binding.camera_pan_left"), L"a family's key is typed by its family");
    Assert::IsFalse(store.Get("render.low_latency_x").has_value(), L"a key the schema does not hold has no value");
    Assert::IsTrue(store.KeysWithPrefix("binding.") == std::vector<std::string>{"binding.camera_pan_left"}, L"the bindings set");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dat201ASaveKeepsEveryLineInItsPlace)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DAT-201")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-081")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-089")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dat201ASaveKeepsEveryLineInItsPlace)
  {
    TemporaryFolder folder;
    const std::filesystem::path file = folder.Path() / L"settings" / L"settings.txt";
    std::filesystem::create_directories(file.parent_path());
    WriteText(file, "# my settings\n"
                    "\n"
                    "render.window_mode = windowed\n"
                    "future.setting = 7 apples\n"
                    "render.window_mode = borderless_full_screen\n"
                    "this line is kept\n"
                    "render.low_latency = false\n");
    Neuron::SettingsStore store(Specs(), file);
    Assert::IsTrue(store.Set("render.window_mode", std::string("borderless_full_screen")), L"a valid value");
    Assert::IsTrue(store.Set("render.frame_rate_cap", std::int64_t{144}), L"a key the file did not hold");
    Assert::IsTrue(store.Set("interface.ui_scale", 1.25), L"another");
    Assert::IsTrue(store.Set("binding.camera_pan_left", std::string("{\"key\":\"A\"}")), L"a binding");
    Assert::IsTrue(store.Save().has_value(), L"the save succeeds");
    ExpectText("# my settings\n"
               "\n"
               "render.window_mode = borderless_full_screen\n"
               "future.setting = 7 apples\n"
               "this line is kept\n"
               "render.low_latency = false\n"
               "binding.camera_pan_left = {\"key\":\"A\"}\n"
               "interface.ui_scale = 1.25\n"
               "render.frame_rate_cap = 144\n",
               ReadText(file), L"comments, blanks and unknown lines in place, each loaded key once, then the new ones in key order");

    // A setting set back to its default keeps the line it has now, and one the file never held is not written.
    Assert::IsTrue(store.Set("render.frame_rate_cap", std::int64_t{0}), L"back to the default");
    Assert::IsTrue(store.Clear("binding.camera_pan_left"), L"a binding cleared");
    Assert::IsTrue(store.Save().has_value(), L"the second save succeeds");
    ExpectContains(ReadText(file), "render.frame_rate_cap = 0\n", L"the line stays, with the default");
    Assert::IsTrue(ReadText(file).find("binding.camera_pan_left") == std::string::npos, L"a cleared binding's line goes");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt034SetNotifiesThePrefixesSubscribersOnAChange)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-034")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DAT-197")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt034SetNotifiesThePrefixesSubscribersOnAChange)
  {
    Neuron::SettingsStore store(Specs(), std::nullopt);
    std::vector<std::string> calls;
    const auto renderId = store.Subscribe("render.", [&](std::string_view _key) { calls.push_back("render:" + std::string(_key)); });
    store.Subscribe("", [&](std::string_view _key) { calls.push_back("all:" + std::string(_key)); });
    Assert::IsTrue(store.Set("render.low_latency", true), L"a change");
    Assert::IsTrue(store.Set("render.low_latency", true), L"the same value");
    Assert::IsTrue(store.Set("interface.ui_scale", 1.5), L"another prefix");
    Assert::IsTrue((calls == std::vector<std::string>{"render:render.low_latency", "all:render.low_latency", "all:interface.ui_scale"}),
                   L"in subscription order, matching prefixes only, and nobody for an equal value");
    store.Unsubscribe(renderId);
    calls.clear();
    Assert::IsTrue(store.Set("render.low_latency", false), L"a change after unsubscribing");
    Assert::IsTrue(calls == std::vector<std::string>{"all:render.low_latency"}, L"the unsubscribed is not called");

    Assert::IsFalse(store.Set("interface.ui_scale", 9.0), L"out of range is refused");
    Assert::IsFalse(store.Set("render.window_mode", std::string("exclusive")), L"an unknown enumerator is refused");
    Assert::IsFalse(store.Set("render.window_mode", true), L"another type is refused");
    Assert::IsFalse(store.Set("unknown.key", true), L"a key the schema does not hold is refused");
    Assert::IsFalse(store.Set("binding.camera_pan_left", std::string("a\nb")), L"DAT-197: a text cannot hold a line break");
    Assert::IsTrue(store.GetDecimal("interface.ui_scale") == 1.5, L"a refused value changes nothing");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt018WithoutAFileTheSettingsLiveInMemory)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-018")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-080")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-033")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt018WithoutAFileTheSettingsLiveInMemory)
  {
    TemporaryFolder folder;
    const Neuron::SettingsStore firstRun(Specs(), folder.Path() / L"settings.txt");
    Assert::IsFalse(firstRun.LoadedFromFile(), L"PLT-080: a missing file is a first run");
    ExpectText("borderless_full_screen", firstRun.GetText("render.window_mode"), L"with the defaults table's values");
    Assert::IsFalse(firstRun.Get("render.render_width").has_value(), L"and an absent default absent");

    Neuron::SettingsStore memory(Specs(), std::nullopt);
    Assert::IsTrue(memory.Set("render.low_latency", true), L"a change is kept in memory");
    Assert::IsTrue(memory.GetBool("render.low_latency"), L"and read back");
    Assert::IsFalse(memory.Save().has_value(), L"every save reports its failure");

    // PLT-033: reading with another type is a development check, and reads as the type's zero.
    ExpectEqual(0, memory.GetInteger("render.low_latency"), L"a boolean read as an integer");
    Assert::IsFalse(TakeDevelopmentChecks().empty(), L"is a development check");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dat197ValuesAreWrittenInTheFormOfTheirType)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DAT-197")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DAT-195")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dat197ValuesAreWrittenInTheFormOfTheirType)
  {
    ExpectText("true", Neuron::FormatSettingValue(true), L"a boolean");
    ExpectText("-12", Neuron::FormatSettingValue(std::int64_t{-12}), L"an integer");
    ExpectText("0.1", Neuron::FormatSettingValue(0.1), L"a decimal in its shortest round-trip form");
    ExpectText("2", Neuron::FormatSettingValue(2.0), L"a whole decimal");
    ExpectText("50% {x}", Neuron::FormatSettingValue(std::string("50% {x}")), L"a text literally");
    Assert::IsTrue(Neuron::IsSettingKey("render.low_latency"), L"a key");
    Assert::IsFalse(Neuron::IsSettingKey("Render.Low"), L"upper case is not");
    Assert::IsFalse(Neuron::IsSettingKey(std::string(Neuron::SETTING_KEY_CHARS_MAX + 1, 'a')), L"nor an over-long key");
    const std::vector<Neuron::SettingSpec> specs = Specs();
    const Neuron::SettingSpec* integer = Neuron::FindSettingSpec(specs, "render.frame_rate_cap");
    Assert::IsTrue(integer != nullptr, L"the spec");
    Assert::IsFalse(Neuron::ParseSettingValue(*integer, "+5").has_value(), L"no plus sign");
    Assert::IsFalse(Neuron::ParseSettingValue(*integer, "1001").has_value(), L"nothing above the range");
    Assert::IsTrue(Neuron::FindSettingSpec(specs, "binding.") == nullptr, L"a family's own prefix is no key");
  }
};

} // namespace NeuronCoreTests
