#include "pch.h"

#include "TestSupport.h"
#include "TextEncoding.h"

#include <string>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

TEST_CLASS(TextEncodingTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt088Utf8RoundTripsThroughUtf16)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-088")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt088Utf8RoundTripsThroughUtf16)
  {
    const std::string utf8 = "Gr\xC3\xBC\xC3\x9F"
                             "e, \xE4\xB8\x96\xE7\x95\x8C \xF0\x9F\x99\x82";
    const std::wstring utf16 = Neuron::ToUtf16(utf8);
    Assert::AreEqual(L"Gr\u00FC\u00DFe, \u4E16\u754C \U0001F642", utf16.c_str());
    ExpectText(utf8, Neuron::ToUtf8(utf16), L"round trip");
    Assert::IsTrue(Neuron::ToUtf16("").empty() && Neuron::ToUtf8(L"").empty(), L"empty text stays empty");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt088IllFormedTextBecomesTheReplacementCharacter)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-088")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt088IllFormedTextBecomesTheReplacementCharacter)
  {
    Assert::AreEqual(L"a\uFFFD", Neuron::ToUtf16("a\xC3").c_str());
    const std::wstring unpaired(1, static_cast<wchar_t>(0xD800));
    ExpectText("\xEF\xBF\xBD", Neuron::ToUtf8(unpaired), L"an unpaired surrogate");
  }
};

} // namespace NeuronCoreTests
