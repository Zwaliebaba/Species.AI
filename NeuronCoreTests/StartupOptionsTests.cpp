#include "pch.h"

#include "Development.h"
#include "StartupOptions.h"
#include "TestSupport.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

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
