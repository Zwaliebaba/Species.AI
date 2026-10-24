#include "pch.h"

#include "GameUi.h"
#include "PageSkeleton.h"
#include "SettlementHud.h"
#include "WorldMenu.h"

#include <algorithm>
#include <array>
#include <format>
#include <utility>

namespace Species
{

namespace
{

// The world's menu and epilogue strings (ADR-170), each in GameData/Strings/en_us.json.
constexpr std::string_view STRING_MENU = "ui.world.menu";
constexpr std::string_view STRING_RESUME = "ui.world.resume";
constexpr std::string_view STRING_EPILOGUE = "ui.world.epilogue";
constexpr std::string_view STRING_LEAVE = "ui.world.leave";
constexpr std::string_view STRING_EPILOGUE_WAITS = "ui.world.epilogue_waits";
constexpr std::string_view STRING_QUIT = "ui.front.quit";
constexpr std::string_view STRING_BACK = "ui.front.back";
constexpr std::string_view STRING_STANDING = "ui.hud.standing";
constexpr std::string_view STRING_PLACE = "ui.epilogue.place";
constexpr std::string_view STRING_SOURCES = "ui.epilogue.sources";
constexpr std::string_view STRING_YOURS = "ui.epilogue.yours";
constexpr std::string_view STRING_GROWTH = "ui.epilogue.growth";
constexpr std::string_view STRING_CONTACT = "ui.epilogue.contact";
constexpr std::string_view STRING_BATTLES = "ui.epilogue.battles";
constexpr std::string_view STRING_CLAIMS = "ui.epilogue.claims";
constexpr std::string_view STRING_STRUCTURES = "ui.epilogue.structures";
constexpr std::string_view STRING_WORKERS = "ui.epilogue.workers";
constexpr std::string_view STRING_STRENGTH = "ui.epilogue.strength";
// MVP-074's away report (ADR-191).
constexpr std::string_view STRING_AWAY_REPORT = "ui.world.away_report";
constexpr std::string_view STRING_AWAY_WAITS = "ui.world.away_waits";
constexpr std::string_view STRING_AWAY_SUMMARY = "ui.away.summary";
constexpr std::string_view STRING_AWAY_DEPUTY = "ui.away.deputy";
constexpr std::string_view STRING_AWAY_LENGTH = "ui.away.length";
constexpr std::string_view STRING_AWAY_WORKERS = "ui.away.workers";
constexpr std::string_view STRING_AWAY_STORES = "ui.away.stores";
constexpr std::string_view STRING_AWAY_BUILT = "ui.away.built";
constexpr std::string_view STRING_AWAY_BATTLE = "ui.away.battle";
constexpr std::string_view STRING_AWAY_CLAIM = "ui.away.claim";
constexpr std::string_view STRING_AWAY_GRIEVANCES = "ui.away.grievances";
constexpr std::string_view STRING_AWAY_FREEZE = "ui.away.freeze";
constexpr std::string_view STRING_AWAY_QUIET = "ui.away.quiet";
constexpr std::string_view STRING_AWAY_ACTION = "ui.away.action";
constexpr std::string_view STRING_AWAY_NO_ACTION = "ui.away.no_action";
constexpr std::string_view STRING_AWAY_MORE = "ui.away.more";
// FreezeCause's selectors, in its order (PIL-131).
constexpr std::array<std::string_view, 4> FREEZE_SELECTORS{"stall", "unclean", "stopped", "sleep"};

constexpr std::int64_t MILLI = 1'000;
constexpr double SECONDS_PER_MINUTE = 60.0;
constexpr std::int64_t MINUTES_PER_HOUR = 60;
constexpr float LINE_ROWS = 1.5f;    // a line of the epilogue's text, × the small font
constexpr float HEADING_ROWS = 1.6f; // the section's heading, × the medium font

/// The world's menu (ADR-170): Return to the world, the epilogue once it has come, Leave to the front end and Quit. It is
/// the screen's root; its close action returns to the world.
class MenuPage final : public PageSkeleton
{
public:
  explicit MenuPage(WorldMenu& _menu)
    : PageSkeleton(*_menu.Settings().presentation, "world menu"),
      m_menu(_menu)
  {
  }

