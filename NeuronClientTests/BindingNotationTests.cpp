#include "pch.h"

#include "BindingNotation.h"
#include "InputNames.h"
#include "TestSupport.h"

#include <array>
#include <string>
#include <string_view>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronClientTests
{

TEST_CLASS(BindingNotationTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Inp072EveryNodeOfTheTableReadsBackTheSame)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"INP-071")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"INP-072")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Inp072EveryNodeOfTheTableReadsBackTheSame)
  {
    // INP-071's table, each in its canonical form; the gamepad's row is MVP-100's refusal below.
    constexpr std::array CANONICAL{
      std::string_view{R"({"key": "a", "when": "held"})"},
      std::string_view{R"({"mouse": "primary", "when": "down"})"},
      std::string_view{R"({"all": [{"key": "alt", "when": "held"}, {"key": "enter", "when": "down"}]})"},
      std::string_view{R"({"chord": [{"key": "control"}, {"key": "c"}]})"},
      std::string_view{R"({"not": {"key": "shift", "when": "held"}})"},
      std::string_view{R"({"alias": "action.ui.show_cursor"})"},
      std::string_view{R"({"latch": {"on": {"key": "f2", "when": "down"}, "off": {"key": "escape", "when": "down"}}})"},
      std::string_view{R"({"idle": "at_least", "seconds": 60})"},
      std::string_view{R"({"idle": "reaches", "seconds": 60})"},
      std::string_view{R"({"idle": "no_longer"})"},
      std::string_view{R"({"setting": "interface.tips"})"},
      std::string_view{R"({"setting": "interface.tips", "negate": true})"},
      std::string_view{R"({"value": "always_true"})"},
      std::string_view{R"({"value": "feature", "flag": "tutorial"})"},
      std::string_view{R"({"mouse": "wheel", "when": "move"})"},
      std::string_view{R"({"mouse": "wheel_horizontal", "when": "move"})"},
      std::string_view{R"({"key": "num_minus", "when": "repeat"})"},
    };
    for (const std::string_view text : CANONICAL)
    {
      const auto binding = Neuron::ParseBinding(text);
      Assert::IsTrue(binding.has_value(), L"each row parses");
      ExpectText(text, Neuron::WriteBinding(*binding), L"and writes back as it was");
    }
    // INP-055: names are matched without regard to case, spaces carry no meaning, and the writer is canonical.
    const auto loose = Neuron::ParseBinding("{ \"when\":\"HELD\",  \"key\" : \"Page_Up\" }");
    Assert::IsTrue(loose.has_value(), L"any case and order");
    ExpectText(R"({"key": "page_up", "when": "held"})", Neuron::WriteBinding(*loose), L"canonical names and order");
    Assert::IsTrue(*loose == Neuron::KeyBinding(Neuron::KeyCode::PageUp, Neuron::Condition::Held), L"the same binding");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Inp073ANodeThatBreaksTheNotationIsRefused)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"INP-073")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"INP-172")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Inp073ANodeThatBreaksTheNotationIsRefused)
  {
    constexpr std::array REFUSED{
      std::string_view{R"({"key": "a"})"},                                // no condition
      std::string_view{R"({"key": "a", "when": "pressed"})"},             // an unknown condition word
      std::string_view{R"({"key": "aa", "when": "held"})"},               // an unknown key name
      std::string_view{R"({"key": "a", "when": "held", "speed": 2})"},    // an unknown property
      std::string_view{R"({"key": "a", "mouse": "primary"})"},            // two node-kind properties
      std::string_view{R"({"when": "held"})"},                            // no node-kind property
      std::string_view{R"({"pad": "face_south", "when": "down"})"},       // MVP-100: no gamepad
      std::string_view{R"({"chord": [{"key": "a", "when": "held"}]})"},   // a chord's part with a condition
      std::string_view{R"({"idle": "at_least"})"},                        // no seconds
      std::string_view{R"({"idle": "at_least", "seconds": 1.5})"},        // not whole
      std::string_view{R"({"latch": {"on": {"value": "always_true"}}})"}, // no off
      std::string_view{R"({"value": "feature"})"},                        // no flag
      std::string_view{R"([{"key": "a", "when": "held"}])"},              // not an object
      std::string_view{R"({"key": "a", "when": "held")"},                 // not JSON
    };
    for (const std::string_view text : REFUSED)
    {
      const auto binding = Neuron::ParseBinding(text);
      Assert::IsFalse(binding.has_value(), L"refused, never a default (DEC-096)");
      Assert::IsFalse(binding.error().empty(), L"with a message");
    }
    ExpectContains(Neuron::ParseBinding(R"({"all": [{"key": "a", "when": "held"}, {"key": "zz", "when": "down"}]})").error(), "$.all[1]",
                   L"the message names the node's path");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Inp055EveryKeyHasOneNameAndBack)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"INP-055")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"INP-056")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Inp055EveryKeyHasOneNameAndBack)
  {
    int named = 0;
    for (int code = 0; code < Neuron::KEY_CODE_COUNT; ++code)
    {
      const std::string_view name = Neuron::NameOfKey(static_cast<Neuron::KeyCode>(code));
      if (name.empty())
      {
        continue;
      }
      ++named;
      Assert::IsTrue(Neuron::KeyOfName(name) == static_cast<Neuron::KeyCode>(code), L"the name gives its key back");
    }
    ExpectEqual(113, named, L"table 3.2-A: 26 letters, 10 digits, 24 function keys and 53 others");
    Assert::IsTrue(Neuron::KeyOfName("NUM_SEPARATOR") == Neuron::KeyCode::NumSeparator, L"any case");
    Assert::IsTrue(Neuron::MouseControlOfName("raw_motion") == Neuron::MouseControl::RawMotion, L"the mouse's names");
    Assert::IsFalse(Neuron::ConditionOfName("pressed").has_value(), L"no other condition");
  }
};

} // namespace NeuronClientTests
