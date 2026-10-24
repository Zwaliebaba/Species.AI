#include "pch.h"

#include "ClientSettings.h"
#include "FrontEnd.h"
#include "GameUi.h"
#include "Log.h"
#include "PageSkeleton.h"
#include "UiLook.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <format>
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
constexpr std::string_view STRING_NEIGHBORS = "ui.front.neighbors";               // O103
constexpr std::string_view STRING_NEIGHBORS_OPTION = "ui.front.neighbors_option"; // O103
constexpr std::string_view STRING_PACE = "ui.front.pace";                         // ADR-191
constexpr std::string_view STRING_PACE_OPTION = "ui.front.pace_option";           // ADR-191
constexpr std::string_view STRING_WINDOW = "ui.front.window";
constexpr std::string_view STRING_ANTI_ALIASING = "ui.front.anti_aliasing";
constexpr std::string_view STRING_ANTI_ALIASING_OPTION = "ui.front.anti_aliasing_option";
constexpr std::string_view STRING_VSYNC = "ui.front.vsync";
constexpr std::string_view STRING_VOLUME = "ui.front.volume";
constexpr std::string_view STRING_VOLUME_OPTION = "ui.front.volume_option";
constexpr std::string_view STRING_CONTINUE_ENDED = "ui.front.continue_ended"; // ADR-170
constexpr std::string_view STRING_PAST_RUNS = "ui.front.past_runs";
constexpr std::string_view STRING_PAST_RUN = "ui.front.past_run";
constexpr std::string_view STRING_NO_PAST_RUN = "ui.front.no_past_run";

// ContinueFault's selectors, in its order.
constexpr std::array<std::string_view, 5> FAULT_SELECTORS{"no_snapshot", "unreadable", "damaged", "version", "server"};
// The anti-aliasing options, as the settings store spells them and as the string's selector names them (O96).
constexpr std::array<std::string_view, 3> ANTI_ALIASING_VALUES{ANTI_ALIASING_OFF, ANTI_ALIASING_2X, ANTI_ALIASING_4X};
constexpr std::array<std::string_view, 3> ANTI_ALIASING_SELECTORS{"off", "two", "four"};
constexpr std::uint8_t SITE_COUNT = 4; // WLD-040: the region's settlement sites
// O103: the neighbors cycler's options, the AI settlements at the other sites (MVP-013) first, then none.
constexpr std::array<std::string_view, 2> NEIGHBOR_SELECTORS{"three", "none"};
constexpr std::array<std::uint8_t, 2> NEIGHBOR_COUNTS{AI_SETTLEMENTS_FULL, 0};
// MVP-071 (ADR-191): the pace cycler's options, the session pace first; each option's selector is its pace's name.
constexpr std::array<WorldPace, 2> PACES{WorldPace::Session, WorldPace::AlwaysOn};
constexpr std::int64_t PERCENT_FULL = 100; // the volume's 100 %
/// The front end's page skeleton (ADR-155): the shared skeleton, with the front end for its strings and its Back.
class SkeletonPage : public PageSkeleton
{
public:
  SkeletonPage(FrontEnd& _frontEnd, std::string_view _name)
    : PageSkeleton(*_frontEnd.Settings().presentation, _name),
      m_frontEnd(_frontEnd)
  {
  }

protected:
  // The notice and, when _back, the Back button; the navigation order is the rows, then Back.
  void Finish(Neuron::UiWindow& _window, bool _back, Neuron::UiHandle _initialFocus = {})
  {
    PageSkeleton::Finish(_window, _back ? std::optional<std::string>(m_frontEnd.Text(STRING_BACK)) : std::nullopt, _initialFocus);
  }

  FrontEnd& m_frontEnd;
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
      // ADR-170: a world whose run has ended continues read-only, on its epilogue.
      const std::string seed = std::to_string(*latest->seed);
      const std::array arguments{Neuron::FormatArgument{"seed", std::string_view(seed)}};
      resume.text = m_frontEnd.Text(latest->ended ? STRING_CONTINUE_ENDED : STRING_CONTINUE_WORLD, arguments);
    }
    else
    {
      resume.text = m_frontEnd.Text(STRING_CONTINUE);
    }
    m_continue = Row(_window, std::move(resume));
    m_newWorld = Row(_window, Neuron::UiWidgetKind::PageButton, "new_world", m_frontEnd.Text(STRING_NEW_WORLD));
    Neuron::UiWidgetDesc pastRuns;
    pastRuns.kind = Neuron::UiWidgetKind::PageButton;
    pastRuns.id = "past_runs";
    pastRuns.text = m_frontEnd.Text(STRING_PAST_RUNS);
    pastRuns.inactive = m_frontEnd.Ended().empty();
    m_pastRuns = Row(_window, std::move(pastRuns));
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
      else if (event.widget == m_pastRuns)
      {
        _navigator.Request({Neuron::UiPageRequest::Kind::Push, static_cast<Neuron::UiPageId>(GamePage::FrontEndPastRuns)});
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
  Neuron::UiHandle m_pastRuns;
  Neuron::UiHandle m_options;
  Neuron::UiHandle m_quit;
};