  void Enter(Neuron::UiWindow& _window) override
  {
    Begin(_window, m_menu.Text(STRING_MENU));
    m_resume = Row(_window, Neuron::UiWidgetKind::PageButton, "resume", m_menu.Text(STRING_RESUME));
    // MVP-074 (ADR-191): the away report, once an absence has one.
    Neuron::UiWidgetDesc away;
    away.kind = Neuron::UiWidgetKind::PageButton;
    away.id = "away";
    away.text = m_menu.Text(STRING_AWAY_REPORT);
    away.inactive = !m_menu.Away().has_value();
    m_away = Row(_window, std::move(away));
    Neuron::UiWidgetDesc epilogue;
    epilogue.kind = Neuron::UiWidgetKind::PageButton;
    epilogue.id = "epilogue";
    epilogue.text = m_menu.Text(STRING_EPILOGUE);
    epilogue.inactive = !m_menu.Ended().has_value();
    m_epilogue = Row(_window, std::move(epilogue));
    m_leave = Row(_window, Neuron::UiWidgetKind::PageButton, "leave", m_menu.Text(STRING_LEAVE));
    m_quit = Row(_window, Neuron::UiWidgetKind::PageButton, "quit", m_menu.Text(STRING_QUIT));
    Finish(_window, std::nullopt, m_resume);
    if (!m_menu.Ended().has_value())
    {
      _window.SetText(m_notice, m_menu.Text(STRING_EPILOGUE_WAITS));
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
      if (event.widget == m_resume)
      {
        Back(_window, _navigator);
      }
      else if (event.widget == m_epilogue)
      {
        _navigator.Request({Neuron::UiPageRequest::Kind::Push, static_cast<Neuron::UiPageId>(GamePage::WorldEpilogue)});
      }
      else if (event.widget == m_away)
      {
        _navigator.Request({Neuron::UiPageRequest::Kind::Push, static_cast<Neuron::UiPageId>(GamePage::WorldAwayReport)});
      }
      else if (event.widget == m_leave)
      {
        m_menu.Choose({WorldMenuChoice::Kind::Leave});
      }
      else if (event.widget == m_quit)
      {
        m_menu.Choose({WorldMenuChoice::Kind::Quit});
      }
    }
  }

  // The root's close action returns to the world (UI-383).
  void Back(Neuron::UiWindow& /*_window*/, Neuron::UiNavigator& /*_navigator*/) override
  {
    m_menu.Choose({WorldMenuChoice::Kind::Resume});
  }

private:
  WorldMenu& m_menu;
  Neuron::UiHandle m_resume;
  Neuron::UiHandle m_away;
  Neuron::UiHandle m_epilogue;
  Neuron::UiHandle m_leave;
  Neuron::UiHandle m_quit;
};

/// MVP-063's epilogue (ADR-170): a row for each of its sections in the left box, which shows that section in the right
/// box, the standing at first, then Leave to the front end and Back; the section's title and its lines in the right box.
/// MVP-074's away report (ADR-191) takes the same page: its summary first, then the deputy's actions, without Leave.
class EpiloguePage final : public PageSkeleton
{
public:
  EpiloguePage(WorldMenu& _menu, bool _away)
    : PageSkeleton(*_menu.Settings().presentation, _away ? "away report" : "epilogue"),
      m_menu(_menu),
      m_away(_away)
  {
  }

