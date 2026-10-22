#include "pch.h"

#include "TextSystem.h"
#include "UiCore.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronClientTests
{

namespace
{

constexpr Neuron::UiPageId TITLE = 1;
constexpr Neuron::UiPageId OPTIONS = 2;
constexpr Neuron::UiPageId CREDITS = 3;

/// What the test pages saw, shared with the test.
struct Record
{
  std::vector<std::string> entered;
  std::vector<std::string> laidOut;
  std::vector<std::string> clicked;
  std::vector<std::string> changed;
  std::vector<std::string> backs;
};

/// A test page: a column of rows on a 100-pixel grid, each row 40 px high and 10 px apart, starting at y = 100, with an
/// optional back action; its update records its events, and a click on "push" or "layer" opens the next page or a layer.
class TestPage : public Neuron::UiPage
{
public:
  TestPage(std::string _name, Record& _record, std::vector<Neuron::UiWidgetDesc> _rows, bool _rootIgnoresBack = false)
    : m_name(std::move(_name)),
      m_record(_record),
      m_rows(std::move(_rows)),
      m_rootIgnoresBack(_rootIgnoresBack)
  {
  }

  [[nodiscard]] std::string_view Name() const override
  {
    return m_name;
  }

  void Enter(Neuron::UiWindow& _window) override
  {
    m_record.entered.push_back(m_name);
    std::vector<Neuron::UiHandle> order;
    for (const Neuron::UiWidgetDesc& row : m_rows)
    {
      const Neuron::UiHandle handle = _window.Add(row);
      m_handles.push_back(handle);
      if (row.kind != Neuron::UiWidgetKind::Label)
      {
        order.push_back(handle);
      }
      if (row.id == "back")
      {
        _window.SetBackAction(handle);
      }
    }
    _window.SetNavigation(std::move(order));
  }

  void Layout(Neuron::UiWindow& _window, const Neuron::UiMetrics& _metrics) override
  {
    m_record.laidOut.push_back(m_name);
    const float width = static_cast<float>(_metrics.client.width) * 0.5f;
    for (std::size_t n = 0; n < m_handles.size(); ++n)
    {
      _window.SetRect(m_handles[n], Neuron::UiRect::FromSize(100.0f, 100.0f + 50.0f * static_cast<float>(n), width, 40.0f));
    }
  }

  void Update(Neuron::UiWindow& _window, std::span<const Neuron::UiEvent> _events, Neuron::UiNavigator& _navigator) override
  {
    for (const Neuron::UiEvent& event : _events)
    {
      const std::string id = IdOf(_window, event.widget);
      (event.kind == Neuron::UiEventKind::Click ? m_record.clicked : m_record.changed).push_back(m_name + ":" + id);
      if (event.kind != Neuron::UiEventKind::Click)
      {
        continue;
      }
      if (id == "push")
      {
        _navigator.Request({Neuron::UiPageRequest::Kind::Push, OPTIONS});
      }
      else if (id == "layer")
      {
        _navigator.OpenLayer(std::make_unique<TestPage>("layer", m_record, std::vector{Row("close")}));
      }
      else if (id == "close" || id == "back")
      {
        Back(_window, _navigator);
      }
    }
  }

  void Back(Neuron::UiWindow& _window, Neuron::UiNavigator& _navigator) override
  {
    m_record.backs.push_back(m_name);
    if (!m_rootIgnoresBack)
    {
      UiPage::Back(_window, _navigator);
    }
  }

  static Neuron::UiWidgetDesc Row(std::string _id, Neuron::UiWidgetKind _kind = Neuron::UiWidgetKind::PageButton, bool _inactive = false)
  {
    Neuron::UiWidgetDesc desc;
    desc.kind = _kind;
    desc.id = std::move(_id);
    desc.text = desc.id;
    desc.inactive = _inactive;
    if (_kind == Neuron::UiWidgetKind::Cycler)
    {
      desc.options = {"one", "two", "three"};
    }
    return desc;
  }

private:
  std::string IdOf(const Neuron::UiWindow& _window, Neuron::UiHandle _widget) const
  {
    for (std::size_t n = 0; n < m_handles.size(); ++n)
    {
      if (m_handles[n] == _widget && _window.Contains(_widget))
      {
        return m_rows[n].id;
      }
    }
    return {};
  }

  std::string m_name;
  Record& m_record;
  std::vector<Neuron::UiWidgetDesc> m_rows;
  std::vector<Neuron::UiHandle> m_handles;
  bool m_rootIgnoresBack = false;
};

/// The UI core with the test's three pages: the title (root, ignoring Back), the options page with a back action, and
/// the credits page without one.
struct Harness
{
  Record record;
  Neuron::UiCore ui{[this](Neuron::UiPageId _page) -> std::unique_ptr<Neuron::UiPage>
                    {
                      using Kind = Neuron::UiWidgetKind;
                      switch (_page)
                      {
                      case TITLE:
                        return std::make_unique<TestPage>("title", record,
                                                          std::vector{TestPage::Row("caption", Kind::Label), TestPage::Row("push"),
                                                                      TestPage::Row("idle", Kind::PageButton, true), TestPage::Row("layer"),
                                                                      TestPage::Row("check", Kind::CheckBox)},
                                                          true);
                      case OPTIONS:
                        return std::make_unique<TestPage>("options", record,
                                                          std::vector{TestPage::Row("cycle", Kind::Cycler), TestPage::Row("back")});
                      case CREDITS:
                        return std::make_unique<TestPage>("credits", record, std::vector{TestPage::Row("only")});
                      default:
                        return nullptr;
                      }
                    }};
  Neuron::UiMetrics metrics;

  Harness()
  {
    metrics.client = {1280, 720};
    ui.OpenScreen(TITLE);
    Frame({});
  }

  void Frame(const Neuron::UiInput& _input)
  {
    ui.Update(_input, metrics);
  }

  [[nodiscard]] std::string CursorId() const
  {
    const Neuron::UiWindow* window = ui.Focused();
    if (window == nullptr)
    {
      return {};
    }
    for (const char* id : {"push", "idle", "layer", "check", "cycle", "back", "only", "close"})
    {
      if (window->Find(id) == window->Cursor() && window->Cursor())
      {
        return id;
      }
    }
    return {};
  }

  // The center of the widget _id in the focused window.
  [[nodiscard]] Neuron::UiInput PointerOn(std::string_view _id) const
  {
    const Neuron::UiWindow* window = ui.Focused();
    const Neuron::UiRect rect = window->RectOf(window->Find(_id));
    Neuron::UiInput input;
    input.pointerX = (rect.left + rect.right) * 0.5f;
    input.pointerY = (rect.top + rect.bottom) * 0.5f;
    input.pointerMoved = true;
    return input;
  }
};

// The quads of _batches that read the image _image of the UI's set.
std::size_t ImageQuads(const std::vector<Neuron::UiBatch>& _batches, std::uint16_t _image)
{
  std::size_t count = 0;
  for (const Neuron::UiBatch& batch : _batches)
  {
    count += batch.source == Neuron::UiSource::Image && batch.image == _image ? batch.quads.size() : 0;
  }
  return count;
}

// Whether _batches hold a solid quad over _rect in _color.
bool HasSolid(const std::vector<Neuron::UiBatch>& _batches, const Neuron::UiRect& _rect, const std::array<float, 4>& _color)
{
  for (const Neuron::UiBatch& batch : _batches)
  {
    for (const Neuron::UiQuad& quad : batch.quads)
    {
      if (batch.source == Neuron::UiSource::Solid && quad.color == _color && quad.rectangle[0] == _rect.left &&
          quad.rectangle[1] == _rect.top && quad.rectangle[2] == _rect.right && quad.rectangle[3] == _rect.bottom)
      {
        return true;
      }
    }
  }
  return false;
}

Neuron::UiInput Keys(bool _down, bool _up = false, bool _activate = false, bool _close = false)
{
  Neuron::UiInput input;
  input.down = _down;
  input.up = _up;
  input.activate = _activate;
  input.close = _close;
  return input;
}

} // namespace

TEST_CLASS(UiCoreTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Ui331ThePageStackPushesPopsReplacesAndUnwinds)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-331")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui331ThePageStackPushesPopsReplacesAndUnwinds)
  {
    Harness h;
    Assert::IsTrue(std::vector<Neuron::UiPageId>(h.ui.Stack().begin(), h.ui.Stack().end()) == std::vector<Neuron::UiPageId>{TITLE},
                   L"UI-332: a new screen's stack is its root");
    h.ui.Request({Neuron::UiPageRequest::Kind::Pop, 0});
    h.Frame({});
    Assert::AreEqual(std::size_t{1}, h.ui.Stack().size(), L"a pop of the only page is refused");
    h.ui.Request({Neuron::UiPageRequest::Kind::Push, OPTIONS});
    h.Frame({});
    h.ui.Request({Neuron::UiPageRequest::Kind::Push, CREDITS});
    h.Frame({});
    Assert::AreEqual(std::size_t{3}, h.ui.Stack().size());
    h.ui.Request({Neuron::UiPageRequest::Kind::Push, OPTIONS});
    h.Frame({});
    Assert::AreEqual(std::size_t{2}, h.ui.Stack().size(), L"a push of a page on the stack unwinds to it");
    Assert::AreEqual(OPTIONS, h.ui.Stack().back());
    h.ui.Request({Neuron::UiPageRequest::Kind::Replace, CREDITS});
    h.Frame({});
    Assert::IsTrue(h.ui.Stack().size() == 2 && h.ui.Stack().back() == CREDITS, L"a replace swaps the top page");
    h.ui.Request({Neuron::UiPageRequest::Kind::UnwindTo, OPTIONS});
    h.Frame({});
    Assert::IsTrue(h.ui.Stack().size() == 2 && h.ui.Stack()[0] == TITLE && h.ui.Stack()[1] == OPTIONS,
                   L"unwinding to a page not on the stack gives the root and the page");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ui351ARequestTakesEffectAtTheNextFrameAndTheLastWins)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-351")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui351ARequestTakesEffectAtTheNextFrameAndTheLastWins)
  {
    Harness h;
    h.Frame(Keys(false, false, true)); // activates the cursor, "push", whose page requests the options page
    Assert::AreEqual(std::size_t{1}, h.ui.Stack().size(), L"not before the next frame");
    Assert::AreEqual(std::string("title"), std::string(h.ui.Focused()->Name()));
    h.ui.Request({Neuron::UiPageRequest::Kind::Push, CREDITS}); // UI-350: a later request of the same frame wins
    h.Frame({});
    Assert::AreEqual(CREDITS, h.ui.Stack().back());
    const std::size_t entered = h.record.entered.size();
    h.ui.Request({Neuron::UiPageRequest::Kind::Push, CREDITS});
    h.Frame({});
    Assert::AreEqual(entered, h.record.entered.size(), L"UI-355: a request for the current page does nothing");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ui051NavigationWrapsAndSkipsInactiveEntries)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-051")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui051NavigationWrapsAndSkipsInactiveEntries)
  {
    Harness h;
    Assert::AreEqual(std::string("push"), h.CursorId(), L"UI-385: the first entry has the initial focus");
    h.Frame(Keys(true));
    Assert::AreEqual(std::string("layer"), h.CursorId(), L"the inactive entry is skipped");
    h.Frame(Keys(true));
    h.Frame(Keys(true));
    Assert::AreEqual(std::string("push"), h.CursorId(), L"down from the last entry wraps to the first");
    h.Frame(Keys(false, true));
    Assert::AreEqual(std::string("check"), h.CursorId(), L"up from the first wraps to the last");
    h.Frame(Keys(false, false, true));
    Assert::IsTrue(h.ui.Focused()->Checked(h.ui.Focused()->Find("check")), L"UI-080: activation toggles a check box");
    Assert::IsTrue(h.record.changed == std::vector<std::string>{"title:check"}, L"and reports its change");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ui381AnAuthoredNeighborWinsOverTheOrder)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-381")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui381AnAuthoredNeighborWinsOverTheOrder)
  {
    Harness h;
    Neuron::UiWindow& window = *h.ui.Focused();
    window.SetNeighbors(window.Find("push"), {.up = {}, .down = window.Find("check"), .left = {}, .right = window.Find("layer")});
    h.Frame(Keys(true));
    Assert::AreEqual(std::string("check"), h.CursorId(), L"down goes to the authored neighbor");
    h.Frame(Keys(false, true));
    Assert::AreEqual(std::string("layer"), h.CursorId(), L"without one, the order applies");
    Neuron::UiInput right;
    right.right = true;
    h.Frame(right);
    Assert::AreEqual(std::string("layer"), h.CursorId(), L"left and right move only to authored neighbors");
    Assert::IsTrue(Neuron::ValidateWindow(window).empty(), L"the window passes the validator");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ui056HoverMovesTheCursorAndAClickNeedsTheReleaseOnTheWidget)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-056")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui056HoverMovesTheCursorAndAClickNeedsTheReleaseOnTheWidget)
  {
    Harness h;
    h.Frame(h.PointerOn("check"));
    Assert::AreEqual(std::string("check"), h.CursorId(), L"the pointer moving onto a widget makes it the cursor");
    Assert::IsTrue(h.ui.Focused()->Highlighted(h.ui.Focused()->Find("check")));
    h.Frame(Keys(false, true));
    Assert::AreEqual(std::string("layer"), h.CursorId(), L"navigation moves on from it");
    Assert::IsFalse(h.ui.Focused()->Highlighted(h.ui.Focused()->Find("check")), L"and ends hover-highlight mode");

    // UI-031, UI-035: a press on one widget and a release on another is no click.
    Neuron::UiInput press = h.PointerOn("check");
    press.pointerDown = true;
    h.Frame(press);
    Neuron::UiInput release = h.PointerOn("layer");
    release.pointerUp = true;
    h.Frame(release);
    Assert::IsTrue(h.record.clicked.empty(), L"no click");
    // A press and a release on the same widget, in one frame, is one.
    Neuron::UiInput click = h.PointerOn("check");
    click.pointerDown = true;
    click.pointerUp = true;
    h.Frame(click);
    Assert::IsTrue(h.record.clicked == std::vector<std::string>{"title:check"}, L"a click");
    // An inactive widget never acts.
    Neuron::UiInput idle = h.PointerOn("idle");
    idle.pointerDown = true;
    idle.pointerUp = true;
    h.Frame(idle);
    Assert::AreEqual(std::size_t{1}, h.record.clicked.size(), L"UI-105: no click on an inactive widget");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ui383TheCloseActionRunsTheBackActionOrTheBackHandler)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-383")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui383TheCloseActionRunsTheBackActionOrTheBackHandler)
  {
    Harness h;
    h.Frame(Keys(false, false, false, true));
    h.Frame({});
    Assert::AreEqual(std::size_t{1}, h.ui.Stack().size(), L"the root ignores Back");
    h.ui.Request({Neuron::UiPageRequest::Kind::Push, OPTIONS});
    h.Frame({});
    h.Frame(Keys(false, false, false, true));
    Assert::IsTrue(h.record.clicked.back() == "options:back", L"the close action clicks the back action");
    h.Frame({});
    Assert::AreEqual(TITLE, h.ui.Stack().back(), L"whose page pops");
    h.ui.Request({Neuron::UiPageRequest::Kind::Push, CREDITS});
    h.Frame({});
    h.Frame(Keys(false, false, false, true));
    h.Frame({});
    Assert::AreEqual(TITLE, h.ui.Stack().back(), L"a page with no back action pops through its back handler");
    // UI-217: the cycler's change is the page's own; Back applies nothing of its own.
    Assert::IsTrue(h.record.changed.empty());
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ui336AModalLayerTakesEveryInputUntilItCloses)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-336")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui336AModalLayerTakesEveryInputUntilItCloses)
  {
    Harness h;
    h.Frame(Keys(true));               // the cursor to "layer"
    h.Frame(Keys(false, false, true)); // which opens a layer at the next frame
    h.Frame({});
    Assert::AreEqual(std::string("layer"), std::string(h.ui.Focused()->Name()), L"the layer has the focus");
    Assert::AreEqual(std::size_t{1}, h.ui.Stack().size(), L"and the page stack is unchanged");
    h.Frame(Keys(true));
    h.Frame(Keys(false, false, true)); // activates "close" in the layer
    Assert::IsTrue(h.record.clicked.back() == "layer:close", L"the input reached the layer, not the page below");
    h.Frame({});
    Assert::AreEqual(std::string("title"), std::string(h.ui.Focused()->Name()), L"the layer closed");
    Assert::AreEqual(std::string("layer"), h.CursorId(), L"and the page kept its cursor");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ui172ARelayoutKeepsEveryWidgetAndItsState)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-172")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui172ARelayoutKeepsEveryWidgetAndItsState)
  {
    Harness h;
    h.Frame(Keys(false, true)); // the cursor to "check"
    h.Frame(Keys(false, false, true));
    const Neuron::UiHandle check = h.ui.Focused()->Find("check");
    const std::size_t entered = h.record.entered.size();
    const std::size_t laidOut = h.record.laidOut.size();
    h.Frame({});
    Assert::AreEqual(laidOut, h.record.laidOut.size(), L"no relayout while nothing changes");
    h.metrics.client = {1920, 1080};
    h.Frame({});
    Assert::AreEqual(laidOut + 1, h.record.laidOut.size(), L"a size change lays the page out again");
    Assert::AreEqual(entered, h.record.entered.size(), L"UI-174: nothing is entered or created again");
    Assert::IsTrue(h.ui.Focused()->Find("check") == check && h.ui.Focused()->Checked(check), L"the widget and its state stay");
    Assert::AreEqual(std::string("check"), h.CursorId(), L"and the cursor");
    Assert::AreEqual(960.0f, h.ui.Focused()->RectOf(check).Width(), 0.0f, L"at the new size");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ui372TheValidatorFindsWhatALayoutGetsWrong)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-372")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui372TheValidatorFindsWhatALayoutGetsWrong)
  {
    Harness h;
    Neuron::UiWindow& window = *h.ui.Focused();
    Assert::IsTrue(Neuron::ValidateWindow(window).empty(), L"the test page passes");
    window.Add(TestPage::Row("push"));                                   // a second "push", in no order
    window.SetNavigation({window.Find("caption"), window.Find("push")}); // a label in the order
    const std::vector<std::string> problems = Neuron::ValidateWindow(window);
    Assert::IsTrue(problems.size() >= 3, L"the duplicate id, the label in the order and the focusable widgets in no order");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ui116ACyclerStepsToTheNextOptionWrapping)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-116")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui116ACyclerStepsToTheNextOptionWrapping)
  {
    Harness h;
    h.ui.Request({Neuron::UiPageRequest::Kind::Push, OPTIONS});
    h.Frame({});
    const Neuron::UiHandle cycle = h.ui.Focused()->Find("cycle");
    for (int n = 0; n < 3; ++n)
    {
      h.Frame(Keys(false, false, true));
    }
    Assert::AreEqual(std::size_t{0}, h.ui.Focused()->Selected(cycle), L"three steps of three options come round");
    Assert::AreEqual(std::size_t{3}, h.record.changed.size());
    Neuron::UiInput left;
    left.left = true;
    h.Frame(left);
    Assert::AreEqual(std::size_t{0}, h.ui.Focused()->Selected(cycle), L"UI-210: left and right do not change it");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ui125TheWidgetsDrawTheirStates)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-125")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui125TheWidgetsDrawTheirStates)
  {
    Harness h;
    Neuron::TextSystem::Desc desc;
    for (std::vector<std::string>& chain : desc.families)
    {
      chain = {"Bahnschrift", "Segoe UI"};
    }
    Neuron::TextSystem text(desc);
    Neuron::UiStyle style;
    style.checkTick = std::uint16_t{3};
    const Neuron::UiWindow& window = *h.ui.Focused();
    std::vector<Neuron::UiBatch> unchecked;
    h.ui.Draw(text, style, 0.0, unchecked);
    Assert::AreEqual(std::size_t{0}, ImageQuads(unchecked, 3), L"no tick while unchecked");
    Assert::IsTrue(HasSolid(unchecked, window.RectOf(window.Find("push")), style.focusBand), L"UI-108: the cursor's focus band");
    Assert::IsTrue(HasSolid(unchecked, window.RectOf(window.Find("check")), style.rowBand), L"UI-117: an unfocused row's band");
    h.Frame(Keys(false, true));        // the cursor to the check box
    h.Frame(Keys(false, false, true)); // which toggles
    std::vector<Neuron::UiBatch> checked;
    h.ui.Draw(text, style, 0.0, checked);
    Assert::AreEqual(std::size_t{1}, ImageQuads(checked, 3), L"UI-081: the tick while checked");
    Assert::IsFalse(HasSolid(checked, window.RectOf(window.Find("push")), style.focusBand), L"the band moved with the cursor");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ui036AClosedScreenTakesNoInputAndAStaleHandleFindsNothing)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-036")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui036AClosedScreenTakesNoInputAndAStaleHandleFindsNothing)
  {
    Harness h;
    const Neuron::UiHandle push = h.ui.Focused()->Find("push");
    h.ui.Request({Neuron::UiPageRequest::Kind::Push, OPTIONS});
    h.Frame({});
    Assert::IsFalse(h.ui.Focused()->Contains(push), L"the left page's handle finds nothing on the new page");
    Assert::IsTrue(h.ui.TakesPointer());
    h.ui.CloseScreen();
    h.Frame(Keys(true));
    Assert::IsTrue(h.ui.Focused() == nullptr && !h.ui.TakesPointer(), L"no screen, no focus");
    Assert::IsTrue(h.ui.Stack().empty());
  }
};

} // namespace NeuronClientTests
