#include "pch.h"

#include "GameUi.h"
#include "MessageLocale.h"
#include "TestSupport.h"
#include "UiCore.h"
#include "UiLook.h"
#include "WorldMenu.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameClientTests
{

namespace
{

/// The world's menu and epilogue pages in a UI core of their own, with the committed strings.
struct WorldMenuHarness
{
  const Neuron::MessageLocale locale{"en-US"};
  Species::WorldMenu menu{DescOf(*this)};
  Neuron::UiCore ui{[this](Neuron::UiPageId _page) -> std::unique_ptr<Neuron::UiPage>
                    {
                      const auto first = static_cast<Neuron::UiPageId>(Species::GamePage::WorldMenu);
                      if (_page >= first && _page <= static_cast<Neuron::UiPageId>(Species::GamePage::WorldEpilogue))
                      {
                        return menu.MakePage(static_cast<Species::WorldMenuPage>(_page - first));
                      }
                      return nullptr;
                    }};
  Neuron::UiMetrics metrics = Species::UiMetricsOf(CommittedPresentation(), {1280, 720}, 1.0f, std::nullopt, std::nullopt);

  static Species::WorldMenu::Desc DescOf(WorldMenuHarness& _harness)
  {
    Species::WorldMenu::Desc desc;
    desc.presentation = &CommittedPresentation();
    desc.tuning = &CommittedTuning();
    desc.strings = &CommittedStrings();
    desc.locale = &_harness.locale;
    return desc;
  }

  void Open()
  {
    ui.OpenScreen(static_cast<Neuron::UiPageId>(Species::GamePage::WorldMenu));
    ui.Update({}, metrics);
  }
  void Close()
  {
    Neuron::UiInput input;
    input.close = true;
    ui.Update(input, metrics);
  }
  // A press and release on the widget _id of the focused window.
  void Click(std::string_view _id)
  {
    const Neuron::UiWindow* window = ui.Focused();
    const Neuron::UiRect rect = window->RectOf(window->Find(_id));
    Neuron::UiInput input;
    input.pointerX = (rect.left + rect.right) * 0.5f;
    input.pointerY = (rect.top + rect.bottom) * 0.5f;
    input.pointerMoved = true;
    input.pointerDown = true;
    ui.Update(input, metrics);
    input.pointerDown = false;
    input.pointerUp = true;
    ui.Update(input, metrics);
  }
  [[nodiscard]] Species::WorldMenuChoice::Kind Taken()
  {
    const std::optional<Species::WorldMenuChoice> choice = menu.TakeChoice();
    Assert::IsTrue(choice.has_value(), L"a choice");
    return choice.value_or(Species::WorldMenuChoice{}).kind;
  }
};

} // namespace

TEST_CLASS(WorldMenuTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp063TheEpilogueTellsTheStandingTheHistoryAndTheStewardsStatistics)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-063")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-125")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-224")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp063TheEpilogueTellsTheStandingTheHistoryAndTheStewardsStatistics)
  {
    Species::Epilogue epilogue;
    epilogue.team = 2;
    epilogue.runEnded = true;
    Species::EpilogueLine first;
    first.site = 2;
    first.renown = {2, 40, 25, 10, 5};
    first.battlesWon = 2;
    first.battlesLost = 1;
    first.claimsMade = 3;
    first.claimsWon = 1;
    first.structures = 14;
    first.grievances = 1;
    first.workersMost = 300;
    first.workersLast = 280;
    first.strengthMilliRu = {150'000, 420'000, 300'000};
    Species::EpilogueLine second;
    second.site = 0;
    second.renown = {0, 10, 0, 0, 2};
    epilogue.settlements = {first, second};
    epilogue.growthSteps = 12;
    epilogue.largestGapTicks = 9'000;   // 15 minutes of 0.1 s ticks
    epilogue.firstHalfwayTick = 75'000; // 2:05 into the run
    const Neuron::MessageLocale locale("en-US");
    const Species::EpilogueText text = Species::EpilogueTextOf(epilogue, 0.1, CommittedStrings(), locale);
    Assert::AreEqual(std::size_t{3}, text.sections.size(), L"the standing, then a history for each settlement");
    const Species::EpilogueSection& standing = text.sections[0];
    Assert::IsTrue(standing.title == "Standing" && standing.lines.size() == 4, L"MVP-062: two lines a settlement");
    Assert::IsTrue(standing.lines[0] == "1. Yellow: 80 renown" && standing.lines[1] == "40 defense, 25 claims, 10 easing, 5 building" &&
                     standing.lines[2].starts_with("2. "),
                   L"in the standing's order, with each total and its sources");
    const Species::EpilogueSection& own = text.sections[1];
    Assert::IsTrue(own.title == "Yellow (yours)" && own.lines.size() == 7, L"the steward's own history, led by its statistics");
    Assert::IsTrue(own.lines[0] == "12 growth steps, the longest gap 15 minutes", L"WLD-224: the growth steps");
    Assert::IsTrue(own.lines[1] == "First halfway line at 2:05", L"and when the territory first reached a halfway line");
    Assert::IsTrue(own.lines[2] == "Battles: 2 won, 1 lost" && own.lines[3] == "Claims: 3 made, 1 won, 0 lost, 0 withdrawn",
                   L"battles and claims");
    Assert::IsTrue(own.lines[4] == "14 structures raised, 1 grievance taken" && own.lines[5] == "Workers: 300 at most, 280 at the end",
                   L"structures, grievances and Workers");
    Assert::IsTrue(own.lines[6] == "Strength: 150 first, 420 peak, 300 last", L"its strength over the run, in whole resource units");
    Assert::IsTrue(text.sections[2].lines.size() == 5 && text.sections[2].lines[0] == "Battles: 0 won, 0 lost", L"another's history");
    // Every line fits the page's right box: at most 48 characters of the small font (ADR-170).
    for (const Species::EpilogueSection& section : text.sections)
    {
      for (const std::string& line : section.lines)
      {
        Assert::IsTrue(line.size() <= 48, L"a short line");
      }
    }
    epilogue.firstHalfwayTick = 0;
    Assert::IsTrue(Species::EpilogueTextOf(epilogue, 0.1, CommittedStrings(), locale).sections[1].lines[1] == "No halfway line reached",
                   L"or that it never did");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr170TheMenuReturnsLeavesOrQuitsAndWaitsForTheEpilogue)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-063")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-372")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-383")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr170TheMenuReturnsLeavesOrQuitsAndWaitsForTheEpilogue)
  {
    WorldMenuHarness h;
    h.Open();
    Assert::IsTrue(h.ui.Focused() != nullptr && Neuron::ValidateWindow(*h.ui.Focused()).empty(), L"UI-372: the menu passes the validator");
    const Neuron::UiWindow& window = *h.ui.Focused();
    Assert::IsTrue(window.Cursor() == window.Find("resume"), L"the cursor starts on Return to the world");
    h.Click("epilogue");
    h.ui.Update({}, h.metrics);
    Assert::AreEqual(std::size_t{1}, h.ui.Stack().size(), L"the epilogue waits for the run's end");
    Assert::IsFalse(h.menu.TakeChoice().has_value(), L"and is no choice");
    h.Click("resume");
    Assert::IsTrue(h.Taken() == Species::WorldMenuChoice::Kind::Resume, L"Return to the world");
    h.Close();
    Assert::IsTrue(h.Taken() == Species::WorldMenuChoice::Kind::Resume, L"UI-383: the root's close action returns too");
    h.Click("leave");
    Assert::IsTrue(h.Taken() == Species::WorldMenuChoice::Kind::Leave, L"Leave to the front end");
    h.Click("quit");
    Assert::IsTrue(h.Taken() == Species::WorldMenuChoice::Kind::Quit, L"Quit");
    Assert::IsFalse(h.menu.TakeChoice().has_value(), L"each taken once");
  }
};

} // namespace GameClientTests
