#include "pch.h"

#include "ClientSettings.h"
#include "FrontEnd.h"
#include "GameUi.h"
#include "MessageLocale.h"
#include "SettingsStore.h"
#include "StringTable.h"
#include "TestSupport.h"
#include "UiCore.h"
#include "UiLook.h"

#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameClientTests
{

namespace
{

/// The front end's pages in a UI core of their own, with an empty string table, whose look-ups give the ids, and a
/// settings store with no file.
struct FrontEndHarness
{
  const Neuron::StringTable strings;
  const Neuron::MessageLocale locale{"en-US"};
  Neuron::SettingsStore settings{Species::ClientSettingSpecs(), std::nullopt};
  Species::FrontEnd frontEnd{DescOf(*this)};
  Neuron::UiCore ui{[this](Neuron::UiPageId _page) -> std::unique_ptr<Neuron::UiPage>
                    {
                      const auto first = static_cast<Neuron::UiPageId>(Species::GamePage::FrontEndMain);
                      if (_page >= first && _page <= static_cast<Neuron::UiPageId>(Species::GamePage::FrontEndPastRuns))
                      {
                        return frontEnd.MakePage(static_cast<Species::FrontEndPage>(_page - first));
                      }
                      return nullptr;
                    }};
  Neuron::UiMetrics metrics = Species::UiMetricsOf(CommittedPresentation(), {1280, 720}, 1.0f, std::nullopt, std::nullopt);

  static Species::FrontEnd::Desc DescOf(FrontEndHarness& _harness)
  {
    Species::FrontEnd::Desc desc;
    desc.presentation = &CommittedPresentation();
    desc.tuning = &CommittedTuning();
    desc.strings = &_harness.strings;
    desc.locale = &_harness.locale;
    desc.settings = &_harness.settings;
    return desc;
  }

  void Open(Species::GamePage _page = Species::GamePage::FrontEndMain)
  {
    ui.OpenScreen(static_cast<Neuron::UiPageId>(Species::GamePage::FrontEndMain));
    if (_page != Species::GamePage::FrontEndMain)
    {
      ui.Request({Neuron::UiPageRequest::Kind::Push, static_cast<Neuron::UiPageId>(_page)});
    }
    Frame({});
  }
  void Frame(const Neuron::UiInput& _input)
  {
    ui.Update(_input, metrics);
  }
  void Activate()
  {
    Neuron::UiInput input;
    input.activate = true;
    Frame(input);
  }
  void Down()
  {
    Neuron::UiInput input;
    input.down = true;
    Frame(input);
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
    Frame(input);
    input.pointerDown = false;
    input.pointerUp = true;
    Frame(input);
  }
  [[nodiscard]] bool CursorOn(std::string_view _id) const
  {
    const Neuron::UiWindow* window = ui.Focused();
    return window != nullptr && window->Cursor() == window->Find(_id);
  }
};

} // namespace

TEST_CLASS(FrontEndTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp063PastRunsOpenTheEpilogueOfAnEndedWorld)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-063")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-372")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp063PastRunsOpenTheEpilogueOfAnEndedWorld)
  {
    // ADR-170: Past runs is inactive until a run has ended; its page lists the ended worlds, each choosing its epilogue.
    FrontEndHarness none;
    none.Open();
    none.Click("past_runs");
    none.Frame({});
    Assert::AreEqual(std::size_t{1}, none.ui.Stack().size(), L"no ended run, no page");

    FrontEndHarness h;
    std::vector<Species::SavedWorld> ended(2);
    ended[0].number = 5;
    ended[0].seed = 11;
    ended[0].ended = true;
    ended[1].number = 2;
    ended[1].ended = true;
    h.frontEnd.SetEnded(ended);
    h.Open();
    h.Click("past_runs");
    h.Frame({});
    Assert::AreEqual(std::size_t{2}, h.ui.Stack().size(), L"the past runs' page");
    Assert::IsTrue(Neuron::ValidateWindow(*h.ui.Focused()).empty(), L"UI-372: the page passes the validator");
    Assert::IsTrue(h.CursorOn("world_0"), L"the newest first");
    h.Click("world_1");
    const std::optional<Species::FrontEndChoice> choice = h.frontEnd.TakeChoice();
    Assert::IsTrue(choice.has_value() && choice->kind == Species::FrontEndChoice::Kind::Epilogue &&
                     choice->world.value_or(Species::SavedWorld{}).number == 2,
                   L"a world chosen opens its epilogue");
    h.Click("back");
    h.Frame({});
    Assert::AreEqual(std::size_t{1}, h.ui.Stack().size(), L"Back returns to the main page");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp103TheMainPageContinuesOnlyAWorldThatCanBe)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-103")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-372")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp103TheMainPageContinuesOnlyAWorldThatCanBe)
  {
    FrontEndHarness h;
    h.Open();
    Assert::IsTrue(Neuron::ValidateWindow(*h.ui.Focused()).empty(), L"UI-372: the page passes the validator");
    Assert::IsTrue(h.CursorOn("new_world"), L"with no world, the cursor starts on New world");
    h.Click("continue");
    Assert::IsFalse(h.frontEnd.TakeChoice().has_value(), L"and Continue is inactive");
    h.Click("new_world");
    h.Frame({}); // UI-351: the page opens at the next frame
    Assert::AreEqual(std::size_t{2}, h.ui.Stack().size(), L"New world opens its page");

    FrontEndHarness latest;
    Species::SavedWorld world;
    world.number = 3;
    world.seed = 42;
    latest.frontEnd.SetLatest(world);
    latest.frontEnd.SetContinueFault(Species::ContinueFault::Damaged);
    latest.Open();
    Assert::IsTrue(latest.CursorOn("continue"), L"a world to continue takes the cursor");
    latest.Activate();
    const std::optional<Species::FrontEndChoice> choice = latest.frontEnd.TakeChoice();
    Assert::IsTrue(choice.has_value() && choice->kind == Species::FrontEndChoice::Kind::Continue, L"Continue chosen");
    Assert::IsFalse(latest.frontEnd.TakeChoice().has_value(), L"taken once");
    Assert::IsFalse(latest.frontEnd.LastFault().has_value(), L"a new choice clears the last fault");
    latest.Click("quit");
    const std::optional<Species::FrontEndChoice> quit = latest.frontEnd.TakeChoice();
    Assert::IsTrue(quit.has_value() && quit->kind == Species::FrontEndChoice::Kind::Quit, L"Quit chosen");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp005ANewWorldStartsFromTheSeedTypedOrADrawnOne)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-005")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-012")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp005ANewWorldStartsFromTheSeedTypedOrADrawnOne)
  {
    FrontEndHarness h;
    h.Open(Species::GamePage::FrontEndNewWorld);
    Assert::IsTrue(Neuron::ValidateWindow(*h.ui.Focused()).empty(), L"UI-372: the page passes the validator");
    Assert::IsTrue(h.CursorOn("seed"), L"the seed field first");
    // ADR-154: the field takes the typed digits and applies them on Enter.
    h.Activate();
    const std::vector<Neuron::TextCharacter> typed{{U'4'}, {U'2'}, {U'x'}, {U'\r'}};
    Neuron::UiInput input;
    input.text = typed;
    h.Frame(input);
    h.Down();
    h.Down();
    h.Down();
    h.Down();
    Assert::IsTrue(h.CursorOn("start"), L"the site, the neighbors, the pace, then Start");
    h.Activate();
    // No choice reads as Quit's default, so each check below fails without one.
    Species::FrontEndChoice choice = h.frontEnd.TakeChoice().value_or(Species::FrontEndChoice{});
    Assert::IsTrue(choice.kind == Species::FrontEndChoice::Kind::NewWorld, L"a new world");
    Assert::IsTrue(choice.seed == std::optional<std::uint64_t>(42), L"from the seed typed");
    ExpectEqual(CommittedTuning().mvp.sandbox.stewardSite, choice.site, L"at the default site");
    ExpectEqual(3, choice.aiSettlements, L"O103: with the three AI settlements by default (MVP-013)");
    Assert::IsTrue(choice.pace == Species::WorldPace::Session, L"ADR-191: at the session pace by default");

    // An empty field draws the seed; one beyond 64 bits starts nothing.
    Neuron::UiWindow& window = *h.ui.Focused();
    window.SetValue(window.Find("seed"), "");
    h.Click("site");
    h.Click("neighbors");
    h.Click("pace");
    h.Click("start");
    choice = h.frontEnd.TakeChoice().value_or(Species::FrontEndChoice{});
    Assert::IsTrue(choice.kind == Species::FrontEndChoice::Kind::NewWorld && !choice.seed.has_value(), L"no seed: the server draws one");
    ExpectEqual((CommittedTuning().mvp.sandbox.stewardSite + 1) % 4, choice.site, L"at the site stepped to");
    ExpectEqual(0, choice.aiSettlements, L"O103: and alone, for M2's growth run");
    Assert::IsTrue(choice.pace == Species::WorldPace::AlwaysOn, L"ADR-191: always on, as stepped to");
    window.SetValue(window.Find("seed"), "18446744073709551616");
    h.Click("start");
    Assert::IsFalse(h.frontEnd.TakeChoice().has_value(), L"a seed beyond 64 bits starts nothing");
    h.Click("back");
    h.Frame({});
    Assert::AreEqual(std::size_t{1}, h.ui.Stack().size(), L"Back returns to the main page");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(O096TheOptionsWriteTheSettingsAndFlagTheDisplay)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-103")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-030")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(O096TheOptionsWriteTheSettingsAndFlagTheDisplay)
  {
    FrontEndHarness h;
    const std::string mode = h.settings.GetText(Species::SETTING_WINDOW_MODE);
    const std::string antiAliasing = h.settings.GetText(Species::SETTING_ANTI_ALIASING);
    const std::string present = h.settings.GetText(Species::SETTING_PRESENT_MODE);
    h.settings.Set(Species::SETTING_MASTER_VOLUME, std::int64_t{Species::MASTER_VOLUME_MAX});
    h.Open(Species::GamePage::FrontEndOptions);
    Assert::IsTrue(Neuron::ValidateWindow(*h.ui.Focused()).empty(), L"UI-372: the page passes the validator");
    h.Activate();
    Assert::IsTrue(mode != h.settings.GetText(Species::SETTING_WINDOW_MODE), L"the window mode toggled");
    Assert::IsTrue(h.frontEnd.TakeDisplayChange(), L"and applied at once");
    h.Down();
    h.Activate();
    Assert::IsTrue(antiAliasing != h.settings.GetText(Species::SETTING_ANTI_ALIASING), L"anti-aliasing stepped");
    Assert::IsFalse(h.frontEnd.TakeDisplayChange(), L"for the next start");
    h.Down();
    h.Activate();
    Assert::IsTrue(present != h.settings.GetText(Species::SETTING_PRESENT_MODE), L"vertical sync toggled");
    Assert::IsTrue(h.frontEnd.TakeDisplayChange(), L"and applied at once");
    h.Down();
    h.Activate();
    ExpectEqual(0, h.settings.GetInteger(Species::SETTING_MASTER_VOLUME), L"from 100 % the volume wraps to 0 %");
    h.Activate();
    ExpectEqual(26, h.settings.GetInteger(Species::SETTING_MASTER_VOLUME), L"then 10 % of 255, rounded");
    Neuron::UiInput close;
    close.close = true;
    h.Frame(close);
    h.Frame({});
    Assert::AreEqual(std::size_t{1}, h.ui.Stack().size(), L"the close action leaves the page");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp005ASeedIsDigitsOfAtMost64Bits)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-005")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp005ASeedIsDigitsOfAtMost64Bits)
  {
    const auto empty = Species::SeedOfText("");
    Assert::IsTrue(empty.has_value() && !empty->has_value(), L"empty: drawn");
    const auto digits = Species::SeedOfText("0042");
    Assert::IsTrue(digits.has_value() && digits->value_or(0) == 42, L"digits");
    const auto largest = Species::SeedOfText("18446744073709551615");
    Assert::IsTrue(largest.has_value() && largest->value_or(0) == std::numeric_limits<std::uint64_t>::max(), L"the largest");
    Assert::IsFalse(Species::SeedOfText("18446744073709551616").has_value(), L"one more is refused");
    Assert::IsFalse(Species::SeedOfText("12a").has_value(), L"and so is a letter");
  }
};

} // namespace GameClientTests