  void Enter(Neuron::UiWindow& _window) override
  {
    Begin(_window, m_menu.Text(m_away ? STRING_AWAY_REPORT : STRING_EPILOGUE));
    m_text = {};
    m_rows.clear();
    m_lines.clear();
    const WorldMenu::Desc& desc = m_menu.Settings();
    if (m_away && m_menu.Away().has_value())
    {
      m_text = AwayReportTextOf(*m_menu.Away(), desc.tuning->ref.sim.tickPeriod, *desc.strings, *desc.locale);
    }
    else if (!m_away && m_menu.Ended().has_value())
    {
      m_text = EpilogueTextOf(*m_menu.Ended(), desc.tuning->ref.sim.tickPeriod, *desc.strings, *desc.locale);
    }
    std::size_t lines = 0;
    for (std::size_t index = 0; index < m_text.sections.size(); ++index)
    {
      m_rows.push_back(Row(_window, Neuron::UiWidgetKind::PageButton, std::format("section_{}", index), m_text.sections[index].title));
      lines = std::max(lines, m_text.sections[index].lines.size());
    }
    if (!m_away)
    {
      m_leave = Row(_window, Neuron::UiWidgetKind::PageButton, "leave", m_menu.Text(STRING_LEAVE));
    }
    Finish(_window, m_menu.Text(STRING_BACK), m_rows.empty() ? m_leave : m_rows.front());
    if (m_text.sections.empty())
    {
      _window.SetText(m_notice, m_menu.Text(m_away ? STRING_AWAY_WAITS : STRING_EPILOGUE_WAITS));
      return;
    }
    // The right box's labels, as many as the longest section has lines, which each section fills.
    Neuron::UiWidgetDesc heading;
    heading.id = "heading";
    heading.role = Neuron::FontRole::Title;
    m_heading = _window.Add(std::move(heading));
    for (std::size_t line = 0; line < lines; ++line)
    {
      Neuron::UiWidgetDesc label;
      label.id = std::format("line_{}", line);
      label.role = Neuron::FontRole::Body;
      m_lines.push_back(_window.Add(std::move(label)));
    }
    Show(_window, 0);
  }

  void Update(Neuron::UiWindow& _window, std::span<const Neuron::UiEvent> _events, Neuron::UiNavigator& _navigator) override
  {
    for (const Neuron::UiEvent& event : _events)
    {
      if (event.kind != Neuron::UiEventKind::Click)
      {
        continue;
      }
      if (const auto row = std::ranges::find(m_rows, event.widget); row != m_rows.end())
      {
        Show(_window, static_cast<std::size_t>(row - m_rows.begin()));
      }
      else if (event.widget == m_leave)
      {
        m_menu.Choose({WorldMenuChoice::Kind::Leave});
      }
      else if (event.widget == m_back)
      {
        Back(_window, _navigator);
      }
    }
  }

protected:
  void LayoutMore(Neuron::UiWindow& _window, const Neuron::PageGrid& _grid) override
  {
    if (!m_heading)
    {
      return;
    }
    // The right box holds the section's title and its lines below it, a line each.
    const Neuron::UiRect& box = _grid.rightBox;
    const auto inset = static_cast<float>(m_presentation.ref.meta.layout.titleInset);
    const float width = box.Width() - 2.0f * inset;
    const float headingHeight = HEADING_ROWS * _grid.fontMedium;
    _window.SetRect(m_heading, Neuron::UiRect::FromSize(box.left + inset, box.top + inset, width, headingHeight));
    _window.SetFontPixels(m_heading, _grid.fontMedium);
    const float step = LINE_ROWS * _grid.fontSmall;
    float top = box.top + inset + headingHeight;
    for (const Neuron::UiHandle handle : m_lines)
    {
      _window.SetRect(handle, Neuron::UiRect::FromSize(box.left + inset, top, width, step));
      _window.SetFontPixels(handle, _grid.fontSmall);
      top += step;
    }
  }

private:
  void Show(Neuron::UiWindow& _window, std::size_t _section)
  {
    const EpilogueSection& section = m_text.sections[_section];
    _window.SetText(m_heading, section.title);
    for (std::size_t line = 0; line < m_lines.size(); ++line)
    {
      _window.SetText(m_lines[line], line < section.lines.size() ? section.lines[line] : std::string{});
    }
  }

