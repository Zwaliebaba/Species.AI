#include "pch.h"

#include "FrontEnd.h"
#include "GameUi.h"
#include "InputBindings.h"
#include "UiLook.h"
#include "WorldMenu.h"

#include <chrono>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace Species
{

namespace
{

// The first and last of the world's actions that a focused window withholds (UI-006): the camera's and the verbs'.
constexpr auto WORLD_ACTION_FIRST = static_cast<std::uint16_t>(GameAction::CameraPanLeft);
constexpr auto WORLD_ACTION_LAST = static_cast<std::uint16_t>(GameAction::OrderDemote);

Neuron::Nanoseconds SecondsOf(double _seconds) noexcept
{
  return std::chrono::duration_cast<Neuron::Nanoseconds>(std::chrono::duration<double>(_seconds));
}

#if ENGINE_DEVELOPMENT

// The test page's text field's digits at most: a 64-bit seed's (ADR-154).
constexpr std::size_t TEST_FIELD_DIGITS = 20;

// The test screen (ADR-149): fill boxes on the page grid of UI-163, the page title of UI-373's skeleton, and a column of
// rows in the medium font: page buttons, a check box, a cycler, a text field (ADR-154) and an inactive button. Its texts
// are the test's own.
class TestPage final : public Neuron::UiPage
{
public:
  TestPage(const TuningPresentation& _presentation, bool _root)
    : m_presentation(_presentation),
      m_root(_root)
  {
  }

  [[nodiscard]] std::string_view Name() const override
  {
    return m_root ? "ui test" : "ui test second";
  }

  void Enter(Neuron::UiWindow& _window) override
  {
    using Kind = Neuron::UiWidgetKind;
    const auto add = [&_window](Kind _kind, std::string _id, std::string _text)
    {
      Neuron::UiWidgetDesc desc;
      desc.kind = _kind;
      desc.id = std::move(_id);
      desc.text = std::move(_text);
      return _window.Add(std::move(desc));
    };
    m_titleBar = add(Kind::FillBox, "title_bar", {});
    m_leftBox = add(Kind::FillBox, "left_box", {});
    m_rightBox = add(Kind::FillBox, "right_box", {});
    m_title = add(Kind::PageTitle, "title", m_root ? "UI test page" : "Second page");
    std::vector<Neuron::UiHandle> order;
    if (m_root)
    {
      order.push_back(add(Kind::PageButton, "second", "Open the second page"));
      order.push_back(add(Kind::PageButton, "layer", "Open a modal layer"));
      order.push_back(add(Kind::CheckBox, "check", "A check box"));
      Neuron::UiWidgetDesc cycler;
      cycler.kind = Kind::Cycler;
      cycler.id = "cycler";
      cycler.text = "A cycler";
      cycler.options = {"One", "Two", "Three"};
      order.push_back(_window.Add(std::move(cycler)));
      // ADR-154: a page text field of digits, as the new world's seed is.
      Neuron::UiWidgetDesc field;
      field.kind = Kind::TextField;
      field.id = "field";
      field.text = "A text field of digits";
      field.digitsOnly = true;
      field.maxLength = TEST_FIELD_DIGITS;
      order.push_back(_window.Add(std::move(field)));
      Neuron::UiWidgetDesc inactive;
      inactive.kind = Kind::PageButton;
      inactive.id = "inactive";
      inactive.text = "An inactive button";
      inactive.inactive = true;
      order.push_back(_window.Add(std::move(inactive)));
    }
    else
    {
      order.push_back(add(Kind::CheckBox, "check", "Another check box"));
      m_back = add(Kind::PageButton, "back", "Back");
      order.push_back(m_back);
      _window.SetBackAction(m_back);
    }
    m_rows = order;
    _window.SetNavigation(std::move(order));
  }

  void Layout(Neuron::UiWindow& _window, const Neuron::UiMetrics& _metrics) override
  {
    const Neuron::PageGrid grid = Neuron::ComputePageGrid(_metrics.client, PageGridKeysOf(m_presentation));
    const auto& layout = m_presentation.ref.meta.layout;
    _window.SetRect(m_titleBar, grid.titleBar);
    _window.SetRect(m_leftBox, grid.leftBox);
    _window.SetRect(m_rightBox, grid.rightBox);
    const Neuron::UiRect& left = grid.leftBox;
    const auto inset = static_cast<float>(layout.titleInset);
    _window.SetRect(m_title, Neuron::UiRect::FromSize(left.left + inset, left.top + inset, left.Width() - 2.0f * inset,
                                                      static_cast<float>(layout.titleHeight) * grid.buttonHeight));
    _window.SetFontPixels(m_title, grid.fontMedium);
    // R-META-201: the first row half a button and a title below the box's top, one step lower; Back at its offset above
    // the box's bottom.
    float y = left.top + 1.8f * grid.buttonHeight + grid.ButtonStep();
    for (const Neuron::UiHandle row : m_rows)
    {
      Neuron::UiRect rect = Neuron::UiRect::FromSize(grid.ButtonColumnLeft(), y, grid.buttonWidth, grid.buttonHeight);
      if (row == m_back)
      {
        rect = Neuron::UiRect::FromSize(grid.ButtonColumnLeft(), left.bottom - static_cast<float>(layout.backOffset) * grid.buttonHeight,
                                        grid.buttonWidth, grid.buttonHeight);
      }
      _window.SetRect(row, rect);
      _window.SetFontPixels(row, grid.fontMedium);
      y += grid.ButtonStep();
    }
  }

  void Update(Neuron::UiWindow& _window, std::span<const Neuron::UiEvent> _events, Neuron::UiNavigator& _navigator) override
  {
    for (const Neuron::UiEvent& event : _events)
    {
      if (event.kind != Neuron::UiEventKind::Click)
      {
        continue;
      }
      if (event.widget == _window.Find("second"))
      {
        _navigator.Request({Neuron::UiPageRequest::Kind::Push, static_cast<Neuron::UiPageId>(GamePage::TestSecond)});
      }
      else if (event.widget == _window.Find("layer"))
      {
        _navigator.OpenLayer(std::make_unique<TestLayer>(m_presentation));
      }
      else if (event.widget == m_back)
      {
        Back(_window, _navigator);
      }
    }
  }

  void Back(Neuron::UiWindow& _window, Neuron::UiNavigator& _navigator) override
  {
    if (!m_root)
    {
      UiPage::Back(_window, _navigator); // UI-383: the root ignores the close action
    }
  }

private:
  /// The test's modal layer (UI-336): a fill box in the middle of the screen with a line and a Close button.
  class TestLayer final : public Neuron::UiPage
  {
  public:
    explicit TestLayer(const TuningPresentation& _presentation)
      : m_presentation(_presentation)
    {
    }

    [[nodiscard]] std::string_view Name() const override
    {
      return "ui test layer";
    }

    void Enter(Neuron::UiWindow& _window) override
    {
      Neuron::UiWidgetDesc box;
      box.kind = Neuron::UiWidgetKind::FillBox;
      box.id = "box";
      m_box = _window.Add(std::move(box));
      Neuron::UiWidgetDesc message;
      message.id = "message";
      message.text = "A modal layer takes every input until it closes.";
      message.role = Neuron::FontRole::Body;
      message.align = Neuron::UiAlign::Center;
      m_message = _window.Add(std::move(message));
      Neuron::UiWidgetDesc close;
      close.kind = Neuron::UiWidgetKind::PageButton;
      close.id = "close";
      close.text = "Close";
      close.align = Neuron::UiAlign::Center;
      m_close = _window.Add(std::move(close));
      _window.SetNavigation({m_close});
      _window.SetBackAction(m_close);
    }

    void Layout(Neuron::UiWindow& _window, const Neuron::UiMetrics& _metrics) override
    {
      const auto width = static_cast<float>(_metrics.client.width);
      const auto height = static_cast<float>(_metrics.client.height);
      const Neuron::PageGrid grid = Neuron::ComputePageGrid(_metrics.client, PageGridKeysOf(m_presentation));
      const Neuron::UiRect box = Neuron::UiRect::FromSize(width / 3.0f, height * 0.375f, width / 3.0f, height * 0.25f);
      const float row = grid.buttonHeight;
      _window.SetRect(m_box, box);
      _window.SetRect(m_message, Neuron::UiRect::FromSize(box.left, box.top + row, box.Width(), row));
      _window.SetFontPixels(m_message, grid.fontSmall);
      _window.SetRect(m_close, Neuron::UiRect::FromSize(box.left + box.Width() * 0.25f, box.bottom - 2.0f * row, box.Width() * 0.5f, row));
      _window.SetFontPixels(m_close, grid.fontMedium);
    }

    void Update(Neuron::UiWindow& /*_window*/, std::span<const Neuron::UiEvent> _events, Neuron::UiNavigator& _navigator) override
    {
      for (const Neuron::UiEvent& event : _events)
      {
        if (event.kind == Neuron::UiEventKind::Click && event.widget == m_close)
        {
          _navigator.CloseLayer();
        }
      }
    }

  private:
    const TuningPresentation& m_presentation;
    Neuron::UiHandle m_box;
    Neuron::UiHandle m_message;
    Neuron::UiHandle m_close;
  };

  const TuningPresentation& m_presentation;
  bool m_root = false;
  Neuron::UiHandle m_titleBar;
  Neuron::UiHandle m_leftBox;
  Neuron::UiHandle m_rightBox;
  Neuron::UiHandle m_title;
  Neuron::UiHandle m_back;
  std::vector<Neuron::UiHandle> m_rows;
};

#endif

} // namespace

GameUi::GameUi(const TuningPresentation& _presentation)
  : m_presentation(_presentation),
    m_style(UiStyleOf(_presentation)),
    m_core(
      [this](Neuron::UiPageId _page) -> std::unique_ptr<Neuron::UiPage>
      {
#if ENGINE_DEVELOPMENT
        if (_page == static_cast<Neuron::UiPageId>(GamePage::TestRoot) || _page == static_cast<Neuron::UiPageId>(GamePage::TestSecond))
        {
          return std::make_unique<TestPage>(m_presentation, _page == static_cast<Neuron::UiPageId>(GamePage::TestRoot));
        }
#endif
        // ADR-155: the front end's pages, while it is open.
        const auto first = static_cast<Neuron::UiPageId>(GamePage::FrontEndMain);
        if (m_frontEnd != nullptr && _page >= first && _page <= static_cast<Neuron::UiPageId>(GamePage::FrontEndPastRuns))
        {
          return m_frontEnd->MakePage(static_cast<FrontEndPage>(_page - first));
        }
        // ADR-170: the world's menu and epilogue, while a world runs.
        const auto menu = static_cast<Neuron::UiPageId>(GamePage::WorldMenu);
        if (m_worldMenu != nullptr && _page >= menu && _page <= static_cast<Neuron::UiPageId>(GamePage::WorldEpilogue))
        {
          return m_worldMenu->MakePage(static_cast<WorldMenuPage>(_page - menu));
        }
        return nullptr;
      })
{
}

#if ENGINE_DEVELOPMENT
void GameUi::OpenTestScreen()
{
  m_core.OpenScreen(static_cast<Neuron::UiPageId>(GamePage::TestRoot));
}
#endif

void GameUi::OpenFrontEnd(FrontEnd& _frontEnd, GamePage _page)
{
  m_frontEnd = &_frontEnd;
  m_core.OpenScreen(static_cast<Neuron::UiPageId>(GamePage::FrontEndMain));
  if (_page != GamePage::FrontEndMain)
  {
    m_core.Request({Neuron::UiPageRequest::Kind::Push, static_cast<Neuron::UiPageId>(_page)});
  }
}

void GameUi::CloseFrontEnd()
{
  m_core.CloseScreen();
}

void GameUi::AttachWorldMenu(WorldMenu* _menu)
{
  if (_menu == nullptr && m_worldMenu != nullptr && !m_core.Stack().empty() &&
      m_core.Stack().front() == static_cast<Neuron::UiPageId>(GamePage::WorldMenu))
  {
    m_core.CloseScreen();
  }
  m_worldMenu = _menu;
}

void GameUi::OpenWorldMenu(GamePage _page)
{
  if (m_worldMenu == nullptr)
  {
    return;
  }
  m_core.OpenScreen(static_cast<Neuron::UiPageId>(GamePage::WorldMenu));
  if (_page != GamePage::WorldMenu)
  {
    m_core.Request({Neuron::UiPageRequest::Kind::Push, static_cast<Neuron::UiPageId>(_page)});
  }
}

void GameUi::CloseWorldMenu()
{
  if (!m_core.Stack().empty() && m_core.Stack().front() == static_cast<Neuron::UiPageId>(GamePage::WorldMenu))
  {
    m_core.CloseScreen();
  }
}

void GameUi::SetScales(std::optional<double> _uiScale, std::optional<double> _textScale)
{
  m_uiScale = _uiScale;
  m_textScale = _textScale;
}

void GameUi::Update(Neuron::InputRouter& _router, Neuron::SizePx _client, float _dpiScale, bool _windowFocused)
{
  // INP-133: the menu directions repeat on the record's timing.
  _router.SetRepeatTiming({SecondsOf(m_presentation.inp.repeat.initialDelay), SecondsOf(m_presentation.inp.repeat.period)});
  const auto fired = [&_router](GameAction _action) { return _router.Query(ActionOf(_action)); };
  Neuron::UiInput input;
  input.up = fired(GameAction::MenuUp);
  input.down = fired(GameAction::MenuDown);
  input.left = fired(GameAction::MenuLeft);
  input.right = fired(GameAction::MenuRight);
  input.activate = fired(GameAction::MenuActivate);
  input.close = fired(GameAction::MenuClose);
  const Neuron::PointerState& pointer = _router.Pointer();
  input.pointerX = static_cast<float>(pointer.positionPixels.x);
  input.pointerY = static_cast<float>(pointer.positionPixels.y);
  input.pointerMoved = pointer.moved;
  input.pointerDown = fired(GameAction::UiPointerDown);
  input.pointerHeld = fired(GameAction::UiPointerHeld);
  // UI-035: the release is the end of the hold, also of a press released within its frame.
  input.pointerUp = (m_pointerHeld || input.pointerDown) && !input.pointerHeld;
  m_pointerHeld = input.pointerHeld;
  input.windowFocused = _windowFocused;
  input.text = _router.Text(); // INP-112 (ADR-154)
  // ADR-170: in a world with no window open, the close action opens the world's menu, and does no more this frame.
  if (m_worldMenu != nullptr && input.close && m_core.Stack().empty())
  {
    OpenWorldMenu();
    input.close = false;
  }
  m_metrics = UiMetricsOf(m_presentation, _client, _dpiScale, m_uiScale, m_textScale);
  m_core.Update(input, m_metrics);
  // UI-006: what a focused window takes reaches no world action.
  if (m_core.Focused() != nullptr)
  {
    for (std::uint16_t action = WORLD_ACTION_FIRST; action <= WORLD_ACTION_LAST; ++action)
    {
      _router.Suppress({action});
    }
  }
}

void GameUi::Draw(Neuron::TextSystem& _text, double _seconds, std::vector<Neuron::UiBatch>& _out) const
{
  m_core.Draw(_text, m_style, _seconds, _out);
}

std::optional<Neuron::UiRect> GameUi::TestSampleArea() const
{
#if ENGINE_DEVELOPMENT
  const std::span<const Neuron::UiPageId> stack = m_core.Stack();
  if (!stack.empty() && stack.back() == static_cast<Neuron::UiPageId>(GamePage::TestRoot))
  {
    return Neuron::ComputePageGrid(m_metrics.client, PageGridKeysOf(m_presentation)).rightBox;
  }
#endif
  return std::nullopt;
}

} // namespace Species
