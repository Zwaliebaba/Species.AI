#include "pch.h"

#include "DevelopmentOverlay.h"
#include "RelationMessages.h"
#include "TextSystem.h"
#include "TuningMvp.h"
#include "TuningPresentation.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <deque>
#include <format>
#include <utility>
#include <variant>

namespace Species
{

namespace
{

// R-SIM-415: the base colours of the eight player colours, by colour id, in RGB from 0 to 255: ref.sim.team_color.green.base,
// .red.base, .yellow.base, .blue.base, .orange.base, .purple.base, .cyan.base and .pink.base. They are constants that cite
// their keys, because a record field named `base` would generate a type the R2 checker refuses (ADR-116, ADR-118).
constexpr std::array<std::array<std::uint8_t, 3>, 8> TEAM_BASE_COLORS{{
  {100, 255, 100},
  {255, 50, 50},
  {255, 255, 50},
  {120, 180, 255},
  {255, 150, 0},
  {150, 0, 255},
  {70, 200, 200},
  {255, 150, 255},
}};
constexpr float COLOR_LEVELS = 255.0f;

// ADR-120: the frame page's layout, in unscaled pixels from the target's top-left corner, and its colours. ADR-176: the
// page's top is also its gap under the HUD's settlement panel, and its backdrop reaches past its content by its padding.
constexpr float PAGE_LEFT_PIXELS = 10.0f;
constexpr float PAGE_TOP_PIXELS = 10.0f;
constexpr float LINE_GAP_PIXELS = 2.0f;
constexpr float GRAPH_GAP_PIXELS = 8.0f;
constexpr float LABEL_GAP_PIXELS = 6.0f;
constexpr float BACKDROP_PADDING_PIXELS = 6.0f;
constexpr std::array<float, 4> TEXT_COLOR{1.0f, 1.0f, 1.0f, 1.0f};
constexpr std::array<float, 4> PAGE_BACKDROP_COLOR{0.0f, 0.0f, 0.0f, 0.6f};
constexpr std::array<float, 4> GRAPH_BACKDROP_COLOR{0.0f, 0.0f, 0.0f, 0.5f};
constexpr std::array<float, 4> GRAPH_GUIDE_COLOR{0.6f, 0.6f, 0.6f, 1.0f};
constexpr std::array<float, 4> GRAPH_BAR_COLOR{1.0f, 0.0f, 0.0f, 1.0f}; // DBG-089: one red line per frame
// The parts of a page line (ADR-118).
constexpr const char* SEPARATOR = ", ";

// DBG-097: dbg.overlay.warning_rgb, from 0 to 255.
std::array<float, 4> WarningColor(const TuningPresentation& _presentation) noexcept
{
  const TuningPresentation::Dbg::Overlay::WarningRgb& rgb = _presentation.dbg.overlay.warningRgb;
  return {static_cast<float>(rgb.r) / COLOR_LEVELS, static_cast<float>(rgb.g) / COLOR_LEVELS, static_cast<float>(rgb.b) / COLOR_LEVELS,
          1.0f};
}

// The three percentiles of dbg.telemetry.percentiles, of the frame times or of the frames' work.
std::array<double, 3> Percentiles(const TuningPresentation& _presentation) noexcept
{
  const TuningPresentation::Dbg::Telemetry::Percentiles& percentiles = _presentation.dbg.telemetry.percentiles;
  return {percentiles.first, percentiles.second, percentiles.third};
}

std::string PercentileText(const Neuron::FrameProfile& _profile, const TuningPresentation& _presentation, bool _work)
{
  std::string text;
  for (const double percent : Percentiles(_presentation))
  {
    const double milliseconds = _work ? _profile.WorkPercentileMilliseconds(percent) : _profile.FramePercentileMilliseconds(percent);
    text += std::format("{}p{} {:.1f}", text.empty() ? "" : SEPARATOR, std::llround(percent), milliseconds);
  }
  return text + " ms";
}

// RND-454: the frame's budget, a frame at the target rate; ADR-080 item 6: the tick's, half the tick period.
double FrameBudgetMilliseconds(const TuningPresentation& _presentation) noexcept
{
  return 1000.0 / _presentation.rnd.budget.targetFrameRate;
}

double TickBudgetMilliseconds(const TuningMvp& _tuning) noexcept
{
  return 0.5 * _tuning.ref.sim.tickPeriod * 1000.0;
}

std::string SidesText(const ReplicaCounts& _counts)
{
  std::string text;
  for (const SideCount& side : _counts.sides)
  {
    text += std::format("{}{}+{}", text.empty() ? "" : " : ", side.workers, side.leaders);
  }
  return text.empty() ? std::string("none") : text;
}

} // namespace

std::string FrameRateText(double _framesPerSecond, std::optional<std::size_t> _objects)
{
  // The strings ui.dev.overlay.frame_rate_entities and ui.dev.overlay.frame_rate, until the string table is data; English
  // only, as development text is (DBG-004).
  const long long framesPerSecond = std::llround(std::max(_framesPerSecond, 0.0));
  return _objects.has_value() ? std::format("{} fps · {} objects", framesPerSecond, *_objects) : std::format("{} fps", framesPerSecond);
}

Neuron::OverlayLine FrameRateLine(const TuningPresentation& _presentation, double _framesPerSecond, std::optional<std::size_t> _objects)
{
  Neuron::OverlayLine line;
  line.text = FrameRateText(_framesPerSecond, _objects);
  line.insetX = static_cast<float>(_presentation.dbg.overlay.frameRateRight);
  line.insetY = static_cast<float>(_presentation.dbg.overlay.frameRateBottom);
  return line;
}

std::array<float, 4> TeamColor(std::uint8_t _team) noexcept
{
  // ADR-167: an expedition's team (ADR-161) takes its settlement's colour, its site's.
  const std::size_t team = _team;
  const std::size_t color =
    team >= EXPEDITION_TEAM_FIRST && team < EXPEDITION_TEAM_FIRST + VALLEY_SETTLEMENTS_MAX ? team - EXPEDITION_TEAM_FIRST : team;
  if (color >= TEAM_BASE_COLORS.size())
  {
    return {1.0f, 1.0f, 1.0f, 1.0f};
  }
  const std::array<std::uint8_t, 3>& levels = TEAM_BASE_COLORS[color];
  return {static_cast<float>(levels[0]) / COLOR_LEVELS, static_cast<float>(levels[1]) / COLOR_LEVELS,
          static_cast<float>(levels[2]) / COLOR_LEVELS, 1.0f};
}

ReplicaCounts CountReplica(const StateReplica& _replica)
{
  ReplicaCounts counts;
  for (const auto& [id, state] : _replica.Latest())
  {
    if (const auto* figure = std::get_if<ReplicatedFigure>(&state); figure != nullptr)
    {
      if (figure->team >= counts.sides.size())
      {
        counts.sides.resize(std::size_t{figure->team} + 1);
      }
      if (!figure->dead)
      {
        SideCount& side = counts.sides[figure->team];
        ++(figure->kind == FigureKind::Leader ? side.leaders : side.workers);
      }
    }
    else if (std::holds_alternative<ReplicatedOrb>(state))
    {
      ++counts.orbs;
    }
    else
    {
      ++counts.effects;
    }
  }
  return counts;
}

FramePage BuildFramePage(const FramePageInputs& _inputs, const TuningPresentation& _presentation, const TuningMvp& _tuning,
                         Neuron::TextSystem& _text, std::optional<float> _belowPixels)
{
  FramePage page;
  const std::array<float, 4> warning = WarningColor(_presentation);
  const auto textSize = static_cast<float>(_presentation.dbg.overlay.textSize);
  const float lineHeight = _text.Metrics(Neuron::FontRole::Development, textSize).heightPixels;
  const float pageTop = _belowPixels.value_or(0.0f) + PAGE_TOP_PIXELS;
  // ADR-176: the backdrop reaches the right end of the widest line or graph label.
  float right = PAGE_LEFT_PIXELS;
  const auto widthOf = [&_text, textSize](const std::string& _line)
  { return std::ceil(_text.Layout(_line, Neuron::FontRole::Development, textSize).widthPixels); };
  float top = pageTop;
  const auto line = [&page, &top, &right, &widthOf, lineHeight](const std::string& _line, const std::array<float, 4>& _color)
  {
    Neuron::OverlayLine added;
    added.text = _line;
    added.insetX = PAGE_LEFT_PIXELS;
    added.insetY = top;
    added.corner = Neuron::OverlayCorner::TopLeft;
    added.color = _color;
    right = std::max(right, PAGE_LEFT_PIXELS + widthOf(_line));
    page.lines.push_back(std::move(added));
    top += lineHeight + LINE_GAP_PIXELS;
  };
  // ADR-176: the backdrop, first so that everything else draws over it, sized once the page is laid out.
  page.rects.push_back({0.0f, 0.0f, 0.0f, 0.0f, PAGE_BACKDROP_COLOR});
  const auto backdrop = [&page, &right, pageTop](float _bottom)
  {
    page.rects.front() = {PAGE_LEFT_PIXELS - BACKDROP_PADDING_PIXELS, pageTop - BACKDROP_PADDING_PIXELS,
                          right - PAGE_LEFT_PIXELS + 2.0f * BACKDROP_PADDING_PIXELS, _bottom - pageTop + 2.0f * BACKDROP_PADDING_PIXELS,
                          PAGE_BACKDROP_COLOR};
  };
  // DBG-095 as ADR-176 amends it: in a world the frame-rate line is the page's first row.
  line(FrameRateText(_inputs.framesPerSecond, _inputs.objects), TEXT_COLOR);
  const Neuron::FrameProfile* profile = _inputs.profile;
  if (profile != nullptr)
  {
    // DBG-096: the frame time's percentiles; DBG-097: the frame's work against its budget (RND-454).
    line("frame " + PercentileText(*profile, _presentation, false), TEXT_COLOR);
    const double frameBudget = FrameBudgetMilliseconds(_presentation);
    const double workWorst = profile->WorkPercentileMilliseconds(Percentiles(_presentation)[2]);
    line(std::format("work {} of {:.1f}", PercentileText(*profile, _presentation, true), frameBudget),
         workWorst > frameBudget ? warning : TEXT_COLOR);
    std::string shares;
    for (std::size_t step = 0; step < CLIENT_STEP_NAMES.size(); ++step)
    {
      shares += std::format("{}{} {:.1f}%", shares.empty() ? "" : SEPARATOR, CLIENT_STEP_NAMES[step], profile->StepSharePercent(step));
    }
    line("steps " + shares, TEXT_COLOR);
  }
  for (std::size_t team = 0; team < _inputs.counts.sides.size(); ++team)
  {
    const SideCount& side = _inputs.counts.sides[team];
    line(std::format("team {} {} Workers{}{} Leaders", team, side.workers, SEPARATOR, side.leaders), TEXT_COLOR);
  }
  line(std::format("orbs {}{}effects {}", _inputs.counts.orbs, SEPARATOR, _inputs.counts.effects), TEXT_COLOR);
  if (_inputs.server.has_value())
  {
    // DBG-097: the server's longest tick of the period against half the tick period (ADR-080 item 6).
    const ServerStats& server = *_inputs.server;
    const double tickBudget = TickBudgetMilliseconds(_tuning);
    const double tickMax = static_cast<double>(server.tickMaxMicroseconds) / 1000.0;
    line(std::format("server seed {}{}tick {}{}{:.2f} ms mean{}{:.2f} ms max of {:.1f}", server.worldSeed, SEPARATOR, server.tick,
                     SEPARATOR, static_cast<double>(server.tickMeanMicroseconds) / 1000.0, SEPARATOR, tickMax, tickBudget),
         tickMax > tickBudget ? warning : TEXT_COLOR);
  }
  else
  {
    line("server: no statistics yet", TEXT_COLOR);
  }
  if (profile == nullptr)
  {
    backdrop(top - LINE_GAP_PIXELS);
    return page;
  }
  // DBG-089: the frame-time graph, one line per frame of the history at a pixel each, the newest at the right, under
  // guide lines at the marks, each labelled with its time and rate.
  const TuningPresentation::Dbg::Profile::View& view = _presentation.dbg.profile.view;
  const auto graphHeight = static_cast<float>(view.graphHeight);
  const auto graphWidth = static_cast<float>(std::max<std::int64_t>(_presentation.dbg.profile.frameHistory, 1));
  // On a whole pixel, as its guides are, since the development role's line height need not be one (ADR-176).
  const float graphTop = std::round(top - LINE_GAP_PIXELS + GRAPH_GAP_PIXELS);
  const float graphBottom = graphTop + graphHeight;
  page.rects.push_back({PAGE_LEFT_PIXELS, graphTop, graphWidth, graphHeight, GRAPH_BACKDROP_COLOR});
  right = std::max(right, PAGE_LEFT_PIXELS + graphWidth);
  const std::deque<double>& history = profile->FrameTimesMilliseconds();
  for (std::size_t frame = 0; frame < history.size() && static_cast<float>(frame) < graphWidth; ++frame)
  {
    const auto height = static_cast<float>(std::clamp(view.graphHeight * history[frame] / view.graphFullMs, 0.0, view.graphHeight));
    page.rects.push_back(
      {PAGE_LEFT_PIXELS + graphWidth - 1.0f - static_cast<float>(frame), graphBottom - height, 1.0f, height, GRAPH_BAR_COLOR});
  }
  for (const double mark : {view.graphMarksMs.first, view.graphMarksMs.second, view.graphMarksMs.third})
  {
    const float y = std::round(graphBottom - static_cast<float>(view.graphHeight * mark / view.graphFullMs));
    page.rects.push_back({PAGE_LEFT_PIXELS, y, graphWidth, 1.0f, GRAPH_GUIDE_COLOR});
    Neuron::OverlayLine label;
    label.text = std::format("{} ms ({} fps)", std::llround(mark), std::llround(1000.0 / mark));
    label.insetX = PAGE_LEFT_PIXELS + graphWidth + LABEL_GAP_PIXELS;
    label.insetY = std::round(y - lineHeight / 2.0f);
    label.corner = Neuron::OverlayCorner::TopLeft;
    label.color = GRAPH_GUIDE_COLOR;
    right = std::max(right, label.insetX + widthOf(label.text));
    page.lines.push_back(std::move(label));
  }
  backdrop(std::max(graphBottom, top - LINE_GAP_PIXELS));
  return page;
}

std::string StatisticsLine(const FramePageInputs& _inputs, const TuningPresentation& _presentation)
{
  std::string text = std::format("overlay: {} fps", std::llround(std::max(_inputs.framesPerSecond, 0.0)));
  if (_inputs.profile != nullptr)
  {
    text += std::format("; frame {}; work {}", PercentileText(*_inputs.profile, _presentation, false),
                        PercentileText(*_inputs.profile, _presentation, true));
  }
  text += std::format("; figures {}; orbs {}; effects {}", SidesText(_inputs.counts), _inputs.counts.orbs, _inputs.counts.effects);
  if (_inputs.server.has_value())
  {
    text += std::format("; server tick {}: {:.2f} ms mean, {:.2f} ms max", _inputs.server->tick,
                        static_cast<double>(_inputs.server->tickMeanMicroseconds) / 1000.0,
                        static_cast<double>(_inputs.server->tickMaxMicroseconds) / 1000.0);
  }
  return text;
}

std::vector<Neuron::DevelopmentDot> FigureDots(std::span<const InterpolatedFigure> _figures)
{
  std::vector<Neuron::DevelopmentDot> dots;
  dots.reserve(_figures.size());
  for (const InterpolatedFigure& figure : _figures)
  {
    if (figure.state.dead)
    {
      continue;
    }
    Neuron::DevelopmentDot dot;
    dot.position = figure.position + Neuron::Vec3{0.0, DOT_LIFT_WU, 0.0};
    dot.color = TeamColor(figure.state.team);
    dot.sidePixels = figure.state.kind == FigureKind::Leader ? LEADER_DOT_PIXELS : WORKER_DOT_PIXELS;
    dots.push_back(dot);
  }
  return dots;
}

} // namespace Species
