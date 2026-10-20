#include "pch.h"

#include "ContentKey.h"
#include "TestSupport.h"

#include <string>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

TEST_CLASS(ContentKeyTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ast014AKeyIsTheSourcePathLowerCasedWithoutItsExtension)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AST-014")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ast014AKeyIsTheSourcePathLowerCasedWithoutItsExtension)
  {
    ExpectText("textures/sprite/workerfront", Neuron::KeyOfSource("Textures/Sprite/WorkerFront.dds"), L"a texture's key");
    ExpectText("records/tuning/mvp", Neuron::KeyOfSource("Records/tuning/mvp.json"), L"a record's key");
    ExpectText("fonts/editorfontglyphs", Neuron::KeyOfSource("Fonts/EditorFontGlyphs.json"), L"the second member of a face");
    ExpectText("meshes/noextension", Neuron::KeyOfSource("Meshes/NoExtension"), L"a file without an extension");
    ExpectText("a.b/c", Neuron::KeyOfSource("A.B/C.dds"), L"only the last segment's extension goes");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ast015ARequestIsCanonicalisedAndAKeyIsChecked)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AST-015")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ast015ARequestIsCanonicalisedAndAKeyIsChecked)
  {
    ExpectText("records/tuning/mvp", Neuron::CanonicalKey("Records\\Tuning\\MVP"), L"case and backslashes");
    Assert::IsTrue(Neuron::IsCanonicalKey("textures/marker/crate_landing@shadow"), L"the key character set");
    Assert::IsFalse(Neuron::IsCanonicalKey("Textures/x"), L"an upper-case letter");
    Assert::IsFalse(Neuron::IsCanonicalKey("textures/a b"), L"a space");
    Assert::IsFalse(Neuron::IsCanonicalKey("textures//x"), L"an empty segment");
    Assert::IsFalse(Neuron::IsCanonicalKey("/x"), L"a leading slash");
    Assert::IsFalse(Neuron::IsCanonicalKey(""), L"an empty key");
    Assert::IsTrue(Neuron::IsCanonicalKey(std::string(Neuron::MAX_KEY_BYTES, 'a')), L"ast.pack.max_key_bytes bytes");
    Assert::IsFalse(Neuron::IsCanonicalKey(std::string(Neuron::MAX_KEY_BYTES + 1, 'a')), L"one byte more");
  }
};

} // namespace NeuronCoreTests
