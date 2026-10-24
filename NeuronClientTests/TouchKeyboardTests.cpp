#include "pch.h"

#include "TouchKeyboard.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronClientTests
{

namespace
{

using Neuron::TouchKeyboardRequest;
using Neuron::TouchKeyboardRequestOf;

} // namespace

TEST_CLASS(TouchKeyboardTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Adr181TheKeyboardShowsForATextFieldEditedByTouch)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-100")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr181TheKeyboardShowsForATextFieldEditedByTouch)
  {
    // The arguments: whether a field was being edited, whether one is, touch mode, a tap.
    Assert::IsTrue(TouchKeyboardRequestOf(false, true, true, true) == TouchKeyboardRequest::Show, L"a tap starts the editing");
    Assert::IsTrue(TouchKeyboardRequestOf(false, true, false, false) == TouchKeyboardRequest::None,
                   L"a click or an activation starts it: no keyboard");
    Assert::IsTrue(TouchKeyboardRequestOf(true, true, true, false) == TouchKeyboardRequest::None, L"asked once, not every frame");
    Assert::IsTrue(TouchKeyboardRequestOf(true, true, true, true) == TouchKeyboardRequest::Show,
                   L"a tap while it edits brings back a keyboard closed by its own key");
    Assert::IsTrue(TouchKeyboardRequestOf(true, true, false, false) == TouchKeyboardRequest::None,
                   L"typing on it leaves touch mode, and it stays");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr181TheKeyboardHidesAsTheEditingEnds)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-100")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr181TheKeyboardHidesAsTheEditingEnds)
  {
    Assert::IsTrue(TouchKeyboardRequestOf(true, false, true, true) == TouchKeyboardRequest::Hide, L"a tap elsewhere applies the edit");
    Assert::IsTrue(TouchKeyboardRequestOf(true, false, false, false) == TouchKeyboardRequest::Hide, L"Enter or Escape ends it");
    Assert::IsTrue(TouchKeyboardRequestOf(false, false, true, true) == TouchKeyboardRequest::None, L"no field: nothing");
  }
};

} // namespace NeuronClientTests
