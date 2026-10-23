#include "pch.h"

#include "ClientSettings.h"
#include "FrontEnd.h"
#include "GameUi.h"
#include "Log.h"
#include "UiLook.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace Species
{

namespace
{

// The front end's strings (ADR-146, ADR-155), each in GameData/Strings/en_us.json.
constexpr std::string_view STRING_TITLE = "ui.title.game_name";
constexpr std::string_view STRING_CONTINUE = "ui.front.continue";
constexpr std::string_view STRING_CONTINUE_WORLD = "ui.front.continue_world";
constexpr std::string_view STRING_CONTINUE_FAILED = "ui.front.continue_failed";
constexpr std::string_view STRING_SAVES_UNAVAILABLE = "ui.front.saves_unavailable";
constexpr std::string_view STRING_NEW_WORLD = "ui.front.new_world";
constexpr std::string_view STRING_OPTIONS = "ui.front.options";
constexpr std::string_view STRING_QUIT = "ui.front.quit";
constexpr std::string_view STRING_BACK = "ui.front.back";
constexpr std::string_view STRING_START = "ui.front.start";
constexpr std::string_view STRING_SEED = "ui.front.seed";
constexpr std::string_view STRING_SEED_TOO_LARGE = "ui.front.seed_too_large";
constexpr std::string_view STRING_SITE = "ui.front.site";
constexpr std::string_view STRING_SITE_OPTION = "ui.front.site_option";
constexpr std::string_view STRING_WINDOW = "ui.front.window";
constexpr std::string_view STRING_ANTI_ALIASING = "ui.front.anti_aliasing";
constexpr std::string_view STRING_ANTI_ALIASING_OPTION = "ui.front.anti_aliasing_option";
constexpr std::string_view STRING_VSYNC = "ui.front.vsync";
constexpr std::string_view STRING_VOLUME = "ui.front.volume";
constexpr std::string_view STRING_VOLUME_OPTION = "ui.front.volume_option";

// ContinueFault's selectors, in its order.
constexpr std::array<std::string_view, 4> FAULT_SELECTORS{"no_snapshot", "unreadable", "damaged", "version"};
// The anti-aliasing options, as the settings store spells them and as the string's selector names them (O96).
constexpr std::array<std::string_view, 3> ANTI_ALIASING_VALUES{ANTI_ALIASING_OFF, ANTI_ALIASING_2X, ANTI_ALIASING_4X};
constexpr std::array<std::string_view, 3> ANTI_ALIASING_SELECTORS{"off", "two", "four"};
constexpr std::uint8_t SITE_COUNT = 4;     // WLD-040: the region's settlement sites
constexpr std::int64_t PERCENT_FULL = 100; // the volume's 100 %
constexpr float ROW_TITLE_ROWS = 1.8f;     // R-META-201: the first row half a button and a title below the box's top
constexpr float NOTICE_ROWS = 2.0f;        // a notice spans two rows of the body text

/// The front end's page skeleton (UI-373 at M2, ADR-155): the page grid's title bar and boxes, the page title in the left
/// box, a column of rows in the medium font, a notice in the body role below them, and Back at its offset above the
/// box's bottom.
class SkeletonPage : public Neuron::UiPage
{
public:
  SkeletonPage(FrontEnd& _frontEnd, std::string_view _name)
    : m_frontEnd(_frontEnd),
      m_name(_name)
  {
  }

  [[nodiscard]] std::string_view Name() const override
  {
    return m_name;
  }

  void Layout(Neuron::UiWindow& _window, const Neuron::UiMetrics& _metrics) override
  {
    const TuningPresentation& presentation = *m_frontEnd.Settings().presentation;
    const Neuron::PageGrid grid = Neuron::ComputePageGrid(_metrics.client, PageGridKeysOf(presentation));
    const auto& layout = presentation.ref.meta.layout;
    _window.SetRect(m_titleBar, grid.titleBar);
    _window.SetRect(m_leftBox, grid.leftBox);
    _window.SetRect(m_rightBox, grid.rightBox);
    const Neuron::UiRect& left = grid.leftBox;
    const auto inset = static_cast<float>(layout.titleInset);
    _window.SetRect(m_title, Neuron::UiRect::FromSize(left.left + inset, left.top + inset, left.Width() - 2.0f * inset,
                                                      static_cast<float>(layout.titleHeight) * grid.buttonHeight));
    _window.SetFontPixels(m_title, grid.fontMedium);
    float y = left.top + ROW_TITLE_ROWS * grid.buttonHeight + grid.ButtonStep();
    for (const Neuron::UiHandle row : m_rows)
    {
      _window.SetRect(row, Neuron::UiRect::FromSize(grid.ButtonColumnLeft(), y, grid.buttonWidth, grid.buttonHeight));
      _window.SetFontPixels(row, grid.fontMedium);
      y += grid.ButtonStep();
    }
    _window.SetRect(m_notice, Neuron::UiRect::FromSize(grid.ButtonColumnLeft(), y, grid.buttonWidth, NOTICE_ROWS * grid.buttonHeight));
    _window.SetFontPixels(m_notice, grid.fontSmall);
    if (m_back)
    {
      _window.SetRect(m_back, Neuron::UiRect::FromSize(grid.ButtonColumnLeft(),
                                                       left.bottom - static_cast<float>(layout.backOffset) * grid.buttonHeight,
                                                       grid.buttonWidth, grid.buttonHeight));
      _window.SetFontPixels(m_back, grid.fontMedium);
    }
  }

protected:
  // The skeleton with its title; the page adds its rows, then Finish.
  void Begin(Neuron::UiWindow& _window, std::string _title)
  {
    m_titleBar = Add(_window, Neuron::UiWidgetKind::FillBox, "title_bar", {});
    m_leftBox = Add(_window, Neuron::UiWidgetKind::FillBox, "left_box", {});
    m_rightBox = Add(_window, Neuron::UiWidgetKind::FillBox, "right_box", {});
    m_title = Add(_window, Neuron::UiWidgetKind::PageTitle, "title", std::move(_title));
  }
  Neuron::UiHandle Row(Neuron::UiWindow& _window, Neuron::UiWidgetDesc _desc)
  {
    const Neuron::UiHandle row = _window.Add(std::move(_desc));
    m_rows.push_back(row);
    return row;
  }
  Neuron::UiHandle Row(Neuron::UiWindow& _window, Neuron::UiWidgetKind _kind, std::string _id, std::string _text)
  {
    Neuron::UiWidgetDesc desc;
    desc.kind = _kind;
    desc.id = std::move(_id);
    desc.text = std::move(_text);
    return Row(_window, std::move(desc));
  }
  // The notice and, when _back, the Back button as the back action; the navigation order is the rows, then Back.
  void Finish(Neuron::UiWindow& _window, bool _back, Neuron::UiHandle _initialFocus = {})
  {
    Neuron::UiWidgetDesc notice;
    notice.id = "notice";
    notice.role = Neuron::FontRole::Body;
    m_notice = _window.Add(std::move(notice));
    std::vector<Neuron::UiHandle> order = m_rows;
    if (_back)
    {
      m_back = Add(_window, Neuron::UiWidgetKind::PageButton, "back", m_frontEnd.Text(STRING_BACK));
      _window.SetBackAction(m_back);
      order.push_back(m_back);
    }
    _window.SetNavigation(std::move(order), _initialFocus);
  }
  static Neuron::UiHandle Add(Neuron::UiWindow& _window, Neuron::UiWidgetKind _kind, std::string _id, std::string _text)
  {
    Neuron::UiWidgetDesc desc;
    desc.kind = _kind;
    desc.id = std::move(_id);
    desc.text = std::move(_text);
    return _window.Add(std::move(desc));
  }

  FrontEnd& m_frontEnd;
  Neuron::UiHandle m_notice;
  Neuron::UiHandle m_back;

private:
  std::string_view m_name;
  Neuron::UiHandle m_titleBar;
  Neuron::UiHandle m_leftBox;
  Neuron::UiHandle m_rightBox;
  Neuron::UiHandle m_title;
  std::vector<Neuron::UiHandle> m_rows;
};

/// MVP-103's main page: Continue, New world, Options and Quit, with why the last Continue failed. It is the root, so the
/// close action does nothing (UI-383).
class MainPage final : public SkeletonPage
{
public:
  explicit MainPage(FrontEnd& _frontEnd)
    : SkeletonPage(_frontEnd, "front end")
  {
  }

  void Enter(Neuron::UiWindow& _window) override
  {
    Begin(_window, m_frontEnd.Text(STRING_TITLE));
    const std::optional<SavedWorld>& latest = m_frontEnd.Latest();
    Neuron::UiWidgetDesc resume;
    resume.kind = Neuron::UiWidgetKind::PageButton;
    resume.id = "continue";
    resume.inactive = !latest.has_value();
    if (latest.has_value() && latest->seed.has_value())
    {
      const std::string seed = std::to_string(*latest->seed);
      const std::array arguments{Neuron::FormatArgument{"seed", std::string_view(seed)}};
      resume.text = m_frontEnd.Text(STRING_CONTINUE_WORLD, arguments);
    }
    else
    {
      resume.text = m_frontEnd.Text(STRING_CONTINUE);
    }
    m_continue = Row(_window, std::move(resume));
    m_newWorld = Row(_window, Neuron::UiWidgetKind::PageButton, "new_world", m_frontEnd.Text(STRING_NEW_WORLD));
    m_options = Row(_window, Neuron::UiWidgetKind::PageButton, "options", m_frontEnd.Text(STRING_OPTIONS));
    m_quit = Row(_window, Neuron::UiWidgetKind::PageButton, "quit", m_frontEnd.Text(STRING_QUIT));
    Finish(_window, false, latest.has_value() ? m_continue : m_newWorld);
    // ADR-155: why the last Continue failed; else that no world is kept when the saves folder is missing (PLT-017).
    if (const std::optional<ContinueFault>& fault = m_frontEnd.LastFault(); fault.has_value())
    {
      const std::array arguments{Neuron::FormatArgument{"reason", FAULT_SELECTORS.at(static_cast<std::size_t>(*fault))}};
      _window.SetText(m_notice, m_frontEnd.Text(STRING_CONTINUE_FAILED, arguments));
    }
    else if (!m_frontEnd.Settings().savesAvailable)
    {
      _window.SetText(m_notice, m_frontEnd.Text(STRING_SAVES_UNAVAILABLE));
    }
  }

  void Update(Neuron::UiWindow& /*_window*/, std::span<const Neuron::UiEvent> _events, Neuron::UiNavigator& _navigator) override
  {
    for (const Neuron::UiEvent& event : _events)
    {
      if (event.kind != Neuron::UiEventKind::Click)
      {
        continue;
      }
      if (event.widget == m_continue)
      {
        m_frontEnd.Choose({FrontEndChoice::Kind::Continue});
      }
      else if (event.widget == m_newWorld)
      {
        _navigator.Request({Neuron::UiPageRequest::Kind::Push, static_cast<Neuron::UiPageId>(GamePage::FrontEndNewWorld)});
      }
      else if (event.widget == m_options)
      {
        _navigator.Request({Neuron::UiPageRequest::Kind::Push, static_cast<Neuron::UiPageId>(GamePage::FrontEndOptions)});
      }
      else if (event.widget == m_quit)
      {
        m_frontEnd.Choose({FrontEndChoice::Kind::Quit});
      }
    }
  }

  void Back(Neuron::UiWindow& /*_window*/, Neuron::UiNavigator& /*_navigator*/) override {}

private:
  Neuron::UiHandle m_continue;
  Neuron::UiHandle m_newWorld;
  Neuron::UiHandle m_options;
  Neuron::UiHandle m_quit;
};

/// MVP-005 and MVP-012: the new world's seed, typed or drawn, and its steward's site, then Start.
class NewWorldPage final : public SkeletonPage
{
public:
  explicit NewWorldPage(FrontEnd& _frontEnd)
    : SkeletonPage(_frontEnd, "new world")
  {
  }

  void Enter(Neuron::UiWindow& _window) override
  {
    Begin(_window, m_frontEnd.Text(STRING_NEW_WORLD));
    Neuron::UiWidgetDesc seed;
    seed.kind = Neuron::UiWidgetKind::TextField;
    seed.id = "seed";
    seed.text = m_frontEnd.Text(STRING_SEED);
    seed.digitsOnly = true;
    seed.maxLength = SEED_DIGITS_MAX;
    m_seed = Row(_window, std::move(seed));
    Neuron::UiWidgetDesc site;
    site.kind = Neuron::UiWidgetKind::Cycler;
    site.id = "site";
    site.text = m_frontEnd.Text(STRING_SITE);
    for (std::int64_t number = 1; number <= SITE_COUNT; ++number)
    {
      const std::array arguments{Neuron::FormatArgument{"site", number}};
      site.options.push_back(m_frontEnd.Text(STRING_SITE_OPTION, arguments));
    }
    site.selected =
      static_cast<std::size_t>(std::clamp<std::int64_t>(m_frontEnd.Settings().tuning->mvp.sandbox.stewardSite, 0, SITE_COUNT - 1));
    m_site = Row(_window, std::move(site));
    m_start = Row(_window, Neuron::UiWidgetKind::PageButton, "start", m_frontEnd.Text(STRING_START));
    Finish(_window, true);
  }

  void Update(Neuron::UiWindow& _window, std::span<const Neuron::UiEvent> _events, Neuron::UiNavigator& _navigator) override
  {
    for (const Neuron::UiEvent& event : _events)
    {
      if (event.kind != Neuron::UiEventKind::Click)
      {
        continue;
      }
      if (event.widget == m_start)
      {
        const std::expected<std::optional<std::uint64_t>, std::monostate> seed = SeedOfText(_window.Value(m_seed));
        if (!seed)
        {
          const std::string largest = std::to_string(std::numeric_limits<std::uint64_t>::max());
          const std::array arguments{Neuron::FormatArgument{"largest", std::string_view(largest)}};
          _window.SetText(m_notice, m_frontEnd.Text(STRING_SEED_TOO_LARGE, arguments));
          continue;
        }
        FrontEndChoice choice{FrontEndChoice::Kind::NewWorld};
        choice.seed = *seed;
        choice.site = static_cast<std::uint8_t>(_window.Selected(m_site));
        m_frontEnd.Choose(choice);
      }
      else if (event.widget == m_back)
      {
        Back(_window, _navigator);
      }
    }
  }

private:
  Neuron::UiHandle m_seed;
  Neuron::UiHandle m_site;
  Neuron::UiHandle m_start;
};

/// O96's options: the window mode, anti-aliasing at the next start, vertical sync and the master volume, each written to
/// the settings store as it changes; leaving the page saves the store (PLT-030).
class OptionsPage final : public SkeletonPage
{
public:
  explicit OptionsPage(FrontEnd& _frontEnd)
    : SkeletonPage(_frontEnd, "options")
  {
  }

  void Enter(Neuron::UiWindow& _window) override
  {
    const Neuron::SettingsStore& settings = *m_frontEnd.Settings().settings;
    Begin(_window, m_frontEnd.Text(STRING_OPTIONS));
    Neuron::UiWidgetDesc windowed;
    windowed.kind = Neuron::UiWidgetKind::CheckBox;
    windowed.id = "window";
    windowed.text = m_frontEnd.Text(STRING_WINDOW);
    windowed.checked = settings.GetText(SETTING_WINDOW_MODE) == WINDOW_MODE_WINDOWED;
    m_window = Row(_window, std::move(windowed));
    Neuron::UiWidgetDesc antiAliasing;
    antiAliasing.kind = Neuron::UiWidgetKind::Cycler;
    antiAliasing.id = "anti_aliasing";
    antiAliasing.text = m_frontEnd.Text(STRING_ANTI_ALIASING);
    const std::string stored = settings.GetText(SETTING_ANTI_ALIASING);
    for (std::size_t option = 0; option < ANTI_ALIASING_VALUES.size(); ++option)
    {
      const std::array arguments{Neuron::FormatArgument{"mode", ANTI_ALIASING_SELECTORS[option]}};
      antiAliasing.options.push_back(m_frontEnd.Text(STRING_ANTI_ALIASING_OPTION, arguments));
      antiAliasing.selected = ANTI_ALIASING_VALUES[option] == stored ? option : antiAliasing.selected;
    }
    m_antiAliasing = Row(_window, std::move(antiAliasing));
    Neuron::UiWidgetDesc sync;
    sync.kind = Neuron::UiWidgetKind::CheckBox;
    sync.id = "vsync";
    sync.text = m_frontEnd.Text(STRING_VSYNC);
    sync.checked = settings.GetText(SETTING_PRESENT_MODE) == PRESENT_MODE_VERTICAL_SYNC;
    m_sync = Row(_window, std::move(sync));
    Neuron::UiWidgetDesc volume;
    volume.kind = Neuron::UiWidgetKind::Cycler;
    volume.id = "volume";
    volume.text = m_frontEnd.Text(STRING_VOLUME);
    for (std::int64_t percent = 0; percent <= PERCENT_FULL; percent += VOLUME_STEP)
    {
      const std::array arguments{Neuron::FormatArgument{"percent", percent}};
      volume.options.push_back(m_frontEnd.Text(STRING_VOLUME_OPTION, arguments));
    }
    // The stored volume's nearest step.
    const double steps = static_cast<double>(PERCENT_FULL) / static_cast<double>(VOLUME_STEP);
    volume.selected = static_cast<std::size_t>(
      std::lround(static_cast<double>(settings.GetInteger(SETTING_MASTER_VOLUME)) * steps / static_cast<double>(MASTER_VOLUME_MAX)));
    m_volume = Row(_window, std::move(volume));
    Finish(_window, true);
  }

  void Update(Neuron::UiWindow& _window, std::span<const Neuron::UiEvent> _events, Neuron::UiNavigator& _navigator) override
  {
    Neuron::SettingsStore& settings = *m_frontEnd.Settings().settings;
    for (const Neuron::UiEvent& event : _events)
    {
      if (event.kind == Neuron::UiEventKind::Changed)
      {
        if (event.widget == m_window)
        {
          const std::string_view mode = _window.Checked(m_window) ? WINDOW_MODE_WINDOWED : WINDOW_MODE_BORDERLESS;
          settings.Set(SETTING_WINDOW_MODE, std::string(mode));
          m_frontEnd.DisplayChanged();
        }
        else if (event.widget == m_antiAliasing)
        {
          settings.Set(SETTING_ANTI_ALIASING, std::string(ANTI_ALIASING_VALUES.at(_window.Selected(m_antiAliasing))));
        }
        else if (event.widget == m_sync)
        {
          settings.Set(SETTING_PRESENT_MODE, std::string(_window.Checked(m_sync) ? PRESENT_MODE_VERTICAL_SYNC : PRESENT_MODE_UNCAPPED));
          m_frontEnd.DisplayChanged();
        }
        else if (event.widget == m_volume)
        {
          const auto percent = static_cast<double>(static_cast<std::int64_t>(_window.Selected(m_volume)) * VOLUME_STEP);
          settings.Set(SETTING_MASTER_VOLUME,
                       std::int64_t{std::llround(percent * static_cast<double>(MASTER_VOLUME_MAX) / static_cast<double>(PERCENT_FULL))});
        }
      }
      else if (event.kind == Neuron::UiEventKind::Click && event.widget == m_back)
      {
        Back(_window, _navigator);
      }
    }
  }

  void Back(Neuron::UiWindow& _window, Neuron::UiNavigator& _navigator) override
  {
    // PLT-081: a failed save keeps the previous file and is logged; the options stay in memory for the run.
    if (!m_frontEnd.Settings().settings->Save())
    {
      Neuron::Log(Neuron::LogCategory::Platform, Neuron::LogLevel::Warning, "ui.dialog.settings_save_failed: the options");
    }
    UiPage::Back(_window, _navigator);
  }

private:
  Neuron::UiHandle m_window;
  Neuron::UiHandle m_antiAliasing;
  Neuron::UiHandle m_sync;
  Neuron::UiHandle m_volume;
};

} // namespace

FrontEnd::FrontEnd(const Desc& _desc)
  : m_desc(_desc)
{
}

void FrontEnd::SetLatest(std::optional<SavedWorld> _latest)
{
  m_latest = std::move(_latest);
}

void FrontEnd::SetContinueFault(std::optional<ContinueFault> _fault)
{
  m_fault = _fault;
}

std::unique_ptr<Neuron::UiPage> FrontEnd::MakePage(FrontEndPage _page)
{
  switch (_page)
  {
  case FrontEndPage::Main:
    return std::make_unique<MainPage>(*this);
  case FrontEndPage::NewWorld:
    return std::make_unique<NewWorldPage>(*this);
  case FrontEndPage::Options:
    return std::make_unique<OptionsPage>(*this);
  }
  return nullptr;
}

std::optional<FrontEndChoice> FrontEnd::TakeChoice() noexcept
{
  return std::exchange(m_choice, std::nullopt);
}

bool FrontEnd::TakeDisplayChange() noexcept
{
  return std::exchange(m_displayChanged, false);
}

std::string FrontEnd::Text(std::string_view _id, std::span<const Neuron::FormatArgument> _arguments) const
{
  return m_desc.strings->Format(_id, _arguments, *m_desc.locale);
}

void FrontEnd::Choose(const FrontEndChoice& _choice)
{
  m_choice = _choice;
  m_fault.reset();
}

std::expected<std::optional<std::uint64_t>, std::monostate> SeedOfText(std::string_view _text)
{
  if (_text.empty())
  {
    return std::nullopt;
  }
  std::uint64_t seed = 0;
  const auto [end, error] = std::from_chars(_text.data(), _text.data() + _text.size(), seed);
  if (error != std::errc{} || end != _text.data() + _text.size())
  {
    return std::unexpected(std::monostate{});
  }
  return seed;
}

} // namespace Species
