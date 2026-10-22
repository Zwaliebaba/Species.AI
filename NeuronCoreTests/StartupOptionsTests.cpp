#include "pch.h"

#include "Development.h"
#include "StartupOptions.h"
#include "TestSupport.h"

#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

namespace
{

Neuron::StartupOptions Parse(std::initializer_list<std::wstring_view> _arguments)
{
  const std::vector<std::wstring> arguments(_arguments.begin(), _arguments.end());
  return Neuron::ParseArguments(arguments);
}

bool IgnoredAre(const Neuron::StartupOptions& _options, std::initializer_list<std::string_view> _expected)
{
  return std::vector<std::string>(_expected.begin(), _expected.end()) == _options.ignoredArguments;
}

} // namespace

TEST_CLASS(StartupOptionsTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt022AnOptionIsAWholeArgumentAndTakesTheNext)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-022")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt022AnOptionIsAWholeArgumentAndTakesTheNext)
  {
    const std::vector<std::wstring> arguments{L"--verbose", L"--loose-content", L"D:\\cooked", L"--loose-contents"};
    const Neuron::StartupOptions options = Neuron::ParseArguments(arguments);
#if ENGINE_DEVELOPMENT
    Assert::IsTrue(options.looseContentDirectory == std::filesystem::path(L"D:\\cooked"), L"the option takes the next argument");
    ExpectEqual(2, static_cast<std::int64_t>(options.ignoredArguments.size()), L"the unknown arguments are ignored");
#else
    // PLT-026: a Release build does not compile the option in, so it is an unknown argument like any other.
    Assert::IsFalse(options.looseContentDirectory.has_value(), L"no loose-content option in Release");
    ExpectEqual(4, static_cast<std::int64_t>(options.ignoredArguments.size()), L"every argument is ignored");
#endif
    ExpectText("--verbose", options.ignoredArguments.front(), L"an ignored argument is kept as UTF-8, for the log");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt022TheSeedIsReadInAnyPosition)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-022")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-107")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-005")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt022TheSeedIsReadInAnyPosition)
  {
    // `--seed` takes the next argument as a decimal number of at most 64 bits; a Release build knows no `--seed`.
    const Neuron::StartupOptions options = Parse({L"--verbose", L"--seed", L"4294967296", L"extra"});
#if ENGINE_DEVELOPMENT
    Assert::IsTrue(options.worldSeed == std::uint64_t{4294967296}, L"the seed");
    Assert::IsTrue(IgnoredAre(options, {"--verbose", "extra"}), L"the ignored arguments, in order");
    Assert::IsTrue(Parse({L"--seed", L"18446744073709551615"}).worldSeed == std::numeric_limits<std::uint64_t>::max(), L"64 bits");
    Assert::IsTrue(Parse({L"--seed", L"0", L"--seed", L"9"}).worldSeed == std::uint64_t{9}, L"the last seed given");
#else
    Assert::IsFalse(options.worldSeed.has_value(), L"no seed in Release");
    Assert::IsTrue(IgnoredAre(options, {"--verbose", "--seed", "4294967296", "extra"}), L"every argument is ignored");
#endif
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt022ASeedWithoutANumberIsIgnored)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-022")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt022ASeedWithoutANumberIsIgnored)
  {
    // A value that is not a decimal number of at most 64 bits is ignored with the option, each as an argument of its own.
    for (const std::wstring_view value : {std::wstring_view{L"12a"}, std::wstring_view{L"-1"}, std::wstring_view{L""},
                                          std::wstring_view{L"18446744073709551616"}, std::wstring_view{L"0x10"}})
    {
      const Neuron::StartupOptions options = Parse({L"--seed", value});
      Assert::IsFalse(options.worldSeed.has_value(), L"no seed");
      ExpectEqual(2, static_cast<std::int64_t>(options.ignoredArguments.size()), L"both arguments ignored");
    }
    const Neuron::StartupOptions last = Parse({L"--seed"});
    Assert::IsTrue(!last.worldSeed.has_value() && IgnoredAre(last, {"--seed"}), L"an option with no value is ignored");
    const Neuron::StartupOptions spelled = Parse({L"--SEED", L"5", L"-seed", L"6"});
    Assert::IsTrue(!spelled.worldSeed.has_value() && spelled.ignoredArguments.size() == 4, L"only the exact spelling");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt022TheRenderCheckTakesItsFrameCount)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-022")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-107")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt022TheRenderCheckTakesItsFrameCount)
  {
    // ADR-113: `--render-check` takes a frame count from 1 to RENDER_CHECK_FRAMES_MAX; a Release build knows no such option.
    const Neuron::StartupOptions options = Parse({L"--seed", L"1", L"--render-check", L"30"});
#if ENGINE_DEVELOPMENT
    Assert::IsTrue(options.renderCheckFrames == std::uint32_t{30}, L"the frame count");
    Assert::IsTrue(options.ignoredArguments.empty());
    for (const std::wstring_view value : {std::wstring_view{L"0"}, std::wstring_view{L"10001"}, std::wstring_view{L"x"}})
    {
      const Neuron::StartupOptions refused = Parse({L"--render-check", value});
      Assert::IsTrue(!refused.renderCheckFrames.has_value() && refused.ignoredArguments.size() == 2, L"a count out of range is ignored");
    }
#else
    Assert::IsFalse(options.renderCheckFrames.has_value(), L"no render check in Release");
#endif
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt022TheWatchOptionTakesNoValue)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-022")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-107")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt022TheWatchOptionTakesNoValue)
  {
    // O77: `--watch` stands alone, so the argument after it is read on its own; a Release build knows no such option.
    const Neuron::StartupOptions options = Parse({L"--watch", L"--seed", L"4"});
#if ENGINE_DEVELOPMENT
    Assert::IsTrue(options.watch && options.worldSeed == std::uint64_t{4} && options.ignoredArguments.empty(), L"watched, with its seed");
#else
    Assert::IsFalse(options.watch, L"no watching in Release");
#endif
    Assert::IsFalse(Parse({L"--seed", L"4"}).watch, L"not watched unless asked");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt022TheWorldOptionTakesNoValue)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-022")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-107")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt022TheWorldOptionTakesNoValue)
  {
    // ADR-139: `--world` stands alone and starts a new world in place of the sandbox; a Release build knows no such option.
    const Neuron::StartupOptions options = Parse({L"--world", L"--seed", L"4"});
#if ENGINE_DEVELOPMENT
    Assert::IsTrue(options.world && options.worldSeed == std::uint64_t{4} && options.ignoredArguments.empty(), L"a world, with its seed");
#else
    Assert::IsFalse(options.world, L"no world option in Release");
#endif
    Assert::IsFalse(Parse({L"--seed", L"4"}).world, L"the sandbox unless asked");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt022TheUiTestOptionTakesNoValue)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-022")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-107")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt022TheUiTestOptionTakesNoValue)
  {
    // ADR-145: `--ui-test` stands alone and draws the UI's test page; a Release build knows no such option.
    const Neuron::StartupOptions options = Parse({L"--ui-test", L"--world"});
#if ENGINE_DEVELOPMENT
    Assert::IsTrue(options.uiTest && options.world && options.ignoredArguments.empty(), L"the test page, with a world");
#else
    Assert::IsFalse(options.uiTest, L"no test page option in Release");
#endif
    Assert::IsFalse(Parse({L"--world"}).uiTest, L"no test page unless asked");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ast105TheContentFolderIsBesideTheExecutableByDefault)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AST-105")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ast105TheContentFolderIsBesideTheExecutableByDefault)
  {
    const Neuron::StartupOptions none;
    Assert::IsTrue(Neuron::ContentDirectory(none).filename() == std::filesystem::path(Neuron::CONTENT_FOLDER), L"the content folder");
    Neuron::StartupOptions chosen;
    chosen.looseContentDirectory = std::filesystem::path(L"D:\\cooked");
    Assert::IsTrue(Neuron::ContentDirectory(chosen) == std::filesystem::path(L"D:\\cooked"), L"or the option's folder");
  }
};

} // namespace NeuronCoreTests