  WorldMenu& m_menu;
  bool m_away;
  EpilogueText m_text;
  std::vector<Neuron::UiHandle> m_rows;
  Neuron::UiHandle m_leave;
  Neuron::UiHandle m_heading;
  std::vector<Neuron::UiHandle> m_lines;
};

} // namespace

EpilogueText AwayReportTextOf(const AwayReport& _report, double _tickSeconds, const Neuron::StringTable& _strings,
                              const Neuron::MessageLocale& _locale)
{
  using Argument = Neuron::FormatArgument;
  const auto format = [&](std::string_view _id, std::span<const Argument> _arguments) { return _strings.Format(_id, _arguments, _locale); };
  // A span of world time as hours and two-digit minutes.
  const auto clock = [_tickSeconds](std::uint64_t _ticks)
  {
    const auto minutes = static_cast<std::int64_t>(static_cast<double>(_ticks) * _tickSeconds / SECONDS_PER_MINUTE);
    return std::pair{minutes / MINUTES_PER_HOUR, std::format("{:02}", minutes % MINUTES_PER_HOUR)};
  };
  // At most AWAY_PAGE_LINES_MAX lines, the last counting the lines left out.
  const auto keep = [&](std::vector<std::string>& _lines, std::size_t _more)
  {
    const std::size_t total = _lines.size() + _more;
    if (total > AWAY_PAGE_LINES_MAX)
    {
      _lines.resize(AWAY_PAGE_LINES_MAX - 1);
      const std::array more{Argument{"count", static_cast<std::int64_t>(total - _lines.size())}};
      _lines.push_back(format(STRING_AWAY_MORE, more));
    }
  };
  EpilogueText text;
  // PIL-073: the summary on one page.
  EpilogueSection summary{format(STRING_AWAY_SUMMARY, {}), {}};
  const auto [hours, minutes] = clock(_report.toTick - _report.fromTick);
  const std::array length{Argument{"hours", hours}, Argument{"minutes", std::string_view(minutes)}};
  summary.lines.push_back(format(STRING_AWAY_LENGTH, length));
  const std::array workers{Argument{"from", std::int64_t{_report.workersFrom}}, Argument{"to", std::int64_t{_report.workersTo}}};
  summary.lines.push_back(format(STRING_AWAY_WORKERS, workers));
  const std::array stores{Argument{"from", _report.storesFromMilliRu / MILLI}, Argument{"to", _report.storesToMilliRu / MILLI}};
  summary.lines.push_back(format(STRING_AWAY_STORES, stores));
  const std::array built{Argument{"dwellings", std::int64_t{_report.structuresCompleted[0]}},
                         Argument{"storehouses", std::int64_t{_report.structuresCompleted[1]}},
                         Argument{"turrets", std::int64_t{_report.structuresCompleted[2]}}};
  summary.lines.push_back(format(STRING_AWAY_BUILT, built));
  if (!_report.HasNews())
  {
    summary.lines.push_back(format(STRING_AWAY_QUIET, {}));
  }
  for (const AwayBattleLine& battle : _report.battles)
  {
    const std::string name = CladanName(battle.other, _strings, _locale);
    const std::array arguments{Argument{"won", std::string_view(battle.won ? "yes" : "no")},
                               Argument{"defended", std::string_view(battle.defended ? "yes" : "no")},
                               Argument{"cladan", std::string_view(name)}, Argument{"lost", std::int64_t{battle.workersLost}}};
    summary.lines.push_back(format(STRING_AWAY_BATTLE, arguments));
  }
  for (const AwayClaimLine& claim : _report.claims)
  {
    const std::string area = AreaName(claim.area, _strings, _locale);
    const std::string name = CladanName(claim.to, _strings, _locale);
    const std::array arguments{Argument{"area", std::string_view(area)}, Argument{"cladan", std::string_view(name)}};
    summary.lines.push_back(format(STRING_AWAY_CLAIM, arguments));
  }
  if (_report.grievancesTotal > 0)
  {
    const std::array arguments{Argument{"count", std::int64_t{_report.grievancesTotal}}};
    summary.lines.push_back(format(STRING_AWAY_GRIEVANCES, arguments));
  }
  for (const AwayFreezeLine& freeze : _report.freezes)
  {
    const std::array arguments{Argument{"cause", FREEZE_SELECTORS.at(static_cast<std::size_t>(freeze.cause))},
                               Argument{"minutes", static_cast<std::int64_t>(freeze.seconds / 60)}};
    summary.lines.push_back(format(STRING_AWAY_FREEZE, arguments));
  }
  keep(summary.lines, 0);
  text.sections.push_back(std::move(summary));
  // PIL-026: every deputy action, with the standing order that permitted it, one step away.
  EpilogueSection deputy{format(STRING_AWAY_DEPUTY, {}), {}};
  for (const AwayActionLine& action : _report.actions)
  {
    const auto [actionHours, actionMinutes] = clock(action.tick - _report.fromTick);
    const std::string name = CladanName(action.other, _strings, _locale);
    const std::array arguments{Argument{"hours", actionHours}, Argument{"minutes", std::string_view(actionMinutes)},
                               Argument{"cladan", std::string_view(name)},
                               Argument{"order", std::string_view(action.order == StandingOrder::TheStewardsAttack ? "attack" : "defend")}};
    deputy.lines.push_back(format(STRING_AWAY_ACTION, arguments));
  }
  if (deputy.lines.empty())
  {
    deputy.lines.push_back(format(STRING_AWAY_NO_ACTION, {}));
  }
  keep(deputy.lines, _report.actionsTotal - static_cast<std::uint32_t>(_report.actions.size()));
  text.sections.push_back(std::move(deputy));
  return text;
}

EpilogueText EpilogueTextOf(const Epilogue& _epilogue, double _tickSeconds, const Neuron::StringTable& _strings,
                            const Neuron::MessageLocale& _locale)
{
  using Argument = Neuron::FormatArgument;
  const auto format = [&](std::string_view _id, std::span<const Argument> _arguments) { return _strings.Format(_id, _arguments, _locale); };
  EpilogueText text;
  text.sections.push_back({format(STRING_STANDING, {}), {}});
  std::int64_t place = 0;
  for (const EpilogueLine& line : _epilogue.settlements)
  {
    // MVP-062: the final standing, each settlement's renown and its sources.
    const std::string name = CladanName(line.site, _strings, _locale);
    const std::array standing{Argument{"place", ++place}, Argument{"cladan", std::string_view(name)},
                              Argument{"total", RenownTotal(line.renown)}};
    const std::array sources{Argument{"defense", std::int64_t{line.renown.defense}}, Argument{"claims", std::int64_t{line.renown.claims}},
                             Argument{"easing", std::int64_t{line.renown.easing}},
                             Argument{"structures", std::int64_t{line.renown.structures}}};
    text.sections.front().lines.push_back(format(STRING_PLACE, standing));
    text.sections.front().lines.push_back(format(STRING_SOURCES, sources));
    // PIL-125: its history, the steward's settlement's (its site its team, ADR-130) led by the statistics of WLD-224.
    EpilogueSection history;
    const bool own = line.site == _epilogue.team;
    const std::array named{Argument{"cladan", std::string_view(name)}};
    history.title = own ? format(STRING_YOURS, named) : name;
    if (own)
    {
      const auto gapMinutes = std::llround(static_cast<double>(_epilogue.largestGapTicks) * _tickSeconds / SECONDS_PER_MINUTE);
      const std::array growth{Argument{"steps", std::int64_t{_epilogue.growthSteps}}, Argument{"minutes", std::int64_t{gapMinutes}}};
      history.lines.push_back(format(STRING_GROWTH, growth));
      const auto contactMinutes =
        static_cast<std::int64_t>(static_cast<double>(_epilogue.firstHalfwayTick) * _tickSeconds / SECONDS_PER_MINUTE);
      const std::string minutes = std::format("{:02}", contactMinutes % MINUTES_PER_HOUR);
      const std::array contact{Argument{"reached", std::string_view(_epilogue.firstHalfwayTick != 0 ? "yes" : "no")},
                               Argument{"hours", contactMinutes / MINUTES_PER_HOUR}, Argument{"minutes", std::string_view(minutes)}};
      history.lines.push_back(format(STRING_CONTACT, contact));
    }
    const std::array battles{Argument{"won", std::int64_t{line.battlesWon}}, Argument{"lost", std::int64_t{line.battlesLost}}};
    history.lines.push_back(format(STRING_BATTLES, battles));
    const std::array claims{Argument{"made", std::int64_t{line.claimsMade}}, Argument{"won", std::int64_t{line.claimsWon}},
                            Argument{"lost", std::int64_t{line.claimsLost}}, Argument{"withdrawn", std::int64_t{line.claimsWithdrawn}}};
    history.lines.push_back(format(STRING_CLAIMS, claims));
    const std::array structures{Argument{"structures", std::int64_t{line.structures}},
                                Argument{"grievances", std::int64_t{line.grievances}}};
    history.lines.push_back(format(STRING_STRUCTURES, structures));
    const std::array workers{Argument{"most", std::int64_t{line.workersMost}}, Argument{"last", std::int64_t{line.workersLast}}};
    history.lines.push_back(format(STRING_WORKERS, workers));
    const std::vector<std::int64_t>& strength = line.strengthMilliRu;
    const std::int64_t peak = strength.empty() ? 0 : *std::ranges::max_element(strength);
    const std::array over{Argument{"first", strength.empty() ? 0 : strength.front() / MILLI}, Argument{"peak", peak / MILLI},
                          Argument{"last", strength.empty() ? 0 : strength.back() / MILLI}};
    history.lines.push_back(format(STRING_STRENGTH, over));
    text.sections.push_back(std::move(history));
  }
  return text;
}

WorldMenu::WorldMenu(const Desc& _desc)
  : m_desc(_desc)
{
}

void WorldMenu::Update(ClientSession& _session)
{
  const std::optional<ReplicatedSettlement> own = _session.OwnSettlement();
  if (!own.has_value())
  {
    return;
  }
  m_site = own->site;
  // ADR-163: the valley status says when the run has ended; then the epilogue is asked for, once.
  const std::optional<ValleyStatus>& valley = _session.LatestValleyStatus();
  m_ended = m_ended || (valley.has_value() && valley->team == own->team && valley->runEnded);
  if (m_ended && !m_requested)
  {
    m_requested = _session.SendCommand(ReadEpilogueCommand{own->team}).has_value();
  }
  const std::optional<Epilogue>& epilogue = _session.LatestEpilogue();
  if (!m_epilogue.has_value() && epilogue.has_value() && epilogue->team == own->team && epilogue->runEnded)
  {
    m_epilogue = epilogue;
    m_ended = true;
    m_arrived = true;
  }
  // MVP-074 (ADR-191): in an always-on world, the latest away report is asked for once, for the menu's entry; a return
  // with news sends its own, which opens first (O110).
  if (_session.Pace() == WorldPace::AlwaysOn && !m_awayAsked)
  {
    m_awayAsked = _session.SendCommand(ReadAwayReportCommand{own->team}).has_value();
  }
  if (const std::optional<AwayReport>& away = _session.LatestAwayReport(); away.has_value() && away->team == own->team && away->absence)
  {
    m_away = away;
  }
  m_awayArrived = _session.TakeAwayReportArrived() || m_awayArrived;
}

void WorldMenu::Reset() noexcept
{
  m_site.reset();
  m_ended = false;
  m_requested = false;
  m_arrived = false;
  m_epilogue.reset();
  m_away.reset();
  m_awayAsked = false;
  m_awayArrived = false;
  m_choice.reset();
}

bool WorldMenu::TakeEpilogueArrived() noexcept
{
  return std::exchange(m_arrived, false);
}

bool WorldMenu::TakeAwayReportArrived() noexcept
{
  return std::exchange(m_awayArrived, false);
}

std::unique_ptr<Neuron::UiPage> WorldMenu::MakePage(WorldMenuPage _page)
{
  switch (_page)
  {
  case WorldMenuPage::Menu:
    return std::make_unique<MenuPage>(*this);
  case WorldMenuPage::Epilogue:
    return std::make_unique<EpiloguePage>(*this, false);
  case WorldMenuPage::AwayReport:
    return std::make_unique<EpiloguePage>(*this, true);
  }
  return nullptr;
}

std::optional<WorldMenuChoice> WorldMenu::TakeChoice() noexcept
{
  return std::exchange(m_choice, std::nullopt);
}

std::string WorldMenu::Text(std::string_view _id, std::span<const Neuron::FormatArgument> _arguments) const
{
  return m_desc.strings->Format(_id, _arguments, *m_desc.locale);
}

} // namespace Species
