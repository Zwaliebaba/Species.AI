#include "pch.h"

#include "SettlementHud.h"

#include "StructureCost.h"
#include "UiLook.h"
#include "UiText.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

namespace Species
{

namespace
{

// The HUD's strings (ADR-146, ADR-152), each in GameData/Strings/en_us.json.
constexpr std::string_view STRING_STORES = "ui.hud.stores";
constexpr std::string_view STRING_WORKERS = "ui.hud.workers";
constexpr std::string_view STRING_GROWTH = "ui.hud.growth";
constexpr std::string_view STRING_UPKEEP = "ui.hud.upkeep";
constexpr std::string_view STRING_INACTIVE = "ui.hud.inactive";
constexpr std::string_view STRING_PRIORITY = "ui.hud.priority";
constexpr std::string_view STRING_IDLE = "ui.hud.idle";
constexpr std::string_view STRING_IDLE_REASON = "ui.hud.idle_reason";
constexpr std::string_view STRING_WAITING = "ui.hud.waiting";
constexpr std::string_view STRING_BUILD = "ui.hud.build";
constexpr std::string_view STRING_COST = "ui.hud.cost";
constexpr std::string_view STRING_COMPLETED = "ui.hud.completed";
constexpr std::string_view STRING_SEED = "ui.hud.seed";

// The selectors of the strings' select arguments, in GrowthState's, WorkerTask's, IdleReason's and StructureKind's orders.
constexpr std::array<std::string_view, 3> GROWTH_SELECTORS{"growing", "at_limit", "stores_short"};
constexpr std::array<std::string_view, 3> TASK_SELECTORS{"gather", "construct", "guard"};
constexpr std::array<std::string_view, 4> IDLE_SELECTORS{"no_free_port", "queue_empty", "store_full", "share_met"};

constexpr std::array<UiImage, 3> PRIORITY_ICONS{UiImage::IconPriorityGather, UiImage::IconPriorityConstruct, UiImage::IconPriorityGuard};

// The rows of the settlement panel before its idle rows: the stores, the Workers, growth, the upkeep and the three
// priorities.
constexpr std::size_t FIXED_ROWS = 7;
// The gap between two notification cards and between the queue and the bar, in HUD units.
constexpr double CARD_GAP = 4.0;

std::string_view KindSelector(StructureKind _kind) noexcept
{
  switch (_kind)
  {
  case StructureKind::Dwelling:
    return "dwelling";
  case StructureKind::Storehouse:
    return "storehouse";
  case StructureKind::Turret:
    return "turret";
  case StructureKind::Wellspring:
  case StructureKind::ResourceSite:
  case StructureKind::ResourceSiteCentral:
    break;
  }
  return "other";
}

UiImage KindIcon(StructureKind _kind) noexcept
{
  return _kind == StructureKind::Dwelling     ? UiImage::IconDwelling
         : _kind == StructureKind::Storehouse ? UiImage::IconStorehouse
                                              : UiImage::IconTurret;
}

template <class Color> std::array<float, 4> Rgba(const Color& _color, double _alphaScale = 1.0) noexcept
{
  return {static_cast<float>(_color.r), static_cast<float>(_color.g), static_cast<float>(_color.b),
          static_cast<float>(_color.a * _alphaScale)};
}

Neuron::UiQuad Quad(const Neuron::UiRect& _rect, const std::array<float, 4>& _color,
                    const std::array<float, 4>& _uv = {0.0f, 0.0f, 1.0f, 1.0f})
{
  Neuron::UiQuad quad;
  quad.rectangle = {_rect.left, _rect.top, _rect.right, _rect.bottom};
  quad.uv = _uv;
  quad.color = _color;
  return quad;
}

std::int64_t WholeRu(std::int64_t _milliRu) noexcept
{
  return _milliRu / 1000;
}

std::int64_t DwellingsOf(const StateReplica& _replica, std::uint8_t _team)
{
  std::int64_t dwellings = 0;
  for (const auto& [id, state] : _replica.Latest())
  {
    const auto* structure = std::get_if<ReplicatedStructure>(&state);
    if (structure != nullptr && structure->kind == StructureKind::Dwelling && structure->owner == std::optional<std::uint8_t>(_team))
    {
      ++dwellings;
    }
  }
  return dwellings;
}

} // namespace

HudModel HudModelOf(const ClientSession& _session, const TuningMvp& _tuning, const TuningPresentation& _presentation,
                    std::vector<HudNotice> _notices, std::span<const SettlementNotice> _arrived, double _nowSeconds)
{
  HudModel model;
  model.nowSeconds = _nowSeconds;
  model.tickSeconds = _tuning.ref.sim.tickPeriod;
  model.settlement = _session.OwnSettlement();
  model.seed = _session.WorldSeed();
  const std::optional<SettlementStatus>& status = _session.LatestStatus();
  if (model.settlement.has_value() && status.has_value() && status->team == model.settlement->team)
  {
    model.status = status;
  }
  // MVP-034: the dwelling's cost grows with every dwelling the settlement has, built or queued, as the server counts it.
  const std::int64_t dwellings = model.settlement.has_value() ? DwellingsOf(_session.Replica(), model.settlement->team) : 0;
  for (std::size_t index = 0; index < BUILD_KINDS.size(); ++index)
  {
    model.buildCostsMilliRu[index] = StructureCostMilliRu(_tuning, BUILD_KINDS[index], dwellings);
  }
  model.notices = AgeNotices(std::move(_notices), _arrived, _nowSeconds, _presentation);
  return model;
}

std::vector<HudNotice> AgeNotices(std::vector<HudNotice> _notices, std::span<const SettlementNotice> _arrived, double _nowSeconds,
                                  const TuningPresentation& _presentation)
{
  const auto& hud = _presentation.mvp.hud;
  for (const SettlementNotice& notice : _arrived)
  {
    _notices.push_back({notice, _nowSeconds});
  }
  std::erase_if(_notices, [&](const HudNotice& _notice) { return _nowSeconds - _notice.arrivedSeconds >= hud.noticeSeconds; });
  const auto kept = static_cast<std::size_t>(std::max<std::int64_t>(hud.noticesMax, 0));
  if (_notices.size() > kept)
  {
    _notices.erase(_notices.begin(), _notices.end() - static_cast<std::ptrdiff_t>(kept));
  }
  return _notices;
}

SettlementHud::SettlementHud(const TuningPresentation& _presentation, const TuningMvp& _tuning)
  : m_presentation(_presentation),
    m_tuning(_tuning)
{
}

float SettlementHud::Units(double _hudUnits) const noexcept
{
  return static_cast<float>(
    std::round(_hudUnits * static_cast<double>(m_metrics.HudUnitPixels()) * static_cast<double>(m_metrics.uiScale)));
}

void SettlementHud::Update(ClientSession& _session, double _realSeconds, const Neuron::UiMetrics& _metrics)
{
  const std::vector<SettlementNotice> arrived = _session.TakeNotices();
  m_model = HudModelOf(_session, m_tuning, m_presentation, std::move(m_model.notices), arrived, _realSeconds);
  // MVP-098: the status is server data, asked for once a period while the client has a settlement.
  if (m_model.settlement.has_value() &&
      (!m_lastRequest.has_value() || _realSeconds - *m_lastRequest >= m_presentation.mvp.hud.statusPeriod || _realSeconds < *m_lastRequest))
  {
    ReadSettlementStatusCommand request;
    request.team = m_model.settlement->team;
    if (_session.SendCommand(request).has_value())
    {
      m_lastRequest = _realSeconds;
    }
  }
  Layout(_metrics);
}

void SettlementHud::SetModel(HudModel _model, const Neuron::UiMetrics& _metrics)
{
  m_model = std::move(_model);
  Layout(_metrics);
}

void SettlementHud::Layout(const Neuron::UiMetrics& _metrics)
{
  m_metrics = _metrics;
  m_zones.clear();
  if (!m_model.settlement.has_value())
  {
    return;
  }
  const auto& hud = m_presentation.mvp.hud;
  const auto width = static_cast<float>(_metrics.client.width);
  const auto height = static_cast<float>(_metrics.client.height);
  const float margin = Units(hud.margin);
  const float padding = Units(hud.padding);
  const float row = Units(hud.rowHeight);

  // The settlement panel, top left: the fixed rows, then a heading and a row for each idle reason with Workers.
  std::size_t idleRows = 1;
  if (m_model.status.has_value())
  {
    idleRows += static_cast<std::size_t>(std::ranges::count_if(m_model.status->idle, [](std::uint32_t _count) { return _count > 0; }));
  }
  m_panelRows = m_model.status.has_value() ? FIXED_ROWS + idleRows : 3;
  m_panel = Neuron::UiRect::FromSize(margin, margin, Units(hud.panelWidth), 2.0f * padding + row * static_cast<float>(m_panelRows));
  m_zones.push_back({HudZone::Settlement, 0, m_panel});

  // The construction bar, bottom centre, and the queue's row above it.
  const float buttonWidth = Units(hud.barButtonWidth);
  const float barHeight = Units(hud.barHeight);
  const float gap = Units(CARD_GAP);
  const float barWidth = buttonWidth * static_cast<float>(BUILD_KINDS.size()) + gap * static_cast<float>(BUILD_KINDS.size() - 1);
  const float barLeft = std::round((width - barWidth) * 0.5f);
  const float barTop = height - margin - barHeight;
  for (std::size_t index = 0; index < BUILD_KINDS.size(); ++index)
  {
    m_build[index] = Neuron::UiRect::FromSize(barLeft + static_cast<float>(index) * (buttonWidth + gap), barTop, buttonWidth, barHeight);
    m_zones.push_back({HudZone::Build, static_cast<std::uint8_t>(index), m_build[index]});
  }
  const std::size_t sites = m_model.status.has_value() ? m_model.status->queue.size() : 0;
  const float site = Units(hud.queueSize);
  const float queueWidth = sites > 0 ? site * static_cast<float>(sites) + gap * static_cast<float>(sites - 1) : 0.0f;
  m_queue = Neuron::UiRect::FromSize(std::round((width - queueWidth) * 0.5f), barTop - gap - site, queueWidth, site);
  if (sites > 0)
  {
    m_zones.push_back({HudZone::Queue, 0, m_queue});
  }

  // The notification cards, top right, newest first.
  if (!m_model.notices.empty())
  {
    const float cardWidth = Units(hud.cardWidth);
    const float cardHeight = Units(hud.cardHeight);
    const auto cards = static_cast<float>(m_model.notices.size());
    m_zones.push_back({HudZone::Notices, 0,
                       Neuron::UiRect::FromSize(width - margin - cardWidth, margin, cardWidth, cards * cardHeight + (cards - 1.0f) * gap)});
  }

  // The world's panel, bottom left.
  m_world = Neuron::UiRect::FromSize(margin, height - margin - row - 2.0f * padding, Units(hud.panelWidth), row + 2.0f * padding);
  m_zones.push_back({HudZone::World, 0, m_world});
}

std::optional<HudZoneRect> SettlementHud::ZoneAt(Neuron::PointPx _position) const noexcept
{
  for (const HudZoneRect& zone : m_zones)
  {
    if (zone.rect.Contains(static_cast<float>(_position.x), static_cast<float>(_position.y)))
    {
      return zone;
    }
  }
  return std::nullopt;
}

void SettlementHud::Draw(Neuron::TextSystem& _text, const Neuron::StringTable& _strings, const Neuron::MessageLocale& _locale,
                         std::vector<Neuron::UiBatch>& _out) const
{
  if (!m_model.settlement.has_value())
  {
    return;
  }
  const auto& hud = m_presentation.mvp.hud;
  const auto& palette = hud.palette;
  const float padding = Units(hud.padding);
  const float row = Units(hud.rowHeight);
  const float icon = Units(hud.iconSize);
  const float meterHeight = std::max(Units(hud.meterHeight), 1.0f);
  const float textPixels = m_metrics.TextPixels(hud.textSize * static_cast<double>(m_metrics.HudUnitPixels()), true);
  const auto repeat = static_cast<float>(m_presentation.ui.chrome.bodyRepeat);
  const std::array<float, 4> text = Rgba(palette.text);
  const std::array<float, 4> positive = Rgba(palette.positive);
  const std::array<float, 4> negative = Rgba(palette.negative);

  const auto image = [&](UiImage _image, const Neuron::UiRect& _rect, const std::array<float, 4>& _color,
                         const std::array<float, 4>& _uv = {0.0f, 0.0f, 1.0f, 1.0f})
  {
    Neuron::AppendQuad(_out, Neuron::UiSource::Image, Neuron::UiBlend::Alpha, static_cast<std::uint16_t>(_image), Quad(_rect, _color, _uv));
  };
  const auto panel = [&](UiImage _strip, const Neuron::UiRect& _rect, const std::array<float, 4>& _tint)
  { image(_strip, _rect, _tint, {0.0f, 0.0f, repeat, 1.0f}); };
  const auto line = [&](std::string_view _id, std::span<const Neuron::FormatArgument> _arguments, float _left, float _top, float _height,
                        const std::array<float, 4>& _color, bool _rightAligned = false, float _right = 0.0f)
  {
    const std::string formatted = _strings.Format(_id, _arguments, _locale);
    const Neuron::TextLine laid = _text.Layout(formatted, Neuron::FontRole::Body, textPixels);
    Neuron::TextStyle style;
    style.color = _color;
    const float left = _rightAligned ? std::round(_right - laid.widthPixels) : _left;
    Neuron::AppendText(_out, laid, left, Neuron::RowTextTop(_top, _height, laid.heightPixels), style);
  };
  const auto meter = [&](float _left, float _top, float _width, double _fraction)
  {
    const Neuron::UiRect track = Neuron::UiRect::FromSize(_left, _top, _width, meterHeight);
    image(UiImage::HudMeter, track, Rgba(palette.meterTrack));
    const auto filled = static_cast<float>(std::round(static_cast<double>(_width) * std::clamp(_fraction, 0.0, 1.0)));
    if (filled > 0.0f)
    {
      image(UiImage::HudMeter, Neuron::UiRect::FromSize(_left, _top, filled, meterHeight), Rgba(palette.meterFill));
    }
  };

  // The settlement panel (UI-460's accent strip).
  panel(UiImage::PanelAccent, m_panel, Rgba(palette.accent));
  const float iconLeft = m_panel.left + padding;
  const float textLeft = iconLeft + icon + padding;
  const float meterWidth = m_panel.right - padding - textLeft;
  float top = m_panel.top + padding;
  const auto rowIcon = [&](UiImage _icon)
  { image(_icon, Neuron::UiRect::FromSize(iconLeft, std::round(top + (row - icon) * 0.5f), icon, icon), text); };
  const ReplicatedSettlement& own = *m_model.settlement;
  // MVP-030, MVP-021: the stores and their capacity, in whole resource units, from the replica's latest tick.
  {
    rowIcon(UiImage::IconResource);
    const std::array arguments{Neuron::FormatArgument{"stores", WholeRu(static_cast<std::int64_t>(own.storesMilliRu))},
                               Neuron::FormatArgument{"capacity", WholeRu(static_cast<std::int64_t>(own.capacityMilliRu))}};
    line(STRING_STORES, arguments, textLeft, top, row - meterHeight, text);
    meter(textLeft, top + row - meterHeight - 1.0f, meterWidth,
          own.capacityMilliRu > 0 ? static_cast<double>(own.storesMilliRu) / static_cast<double>(own.capacityMilliRu) : 0.0);
    top += row;
  }
  // MVP-022: the Workers and the population limit.
  {
    rowIcon(UiImage::IconWorkers);
    const std::array arguments{Neuron::FormatArgument{"population", static_cast<std::int64_t>(own.population)},
                               Neuron::FormatArgument{"limit", static_cast<std::int64_t>(own.limit)}};
    line(STRING_WORKERS, arguments, textLeft, top, row - meterHeight, text);
    meter(textLeft, top + row - meterHeight - 1.0f, meterWidth,
          own.limit > 0 ? static_cast<double>(own.population) / static_cast<double>(own.limit) : 0.0);
    top += row;
  }
  if (!m_model.status.has_value())
  {
    line(STRING_WAITING, {}, textLeft, top, row, text);
  }
  else
  {
    const SettlementStatus& status = *m_model.status;
    // MVP-024, MVP-035, MVP-102: growth, or the cause when it stops.
    rowIcon(UiImage::IconGrowth);
    const auto growth = static_cast<std::size_t>(status.growth);
    const auto seconds = static_cast<std::int64_t>(std::ceil(static_cast<double>(status.growthTicks) * m_model.tickSeconds));
    const std::array growthArguments{Neuron::FormatArgument{"state", GROWTH_SELECTORS.at(std::min(growth, GROWTH_SELECTORS.size() - 1))},
                                     Neuron::FormatArgument{"seconds", seconds}};
    line(STRING_GROWTH, growthArguments, textLeft, top, row, status.growth == GrowthState::Growing ? positive : negative);
    top += row;
    // MVP-035: the upkeep an hour, and the structures it made inactive.
    const std::array upkeep{Neuron::FormatArgument{"upkeep", WholeRu(status.upkeepMilliRuPerHour)}};
    line(STRING_UPKEEP, upkeep, textLeft, top, row, text);
    if (status.inactiveStructures > 0)
    {
      const std::array inactive{Neuron::FormatArgument{"count", static_cast<std::int64_t>(status.inactiveStructures)}};
      line(STRING_INACTIVE, inactive, 0.0f, top, row, negative, true, m_panel.right - padding);
    }
    top += row;
    // MVP-036: each priority's share with the Workers that hold its task.
    for (std::size_t task = 0; task < TASK_SELECTORS.size(); ++task)
    {
      rowIcon(PRIORITY_ICONS[task]);
      const std::array arguments{Neuron::FormatArgument{"task", TASK_SELECTORS[task]},
                                 Neuron::FormatArgument{"share", static_cast<std::int64_t>(status.priorities[task])},
                                 Neuron::FormatArgument{"workers", static_cast<std::int64_t>(status.tasks[task])}};
      line(STRING_PRIORITY, arguments, textLeft, top, row - meterHeight, text);
      meter(textLeft, top + row - meterHeight - 1.0f, meterWidth, static_cast<double>(status.priorities[task]) / 100.0);
      top += row;
    }
    // MVP-037: the idle Workers, by reason.
    std::int64_t idle = 0;
    for (const std::uint32_t count : status.idle)
    {
      idle += count;
    }
    const std::array idleArguments{Neuron::FormatArgument{"count", idle}};
    line(STRING_IDLE, idleArguments, iconLeft, top, row, text);
    top += row;
    for (std::size_t reason = 0; reason < IDLE_SELECTORS.size(); ++reason)
    {
      if (status.idle[reason] == 0)
      {
        continue;
      }
      const std::array arguments{Neuron::FormatArgument{"reason", IDLE_SELECTORS[reason]},
                                 Neuron::FormatArgument{"count", static_cast<std::int64_t>(status.idle[reason])}};
      line(STRING_IDLE_REASON, arguments, textLeft, top, row, negative);
      top += row;
    }
  }

  // MVP-032, MVP-034: the construction bar, each structure with its cost, red while the stores cannot pay it.
  for (std::size_t index = 0; index < BUILD_KINDS.size(); ++index)
  {
    const Neuron::UiRect& button = m_build[index];
    panel(UiImage::PanelNeutral, button, Rgba(palette.neutral));
    image(KindIcon(BUILD_KINDS[index]),
          Neuron::UiRect::FromSize(button.left + padding, std::round(button.top + (button.Height() - icon) * 0.5f), icon, icon), text);
    const float left = button.left + 2.0f * padding + icon;
    const float half = std::round(button.Height() * 0.5f);
    const std::array name{Neuron::FormatArgument{"kind", KindSelector(BUILD_KINDS[index])}};
    line(STRING_BUILD, name, left, button.top, half, text);
    const std::int64_t cost = m_model.buildCostsMilliRu[index];
    const std::array costArguments{Neuron::FormatArgument{"cost", WholeRu(cost)}};
    line(STRING_COST, costArguments, left, button.top + half, button.Height() - half,
         static_cast<std::int64_t>(own.storesMilliRu) >= cost ? positive : negative);
  }
  // MVP-032, MVP-033: the queue, each site with its progress, in the order it is built.
  if (m_model.status.has_value())
  {
    const float site = m_queue.Height();
    const float gap = Units(CARD_GAP);
    float left = m_queue.left;
    for (const QueuedSite& queued : m_model.status->queue)
    {
      const Neuron::UiRect cell = Neuron::UiRect::FromSize(left, m_queue.top, site, site);
      panel(UiImage::PanelNeutral, cell, Rgba(palette.neutral));
      const float inset = std::round((site - icon) * 0.5f);
      image(KindIcon(queued.kind), Neuron::UiRect::FromSize(cell.left + inset, cell.top + inset - meterHeight, icon, icon), text);
      meter(cell.left + 2.0f, cell.bottom - meterHeight - 2.0f, site - 4.0f, static_cast<double>(queued.progressPermille) / 1000.0);
      left += site + gap;
    }
  }

  // MVP-102: the notification cards, newest first, fading out over their last mvp.hud.notice_fade.
  if (!m_model.notices.empty())
  {
    const float cardWidth = Units(hud.cardWidth);
    const float cardHeight = Units(hud.cardHeight);
    const float gap = Units(CARD_GAP);
    float cardTop = Units(hud.margin);
    for (auto notice = m_model.notices.rbegin(); notice != m_model.notices.rend(); ++notice)
    {
      const double left = hud.noticeSeconds - (m_model.nowSeconds - notice->arrivedSeconds);
      const double alpha = hud.noticeFade > 0.0 ? std::clamp(left / hud.noticeFade, 0.0, 1.0) : 1.0;
      const Neuron::UiRect card = Neuron::UiRect::FromSize(static_cast<float>(m_metrics.client.width) - Units(hud.margin) - cardWidth,
                                                           cardTop, cardWidth, cardHeight);
      image(UiImage::NotificationCard, card, Rgba(palette.neutral, alpha));
      image(KindIcon(notice->notice.structure),
            Neuron::UiRect::FromSize(card.left + padding, std::round(card.top + (cardHeight - icon) * 0.5f), icon, icon),
            Rgba(palette.text, alpha));
      const std::array arguments{Neuron::FormatArgument{"kind", KindSelector(notice->notice.structure)}};
      line(STRING_COMPLETED, arguments, card.left + 2.0f * padding + icon, card.top, cardHeight, Rgba(palette.positive, alpha));
      cardTop += cardHeight + gap;
    }
  }

  // MVP-005: the world's seed, as digits that can be typed again.
  panel(UiImage::PanelAccent, m_world, Rgba(palette.accent));
  if (m_model.seed.has_value())
  {
    const std::string seed = std::to_string(*m_model.seed);
    const std::array arguments{Neuron::FormatArgument{"seed", std::string_view(seed)}};
    line(STRING_SEED, arguments, m_world.left + padding, m_world.top + padding, m_world.Height() - 2.0f * padding, text);
  }
}

} // namespace Species