/// MVP-063 (ADR-170): the ended worlds, the newest first and at most PAST_RUNS_MAX, each opening on its epilogue.
class PastRunsPage final : public SkeletonPage
{
public:
  explicit PastRunsPage(FrontEnd& _frontEnd)
    : SkeletonPage(_frontEnd, "past runs")
  {
  }

  void Enter(Neuron::UiWindow& _window) override
  {
    Begin(_window, m_frontEnd.Text(STRING_PAST_RUNS));
    const std::vector<SavedWorld>& ended = m_frontEnd.Ended();
    m_worlds.clear();
    for (std::size_t index = 0; index < ended.size() && index < PAST_RUNS_MAX; ++index)
    {
      const SavedWorld& world = ended[index];
      const std::string seed = world.seed.has_value() ? std::to_string(*world.seed) : std::string("?");
      const std::array arguments{Neuron::FormatArgument{"number", std::int64_t{world.number}},
                                 Neuron::FormatArgument{"seed", std::string_view(seed)}};
      m_worlds.push_back(
        Row(_window, Neuron::UiWidgetKind::PageButton, std::format("world_{}", index), m_frontEnd.Text(STRING_PAST_RUN, arguments)));
    }
    Finish(_window, true);
    if (ended.empty())
    {
      _window.SetText(m_notice, m_frontEnd.Text(STRING_NO_PAST_RUN));
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
      if (const auto chosen = std::ranges::find(m_worlds, event.widget); chosen != m_worlds.end())
      {
        FrontEndChoice choice{FrontEndChoice::Kind::Epilogue};
        choice.world = m_frontEnd.Ended()[static_cast<std::size_t>(chosen - m_worlds.begin())];
        m_frontEnd.Choose(choice);
      }
      else if (event.widget == m_back)
      {
        Back(_window, _navigator);
      }
    }
  }

private:
  std::vector<Neuron::UiHandle> m_worlds;
};

/// MVP-005 and MVP-012: the new world's seed, typed or drawn, its steward's site, its neighbors (O103) and its pace
/// (MVP-071), then Start.
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
    Neuron::UiWidgetDesc neighbors;
    neighbors.kind = Neuron::UiWidgetKind::Cycler;
    neighbors.id = "neighbors";
    neighbors.text = m_frontEnd.Text(STRING_NEIGHBORS);
    for (const std::string_view selector : NEIGHBOR_SELECTORS)
    {
      const std::array arguments{Neuron::FormatArgument{"count", selector}};
      neighbors.options.push_back(m_frontEnd.Text(STRING_NEIGHBORS_OPTION, arguments));
    }
    m_neighbors = Row(_window, std::move(neighbors));
    Neuron::UiWidgetDesc pace;
    pace.kind = Neuron::UiWidgetKind::Cycler;
    pace.id = "pace";
    pace.text = m_frontEnd.Text(STRING_PACE);
    for (const WorldPace option : PACES)
    {
      const std::array arguments{Neuron::FormatArgument{"pace", PaceName(option)}};
      pace.options.push_back(m_frontEnd.Text(STRING_PACE_OPTION, arguments));
    }
    m_pace = Row(_window, std::move(pace));
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
        choice.aiSettlements = NEIGHBOR_COUNTS.at(_window.Selected(m_neighbors));
        choice.pace = PACES.at(_window.Selected(m_pace));
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
  Neuron::UiHandle m_neighbors;
  Neuron::UiHandle m_pace;
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

void FrontEnd::SetEnded(std::vector<SavedWorld> _ended)
{
  m_ended = std::move(_ended);
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
  case FrontEndPage::PastRuns:
    return std::make_unique<PastRunsPage>(*this);
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
