#include "pch.h"

#include "SettlementHud.h"

#include "DevelopmentOverlay.h"
#include "RegionTerrain.h"
#include "StructureCost.h"
#include "UiLook.h"
#include "UiText.h"

#include <algorithm>
#include <cmath>
#include <format>
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
constexpr std::string_view STRING_ALWAYS_ON = "ui.hud.always_on"; // ADR-191
constexpr std::string_view STRING_PLACING = "ui.hud.placing";     // ADR-153
constexpr std::string_view STRING_REFUSED = "ui.hud.refused";
// M3 task T11 (ADR-166): the relations panel and the notices of M3's kinds.
constexpr std::string_view STRING_CLADAN = "ui.hud.cladan";
constexpr std::string_view STRING_AREA = "ui.hud.area";
constexpr std::string_view STRING_RELATIONS = "ui.hud.relations";
constexpr std::string_view STRING_WARNING = "ui.hud.warning";
constexpr std::string_view STRING_EXPEDITION = "ui.hud.expedition";
constexpr std::string_view STRING_RESPITE = "ui.hud.respite";
constexpr std::string_view STRING_ANGER = "ui.hud.anger";
constexpr std::string_view STRING_GRIEVANCE = "ui.hud.grievance";
constexpr std::string_view STRING_CLAIMS = "ui.hud.claims";
constexpr std::string_view STRING_CLAIM = "ui.hud.claim";
constexpr std::string_view STRING_MORE = "ui.hud.more";
constexpr std::string_view STRING_NOTICE = "ui.hud.notice";
// M3 task T12 (ADR-167): a claimable area, the attack being declared and a conflict command's refusal.
constexpr std::string_view STRING_CLAIMABLE = "ui.hud.claimable";
constexpr std::string_view STRING_DECLARE = "ui.hud.declare";
constexpr std::string_view STRING_CONFLICT_REFUSED = "ui.hud.conflict_refused";
// ADR-168: the result card's lines and a public line's card.
constexpr std::string_view STRING_RESULT_TITLE = "ui.hud.result_title";
constexpr std::string_view STRING_RESULT_END = "ui.hud.result_end";
constexpr std::string_view STRING_RESULT_LOST = "ui.hud.result_lost";
constexpr std::string_view STRING_RESULT_STORES = "ui.hud.result_stores";
constexpr std::string_view STRING_RESULT_ESSENCE = "ui.hud.result_essence";
constexpr std::string_view STRING_RESULT_AREA = "ui.hud.result_area";
constexpr std::string_view STRING_RESULT_GRIEVANCE = "ui.hud.result_grievance";
constexpr std::string_view STRING_RESULT_RESPITE = "ui.hud.result_respite";
constexpr std::string_view STRING_RESULT_RENOWN = "ui.hud.result_renown";
constexpr std::string_view STRING_RESULT_CLOSE = "ui.hud.result_close";
constexpr std::string_view STRING_RESULT_PUBLIC = "ui.hud.result_public";
// BattleEnd's selectors in ui.hud.result_end, by its value.
constexpr std::array<std::string_view, 5> END_SELECTORS{"objective", "loss_cap", "rout", "withdrawn", "time"};
constexpr double SECONDS_PER_MINUTE = 60.0;
// ADR-169: the world panel's time left and standing.
constexpr std::string_view STRING_TIME_LEFT = "ui.hud.time_left";
constexpr std::string_view STRING_STANDING = "ui.hud.standing";
constexpr std::string_view STRING_RENOWN = "ui.hud.renown";
constexpr std::int64_t SECONDS_PER_HOUR = 3'600;

// The selectors of the strings' select arguments, in GrowthState's, WorkerTask's, IdleReason's and StructureKind's orders.
constexpr std::array<std::string_view, 4> GROWTH_SELECTORS{"growing", "at_limit", "stores_short", "held"};
constexpr std::array<std::string_view, 3> TASK_SELECTORS{"gather", "construct", "guard"};
constexpr std::array<std::string_view, 4> IDLE_SELECTORS{"no_free_port", "queue_empty", "store_full", "share_met"};
// PlacementRefusal's selectors, from NotBuildable (ADR-153).
constexpr std::array<std::string_view, PLACEMENT_REFUSAL_MAX> REFUSAL_SELECTORS{
  "not_buildable", "queue_full", "outside_territory", "in_center_ring", "not_walkable", "overlaps", "cannot_afford"};

// The Cladans by their colour, the site's index (MVP-013, ADR-130), and the selectors of GrievanceCause, ExpeditionState
// and ClaimState (ADR-166).
constexpr std::array<std::string_view, 4> CLADAN_SELECTORS{"green", "red", "yellow", "blue"};
constexpr std::array<std::string_view, 3> CAUSE_SELECTORS{"pressure", "claim", "attack"};
constexpr std::array<std::string_view, 3> EXPEDITION_SELECTORS{"held", "released", "fighting"};
constexpr std::array<std::string_view, 3> CLAIM_SELECTORS{"open", "waiting", "contested"};
constexpr std::array<UiImage, 3> CAUSE_ICONS{UiImage::IconContact, UiImage::IconClaim, UiImage::IconAttack};
// The refusals' selectors from their first value, 1, in ClaimRefusal's, AttackRefusal's and TributeRefusal's orders, and
// ConflictKind's (ADR-167).
constexpr std::array<std::string_view, 7> CLAIM_REFUSAL_SELECTORS{"no_settlement",   "not_claimable", "not_held",    "already_held",
                                                                  "already_claimed", "no_such_claim", "cooling_down"};
constexpr std::array<std::string_view, 8> ATTACK_REFUSAL_SELECTORS{"no_settlement", "underway",  "no_target",     "under_respite",
                                                                   "no_leader",     "too_large", "cannot_afford", "quiet"};
constexpr std::array<std::string_view, 4> TRIBUTE_REFUSAL_SELECTORS{"no_settlement", "no_grievance", "no_amount", "cannot_afford"};
constexpr std::array<std::string_view, 3> CONFLICT_SELECTORS{"claim", "attack", "tribute"};

constexpr std::array<UiImage, 3> PRIORITY_ICONS{UiImage::IconPriorityGather, UiImage::IconPriorityConstruct, UiImage::IconPriorityGuard};

// The rows of the settlement panel before its idle rows: the stores, the Workers, growth, the upkeep and the three
// priorities, which start at PRIORITY_ROW.
constexpr std::size_t FIXED_ROWS = 7;
constexpr std::size_t GROWTH_ROW = 2;
constexpr std::size_t ALWAYS_ON_ROW = 1; // the world panel's row after the seed's (ADR-191)
constexpr std::size_t PRIORITY_ROW = 4;
// The share a priority reaches at most (MVP-036).
constexpr std::uint8_t SHARE_MAX = 100;
// The gap between two notification cards and between the queue and the bar, in HUD units.
constexpr double CARD_GAP = 4.0;
// ADR-167: the relations rows a button's zone index can name, two buttons a row in 8 bits.
constexpr std::size_t RELATION_ROWS_ADDRESSED = 128;

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

// O106 (ADR-178): the growth the HUD shows: the hold sent while no status holds it yet, else the status's. A hold let go
// shows the timer the status gave, which waited while held.
GrowthState GrowthShown(const SettlementStatus& _status, const HudVerbs& _verbs) noexcept
{
  if (!_verbs.growthHeld.has_value())
  {
    return _status.growth;
  }
  if (*_verbs.growthHeld)
  {
    return GrowthState::Held;
  }
  return _status.growth == GrowthState::Held ? GrowthState::Growing : _status.growth;
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

std::string_view RefusalSelector(PlacementRefusal _refusal) noexcept
{
  const auto index = static_cast<std::size_t>(_refusal);
  return index >= 1 && index <= REFUSAL_SELECTORS.size() ? REFUSAL_SELECTORS[index - 1] : "other";
}

template <std::size_t N> std::string_view Selector(const std::array<std::string_view, N>& _selectors, std::size_t _index) noexcept
{
  return _index < N ? _selectors[_index] : std::string_view("other");
}

// A weight in whole units, from its millionths.
std::int64_t WholeWeight(std::int64_t _weightMicro) noexcept
{
  return static_cast<std::int64_t>(std::llround(static_cast<double>(_weightMicro) / 1e6));
}

// The ticks from _tick to _until in whole seconds, rounded up; 0 once it has passed.
std::int64_t SecondsUntil(std::uint64_t _until, std::uint64_t _tick, double _tickSeconds) noexcept
{
  return _until > _tick ? static_cast<std::int64_t>(std::ceil(static_cast<double>(_until - _tick) * _tickSeconds)) : 0;
}

/// MVP-102 (ADR-166): how a notification card shows a notice: its icon, its text's selector and whether it is good news
/// for the steward.
struct NoticeLook
{
  UiImage icon = UiImage::IconDwelling;
  std::string_view kind;
  bool positive = true;
};

// ADR-167: a relation action's button's icon.
UiImage ActionIcon(RelationAction _action) noexcept
{
  switch (_action)
  {
  case RelationAction::Raid:
  case RelationAction::ClaimBattle:
    return UiImage::IconAttack;
  case RelationAction::WithdrawClaim:
  case RelationAction::WithdrawAttack:
    return UiImage::IconWithdraw;
  case RelationAction::Tribute:
    return UiImage::IconTribute;
  case RelationAction::Claim:
  case RelationAction::None:
    break;
  }
  return UiImage::IconClaim;
}

std::string_view ConflictReasonSelector(ConflictKind _kind, std::uint8_t _reason) noexcept
{
  const std::size_t index = _reason > 0 ? std::size_t{_reason} - 1 : std::size_t{255};
  switch (_kind)
  {
  case ConflictKind::Claim:
    return Selector(CLAIM_REFUSAL_SELECTORS, index);
  case ConflictKind::Attack:
    return Selector(ATTACK_REFUSAL_SELECTORS, index);
  case ConflictKind::Tribute:
    return Selector(TRIBUTE_REFUSAL_SELECTORS, index);
  }
  return "other";
}

NoticeLook NoticeLookOf(const SettlementNotice& _notice) noexcept
{
  const bool ours = _notice.ours;
  switch (_notice.kind)
  {
  case NoticeKind::StructureCompleted:
    return {KindIcon(_notice.structure), "", true};
  case NoticeKind::Contact:
    return {UiImage::IconContact, "contact", true};
  case NoticeKind::ClaimMade:
    return {UiImage::IconClaim, ours ? "claim_ours" : "claim_theirs", ours};
  case NoticeKind::ClaimWithdrawn:
    return {UiImage::IconWithdraw, ours ? "withdrawn_ours" : "withdrawn_theirs", !ours};
  case NoticeKind::ClaimLost:
    return {UiImage::IconClaim, ours ? "lost_ours" : "lost_theirs", !ours};
  case NoticeKind::ClaimWon:
    return {UiImage::IconClaim, ours ? "won_ours" : "won_theirs", ours};
  case NoticeKind::AttackDeclared:
    return {UiImage::IconAttack, ours ? "attack_ours" : "attack_theirs", ours};
  case NoticeKind::AttackEnded:
    return {UiImage::IconWithdraw, ours ? "ended_ours" : "ended_theirs", !ours};
  case NoticeKind::BattleStarted:
    return {UiImage::IconBattle, "battle", true};
  case NoticeKind::BattleEnded:
  {
    // The detail is 1 when the attacker won; the steward's settlement is the attacker when it acted.
    const bool won = (_notice.detail == 1) == ours;
    return {UiImage::IconBattle, won ? "battle_won" : "battle_lost", won};
  }
  case NoticeKind::RespiteStarted:
    // The settlement under the respite acted; the attacker's steward learns that its target is under one.
    return {UiImage::IconRespite, ours ? "respite" : "respite_theirs", ours};
  case NoticeKind::RunQuiet:
    return {UiImage::IconRunClock, "quiet", true};
  case NoticeKind::RunEnded:
    return {UiImage::IconRunClock, "run_ended", true};
  }
  return {UiImage::IconBattle, "other", true};
}

} // namespace

std::int64_t TeamDwellings(const StateReplica::View& _latest, std::uint8_t _team)
{
  std::int64_t dwellings = 0;
  for (const auto& [id, state] : _latest)
  {
    const auto* structure = std::get_if<ReplicatedStructure>(&state);
    if (structure != nullptr && structure->kind == StructureKind::Dwelling && structure->owner == std::optional<std::uint8_t>(_team))
    {
      ++dwellings;
    }
  }
  return dwellings;
}

std::vector<RelationRow> RelationRowsOf(const ValleyStatus& _valley, std::uint8_t _site, std::uint64_t _tick, double _tickSeconds,
                                        double _warningSeconds, std::size_t _rowsMax)
{
  std::vector<RelationRow> rows{{RelationRowKind::Heading}};
  // MVP-042: each attack declared on the settlement, counting down to the warning's end; and its own expedition.
  const auto warningTicks = static_cast<std::uint64_t>(std::llround(_warningSeconds / std::max(_tickSeconds, 1e-9)));
  for (const ExpeditionLine& expedition : _valley.expeditions)
  {
    if (expedition.target == _site && expedition.state == ExpeditionState::Held)
    {
      rows.push_back(
        {RelationRowKind::Warning, expedition.attacker, SecondsUntil(expedition.declaredTick + warningTicks, _tick, _tickSeconds)});
    }
    else if (expedition.attacker == _site)
    {
      rows.push_back({RelationRowKind::Expedition, expedition.target, 0, static_cast<std::uint8_t>(expedition.state)});
    }
  }
  // MVP-047: the settlement's own respite, in whole minutes left, rounded up.
  const auto respiteOf = [&](std::uint8_t _of) { return std::ranges::find(_valley.respites, _of, &RespiteLine::site); };
  if (const auto own = respiteOf(_site); own != _valley.respites.end() && own->until > _tick)
  {
    rows.push_back({RelationRowKind::Respite, _site, (SecondsUntil(own->until, _tick, _tickSeconds) + 59) / 60});
  }
  // MVP-050: each other Cladan's anger toward it, the sum of the grievances listed under it.
  for (const RenownLine& settlement : _valley.renown)
  {
    if (settlement.site == _site)
    {
      continue;
    }
    std::int64_t angerMicro = 0;
    for (const GrievanceLine& grievance : _valley.grievances)
    {
      angerMicro += grievance.holder == settlement.site && grievance.against == _site ? grievance.weightMicro : 0;
    }
    const auto respite = respiteOf(settlement.site);
    rows.push_back({RelationRowKind::Cladan, settlement.site, WholeWeight(angerMicro), 0,
                    static_cast<std::uint8_t>(respite != _valley.respites.end() && respite->until > _tick ? 1 : 0)});
    for (const GrievanceLine& grievance : _valley.grievances)
    {
      if (grievance.holder == settlement.site && grievance.against == _site)
      {
        rows.push_back({RelationRowKind::Grievance, settlement.site, WholeWeight(grievance.weightMicro),
                        static_cast<std::uint8_t>(grievance.cause), static_cast<std::uint8_t>(grievance.causeRemoved ? 1 : 0),
                        grievance.subject, grievance.weightMicro});
      }
    }
  }
  // MVP-053: every claim, then each area the settlement may claim now (ADR-167): a half of its valley pair that its
  // neighbour holds, or the central resource site that it does not hold, which it does not already claim and whose
  // cooldown has passed (O102), as CheckClaim decides on the server.
  std::vector<RelationRow> claimable;
  for (std::uint8_t area = 0; area < VALLEY_AREAS; ++area)
  {
    const AreaLine& line = _valley.areas[area];
    const bool claimed =
      std::ranges::any_of(_valley.claims, [&](const ClaimLine& _claim) { return _claim.claimant == _site && _claim.area == area; });
    bool open = area == CLAIM_AREA_CENTRAL ? line.holder != std::optional<std::uint8_t>(_site) : false;
    if (area < CLAIM_AREA_CENTRAL)
    {
      const auto& pair = VALLEY_PAIRS[area / 2U];
      const bool ours = pair[0] == _site || pair[1] == _site;
      const std::uint8_t neighbor = pair[0] == _site ? pair[1] : pair[0];
      open = ours && line.holder == std::optional<std::uint8_t>(neighbor);
    }
    if (open && !claimed && line.claimableTick <= _tick)
    {
      claimable.push_back({RelationRowKind::Claimable, _site, 0, area});
    }
  }
  if (!_valley.claims.empty() || !claimable.empty())
  {
    rows.push_back({RelationRowKind::ClaimsHeading});
  }
  for (const ClaimLine& claim : _valley.claims)
  {
    rows.push_back({RelationRowKind::Claim, claim.claimant, 0, claim.area, static_cast<std::uint8_t>(claim.state), claim.id});
  }
  rows.insert(rows.end(), claimable.begin(), claimable.end());
  if (_rowsMax > 0 && rows.size() > _rowsMax)
  {
    const auto left = static_cast<std::int64_t>(rows.size() - (_rowsMax - 1));
    rows.resize(_rowsMax - 1);
    rows.push_back({RelationRowKind::More, 0, left});
  }
  return rows;
}

std::array<RelationAction, 2> RelationActionsOf(const RelationRow& _row, std::uint8_t _site) noexcept
{
  switch (_row.kind)
  {
  case RelationRowKind::Cladan:
    return {RelationAction::Raid, RelationAction::None};
  case RelationRowKind::Grievance:
    return {RelationAction::Tribute, RelationAction::None};
  case RelationRowKind::Expedition:
    return {RelationAction::WithdrawAttack, RelationAction::None};
  case RelationRowKind::Claimable:
    return {RelationAction::Claim, RelationAction::None};
  case RelationRowKind::Claim:
    if (_row.site != _site)
    {
      break;
    }
    // ADR-161: an open or contested claim has its battle; a waiting one is held by its wait alone.
    return _row.state == static_cast<std::uint8_t>(ClaimState::Waiting)
             ? std::array<RelationAction, 2>{RelationAction::WithdrawClaim, RelationAction::None}
             : std::array<RelationAction, 2>{RelationAction::WithdrawClaim, RelationAction::ClaimBattle};
  case RelationRowKind::Heading:
  case RelationRowKind::Warning:
  case RelationRowKind::Respite:
  case RelationRowKind::ClaimsHeading:
  case RelationRowKind::More:
    break;
  }
  return {RelationAction::None, RelationAction::None};
}

std::vector<ResultRow> ResultRowsOf(const BattleResultNotice& _result, std::uint8_t _site, double _tickSeconds, std::size_t _waiting)
{
  std::vector<ResultRow> rows;
  const bool won = (_result.attacker == _site) == _result.attackerWon;
  rows.push_back({ResultRowKind::Title, won ? 1 : 0});
  const std::uint64_t ticks = _result.endTick > _result.startTick ? _result.endTick - _result.startTick : 0;
  rows.push_back({ResultRowKind::End, static_cast<std::int64_t>(_result.end), std::llround(static_cast<double>(ticks) * _tickSeconds)});
  rows.push_back({ResultRowKind::Lost, _result.attackerLost, _result.defenderLost});
  // MVP-046: a won raid's stores go home with the attackers; PIL-045: the defender takes the fallen attackers' Essence.
  if (const std::int64_t stores = WholeRu(_result.storesMovedMilliRu); stores > 0)
  {
    rows.push_back({ResultRowKind::Stores, stores});
  }
  if (_result.essenceMoved > 0)
  {
    rows.push_back({ResultRowKind::Essence, _result.essenceMoved});
  }
  if (_result.areaMoved && _result.area.has_value())
  {
    rows.push_back({ResultRowKind::Area});
  }
  if (const std::int64_t weight = WholeWeight(_result.grievanceMicro); weight > 0)
  {
    rows.push_back({ResultRowKind::Grievance, weight});
  }
  if (_result.respiteUntil > _result.endTick)
  {
    const double minutes = static_cast<double>(_result.respiteUntil - _result.endTick) * _tickSeconds / SECONDS_PER_MINUTE;
    rows.push_back({ResultRowKind::Respite, static_cast<std::int64_t>(std::ceil(minutes))});
  }
  if (_result.renown > 0)
  {
    rows.push_back({ResultRowKind::Renown, _result.renown});
  }
  rows.push_back({ResultRowKind::Close, static_cast<std::int64_t>(_waiting)});
  return rows;
}

std::string CladanName(std::uint8_t _site, const Neuron::StringTable& _strings, const Neuron::MessageLocale& _locale)
{
  const std::array arguments{Neuron::FormatArgument{"cladan", Selector(CLADAN_SELECTORS, _site)}};
  return _strings.Format(STRING_CLADAN, arguments, _locale);
}

std::int64_t RenownTotal(const RenownLine& _line) noexcept
{
  return std::int64_t{_line.defense} + _line.claims + _line.easing + _line.structures;
}

std::vector<RenownLine> StandingOf(const ValleyStatus& _valley)
{
  std::vector<RenownLine> standing = _valley.renown;
  std::ranges::sort(standing, [](const RenownLine& _a, const RenownLine& _b)
                    { return RenownTotal(_a) != RenownTotal(_b) ? RenownTotal(_a) > RenownTotal(_b) : _a.site < _b.site; });
  return standing;
}

RunTime RunTimeOf(const ValleyStatus& _valley, std::uint64_t _tick, double _tickSeconds) noexcept
{
  if (_valley.runEnded || _tick >= _valley.runEndTick)
  {
    return {RunTime::State::Ended, 0};
  }
  // The replica's tick runs on between statuses, so the time left counts down every frame.
  const auto seconds = static_cast<std::int64_t>(std::ceil(static_cast<double>(_valley.runEndTick - _tick) * _tickSeconds));
  return {_tick >= _valley.quietTick ? RunTime::State::Quiet : RunTime::State::Running, seconds};
}

HudModel HudModelOf(const ClientSession& _session, const TuningMvp& _tuning, const TuningPresentation& _presentation,
                    std::vector<HudNotice> _notices, std::span<const SettlementNotice> _arrived, double _nowSeconds, const HudVerbs& _verbs)
{
  HudModel model;
  model.verbs = _verbs;
  model.nowSeconds = _nowSeconds;
  model.tickSeconds = _tuning.ref.sim.tickPeriod;
  model.settlement = _session.OwnSettlement();
  model.seed = _session.WorldSeed();
  model.pace = _session.Pace();
  model.idleMinutes = _tuning.mvp.presence.idleMinutes;
  const std::optional<AwayReport>& away = _session.LatestAwayReport();
  model.awayReport = model.settlement.has_value() && away.has_value() && away->team == model.settlement->team && away->absence;
  const std::optional<SettlementStatus>& status = _session.LatestStatus();
  if (model.settlement.has_value() && status.has_value() && status->team == model.settlement->team)
  {
    model.status = status;
  }
  // MVP-034: the dwelling's cost grows with every dwelling the settlement has, built or queued, as the server counts it.
  const std::int64_t dwellings = model.settlement.has_value() ? TeamDwellings(_session.Replica().Latest(), model.settlement->team) : 0;
  for (std::size_t index = 0; index < BUILD_KINDS.size(); ++index)
  {
    model.buildCostsMilliRu[index] = StructureCostMilliRu(_tuning, BUILD_KINDS[index], dwellings);
  }
  model.notices = AgeNotices(std::move(_notices), _arrived, _nowSeconds, _presentation);
  // ADR-166: the valley's status of the same settlement, and the tick its countdowns run from.
  const std::optional<ValleyStatus>& valley = _session.LatestValleyStatus();
  if (model.settlement.has_value() && valley.has_value() && valley->team == model.settlement->team)
  {
    model.valley = valley;
  }
  model.tick = _session.ServerTick().value_or(0);
  model.warningSeconds = _tuning.mvp.conflict.warningMinutes * 60.0;
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

std::string AreaName(std::uint8_t _area, const Neuron::StringTable& _strings, const Neuron::MessageLocale& _locale)
{
  if (_area >= CLAIM_AREA_CENTRAL)
  {
    const std::array arguments{Neuron::FormatArgument{"area", std::string_view("central")}};
    return _strings.Format(STRING_AREA, arguments, _locale);
  }
  const auto& pair = VALLEY_PAIRS[_area / 2U];
  const std::string owner = CladanName(pair[_area % 2U], _strings, _locale);
  const std::string facing = CladanName(pair[1U - _area % 2U], _strings, _locale);
  const std::array arguments{Neuron::FormatArgument{"area", std::string_view("half")},
                             Neuron::FormatArgument{"owner", std::string_view(owner)},
                             Neuron::FormatArgument{"facing", std::string_view(facing)}};
  return _strings.Format(STRING_AREA, arguments, _locale);
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

Neuron::UiRect SettlementHud::RelationButton(std::size_t _place, std::size_t _slot) const noexcept
{
  const auto& hud = m_presentation.mvp.hud;
  const float padding = Units(hud.padding);
  const float row = Units(hud.rowHeight);
  const float icon = Units(hud.iconSize);
  const float top = std::round(m_relationsInner.top + padding + row * static_cast<float>(_place) + (row - icon) * 0.5f);
  const float right = m_relationsInner.right - padding - static_cast<float>(_slot) * (icon + padding);
  return Neuron::UiRect::FromSize(right - icon, top, icon, icon);
}

void SettlementHud::Update(ClientSession& _session, double _realSeconds, const Neuron::UiMetrics& _metrics, const HudVerbs& _verbs,
                           std::span<const MarkerIconPlace> _markers)
{
  const std::vector<SettlementNotice> arrived = _session.TakeNotices();
  // ADR-168: the results of the settlement's battles wait for the result card; every other battle's public line is a
  // card among the notices.
  std::vector<HudNotice> notices = std::move(m_model.notices);
  for (const BattleResultNotice& result : _session.TakeBattleResults())
  {
    if (result.side)
    {
      m_results.push_back(result);
    }
    else
    {
      notices.push_back({SettlementNotice{}, _realSeconds, result});
    }
  }
  m_model = HudModelOf(_session, m_tuning, m_presentation, std::move(notices), arrived, _realSeconds, _verbs);
  m_arrived = arrived;
  m_model.markers.assign(_markers.begin(), _markers.end());
  // Each press on the card closes the result it shows. The verbs count the presses; the presses before the HUD's first
  // update closed none of its results.
  std::uint64_t closed = std::min(m_resultsClosed.value_or(_verbs.resultsClosed), _verbs.resultsClosed);
  for (; closed < _verbs.resultsClosed; ++closed)
  {
    if (!m_results.empty())
    {
      m_results.pop_front();
    }
  }
  m_resultsClosed = closed;
  if (!m_model.settlement.has_value())
  {
    m_results.clear();
  }
  m_model.results.assign(m_results.begin(), m_results.end());
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
    // ADR-166: and the valley's, at the same pace.
    static_cast<void>(_session.SendCommand(ReadValleyStatusCommand{m_model.settlement->team}));
  }
  Layout(_metrics);
}

void SettlementHud::SetModel(HudModel _model, const Neuron::UiMetrics& _metrics)
{
  m_model = std::move(_model);
  Layout(_metrics);
}

void SettlementHud::SetText(Neuron::TextSystem& _text, const Neuron::StringTable& _strings, const Neuron::MessageLocale& _locale) noexcept
{
  m_textSystem = &_text;
  m_strings = &_strings;
  m_locale = &_locale;
}

float SettlementHud::TextPixels() const noexcept
{
  return m_metrics.TextPixels(m_presentation.mvp.hud.textSize * static_cast<double>(m_metrics.HudUnitPixels()), true);
}

float SettlementHud::PlatePixels(bool _button) const noexcept
{
  const auto& chrome = m_presentation.ui.chrome;
  return static_cast<float>((_button ? chrome.buttonTexel : chrome.plateTexel) * static_cast<double>(m_metrics.HudUnitPixels()) *
                            static_cast<double>(m_metrics.uiScale));
}

std::array<float, 2> SettlementHud::PlateSize(const Neuron::NineSlice& _plate, float _width, float _height) const noexcept
{
  const float scale = PlatePixels(false);
  return {std::round(_plate.slices[0] * scale) + std::round(_plate.slices[2] * scale) + _width,
          std::round(_plate.slices[1] * scale) + std::round(_plate.slices[3] * scale) + _height};
}

Neuron::UiRect SettlementHud::Inside(const Neuron::NineSlice& _plate, const Neuron::UiRect& _rect, bool _button) const noexcept
{
  return Neuron::NineSliceInner(_plate, _rect, Neuron::NineSliceScale(_plate, _rect, PlatePixels(_button)));
}

float SettlementHud::Widest(std::span<const HudRow> _rows) const
{
  if (m_textSystem == nullptr)
  {
    return 0.0f;
  }
  const float size = TextPixels();
  const float padding = Units(m_presentation.mvp.hud.padding);
  float widest = 0.0f;
  for (const HudRow& row : _rows)
  {
    float width = row.indent + m_textSystem->Layout(row.text, Neuron::FontRole::Body, size).widthPixels + row.trailing;
    if (!row.end.empty())
    {
      width += padding + m_textSystem->Layout(row.end, Neuron::FontRole::Body, size).widthPixels;
    }
    widest = std::max(widest, width);
  }
  return std::ceil(widest);
}

std::vector<HudRow> SettlementHud::SettlementRows(const Neuron::StringTable& _strings, const Neuron::MessageLocale& _locale) const
{
  std::vector<HudRow> rows;
  if (!m_model.settlement.has_value())
  {
    return rows;
  }
  const auto& hud = m_presentation.mvp.hud;
  const std::array<float, 4> text = Rgba(hud.palette.text);
  const std::array<float, 4> positive = Rgba(hud.palette.positive);
  const std::array<float, 4> negative = Rgba(hud.palette.negative);
  const float textIndent = Units(hud.iconSize) + Units(hud.padding);
  const auto add = [&](std::optional<UiImage> _icon, std::string _text, const std::array<float, 4>& _color) -> HudRow&
  {
    HudRow& row = rows.emplace_back();
    row.text = std::move(_text);
    row.color = _color;
    row.indent = textIndent;
    row.icon = _icon;
    row.iconColor = text;
    return row;
  };
  const ReplicatedSettlement& own = *m_model.settlement;
  // MVP-030, MVP-021: the stores and their capacity, in whole resource units, from the replica's latest tick.
  {
    const std::array arguments{Neuron::FormatArgument{"stores", WholeRu(static_cast<std::int64_t>(own.storesMilliRu))},
                               Neuron::FormatArgument{"capacity", WholeRu(static_cast<std::int64_t>(own.capacityMilliRu))}};
    add(UiImage::IconResource, _strings.Format(STRING_STORES, arguments, _locale), text).meter =
      own.capacityMilliRu > 0 ? static_cast<double>(own.storesMilliRu) / static_cast<double>(own.capacityMilliRu) : 0.0;
  }
  // MVP-022: the Workers and the population limit.
  {
    const std::array arguments{Neuron::FormatArgument{"population", static_cast<std::int64_t>(own.population)},
                               Neuron::FormatArgument{"limit", static_cast<std::int64_t>(own.limit)}};
    add(UiImage::IconWorkers, _strings.Format(STRING_WORKERS, arguments, _locale), text).meter =
      own.limit > 0 ? static_cast<double>(own.population) / static_cast<double>(own.limit) : 0.0;
  }
  if (!m_model.status.has_value())
  {
    add(std::nullopt, _strings.Format(STRING_WAITING, {}, _locale), text);
    return rows;
  }
  const SettlementStatus& status = *m_model.status;
  // MVP-024, MVP-035, MVP-102: growth, or the cause when it stops; O106 (ADR-178): the hold sent until a status holds it,
  // and the room for its button.
  {
    const GrowthState state = GrowthShown(status, m_model.verbs);
    const auto growth = static_cast<std::size_t>(state);
    const auto seconds = static_cast<std::int64_t>(std::ceil(static_cast<double>(status.growthTicks) * m_model.tickSeconds));
    const std::array arguments{Neuron::FormatArgument{"state", GROWTH_SELECTORS.at(std::min(growth, GROWTH_SELECTORS.size() - 1))},
                               Neuron::FormatArgument{"seconds", seconds}};
    add(UiImage::IconGrowth, _strings.Format(STRING_GROWTH, arguments, _locale),
        state == GrowthState::Growing ? positive
        : state == GrowthState::Held  ? text
                                      : negative)
      .trailing = Units(hud.iconSize) + Units(hud.padding);
  }
  // MVP-035: the upkeep an hour, and at the row's end the structures it made inactive.
  {
    const std::array arguments{Neuron::FormatArgument{"upkeep", WholeRu(status.upkeepMilliRuPerHour)}};
    HudRow& upkeep = add(std::nullopt, _strings.Format(STRING_UPKEEP, arguments, _locale), text);
    if (status.inactiveStructures > 0)
    {
      const std::array inactive{Neuron::FormatArgument{"count", static_cast<std::int64_t>(status.inactiveStructures)}};
      upkeep.end = _strings.Format(STRING_INACTIVE, inactive, _locale);
      upkeep.endColor = negative;
    }
  }
  // MVP-036: each priority's share with the Workers that hold its task; the shares sent until a status holds them, and
  // the room for the arrows that move them (ADR-153).
  const std::array<std::uint8_t, 3> shares = m_model.verbs.shares.value_or(status.priorities);
  const float arrowsWidth = 2.0f * (Units(hud.iconSize) + Units(hud.padding));
  for (std::size_t task = 0; task < TASK_SELECTORS.size(); ++task)
  {
    const std::array arguments{Neuron::FormatArgument{"task", TASK_SELECTORS[task]},
                               Neuron::FormatArgument{"share", static_cast<std::int64_t>(shares[task])},
                               Neuron::FormatArgument{"workers", static_cast<std::int64_t>(status.tasks[task])}};
    HudRow& priority = add(PRIORITY_ICONS[task], _strings.Format(STRING_PRIORITY, arguments, _locale), text);
    priority.meter = static_cast<double>(shares[task]) / SHARE_MAX;
    priority.trailing = arrowsWidth;
    priority.task = task;
  }
  // MVP-037: the idle Workers, by reason.
  std::int64_t idle = 0;
  for (const std::uint32_t count : status.idle)
  {
    idle += count;
  }
  const std::array idleArguments{Neuron::FormatArgument{"count", idle}};
  add(std::nullopt, _strings.Format(STRING_IDLE, idleArguments, _locale), text).indent = 0.0f;
  for (std::size_t reason = 0; reason < IDLE_SELECTORS.size(); ++reason)
  {
    if (status.idle[reason] == 0)
    {
      continue;
    }
    const std::array arguments{Neuron::FormatArgument{"reason", IDLE_SELECTORS[reason]},
                               Neuron::FormatArgument{"count", static_cast<std::int64_t>(status.idle[reason])}};
    add(std::nullopt, _strings.Format(STRING_IDLE_REASON, arguments, _locale), negative);
  }
  return rows;
}

std::vector<HudRow> SettlementHud::WorldRows(const Neuron::StringTable& _strings, const Neuron::MessageLocale& _locale) const
{
  std::vector<HudRow> rows;
  if (!m_model.settlement.has_value())
  {
    return rows;
  }
  const auto& hud = m_presentation.mvp.hud;
  const std::array<float, 4> text = Rgba(hud.palette.text);
  // MVP-005: the world's seed, as digits that can be typed again; its row stays when there is none.
  HudRow& seedRow = rows.emplace_back();
  seedRow.color = text;
  if (m_model.seed.has_value())
  {
    const std::string seed = std::to_string(*m_model.seed);
    const std::array arguments{Neuron::FormatArgument{"seed", std::string_view(seed)}};
    seedRow.text = _strings.Format(STRING_SEED, arguments, _locale);
  }
  // MVP-071 and MVP-072 (ADR-191): an always-on world, and when its steward counts as away.
  if (m_model.pace == WorldPace::AlwaysOn)
  {
    const std::array arguments{Neuron::FormatArgument{"minutes", static_cast<std::int64_t>(std::llround(m_model.idleMinutes))}};
    HudRow& pace = rows.emplace_back();
    pace.text = _strings.Format(STRING_ALWAYS_ON, arguments, _locale);
    pace.color = text;
    pace.trailing = Units(hud.iconSize) + Units(hud.padding); // O110 (ADR-192): the away report's button
  }
  if (!m_model.valley.has_value())
  {
    return rows;
  }
  // MVP-060 and MVP-062 (ADR-169): the time left in the run, and the standing with each settlement's renown by source.
  const RunTime run = RunTimeOf(*m_model.valley, m_model.tick, m_model.tickSeconds);
  const std::string minutes = std::format("{:02}", run.secondsLeft / 60 % 60);
  const std::string seconds = std::format("{:02}", run.secondsLeft % 60);
  const std::array timeArguments{Neuron::FormatArgument{"state", std::string_view(run.state == RunTime::State::Ended   ? "ended"
                                                                                  : run.state == RunTime::State::Quiet ? "quiet"
                                                                                                                       : "running")},
                                 Neuron::FormatArgument{"hours", run.secondsLeft / SECONDS_PER_HOUR},
                                 Neuron::FormatArgument{"minutes", std::string_view(minutes)},
                                 Neuron::FormatArgument{"seconds", std::string_view(seconds)}};
  HudRow& time = rows.emplace_back();
  time.text = _strings.Format(STRING_TIME_LEFT, timeArguments, _locale);
  time.color = run.state == RunTime::State::Running ? text : Rgba(hud.palette.negative);
  HudRow& standing = rows.emplace_back();
  standing.text = _strings.Format(STRING_STANDING, {}, _locale);
  standing.color = text;
  std::int64_t place = 0;
  for (const RenownLine& renown : StandingOf(*m_model.valley))
  {
    ++place;
    const std::string name = CladanName(renown.site, _strings, _locale);
    const std::array arguments{Neuron::FormatArgument{"place", place},
                               Neuron::FormatArgument{"cladan", std::string_view(name)},
                               Neuron::FormatArgument{"total", RenownTotal(renown)},
                               Neuron::FormatArgument{"defense", std::int64_t{renown.defense}},
                               Neuron::FormatArgument{"claims", std::int64_t{renown.claims}},
                               Neuron::FormatArgument{"easing", std::int64_t{renown.easing}},
                               Neuron::FormatArgument{"structures", std::int64_t{renown.structures}}};
    HudRow& line = rows.emplace_back();
    line.text = _strings.Format(STRING_RENOWN, arguments, _locale);
    line.color = renown.site == m_model.settlement->site ? Rgba(hud.palette.positive) : text;
    line.icon = UiImage::HudMeter; // MVP-013: a swatch of the Cladan's colour
    line.iconColor = TeamColor(renown.site);
    line.indent = Units(hud.iconSize) + Units(hud.padding);
  }
  return rows;
}

std::vector<HudRow> SettlementHud::RelationPanelRows(const Neuron::StringTable& _strings, const Neuron::MessageLocale& _locale) const
{
  std::vector<HudRow> rows;
  if (!m_model.settlement.has_value())
  {
    return rows;
  }
  const auto& hud = m_presentation.mvp.hud;
  const std::array<float, 4> text = Rgba(hud.palette.text);
  const std::array<float, 4> positive = Rgba(hud.palette.positive);
  const std::array<float, 4> negative = Rgba(hud.palette.negative);
  const float icon = Units(hud.iconSize);
  const float padding = Units(hud.padding);
  // Each row keeps the room of its two buttons at its end (ADR-167).
  const float trailing = 2.0f * (icon + padding);
  for (const RelationRow& relation : m_relationRows)
  {
    HudRow& row = rows.emplace_back();
    row.color = text;
    row.iconColor = text;
    row.indent = icon + padding;
    row.trailing = trailing;
    switch (relation.kind)
    {
    case RelationRowKind::Heading:
      row.text = _strings.Format(STRING_RELATIONS, {}, _locale);
      row.indent = 0.0f;
      break;
    case RelationRowKind::Warning:
    {
      const std::string name = CladanName(relation.site, _strings, _locale);
      const std::string seconds = std::format("{:02}", relation.value % 60);
      const std::array arguments{Neuron::FormatArgument{"cladan", std::string_view(name)},
                                 Neuron::FormatArgument{"minutes", relation.value / 60},
                                 Neuron::FormatArgument{"seconds", std::string_view(seconds)}};
      row.text = _strings.Format(STRING_WARNING, arguments, _locale);
      row.color = negative;
      row.icon = UiImage::IconAttack;
      row.iconColor = negative;
      break;
    }
    case RelationRowKind::Expedition:
    {
      const std::string name = CladanName(relation.site, _strings, _locale);
      const std::array arguments{Neuron::FormatArgument{"state", Selector(EXPEDITION_SELECTORS, relation.detail)},
                                 Neuron::FormatArgument{"target", std::string_view(name)}};
      row.text = _strings.Format(STRING_EXPEDITION, arguments, _locale);
      row.icon = UiImage::IconAttack;
      break;
    }
    case RelationRowKind::Respite:
    {
      const std::array arguments{Neuron::FormatArgument{"minutes", relation.value}};
      row.text = _strings.Format(STRING_RESPITE, arguments, _locale);
      row.color = positive;
      row.icon = UiImage::IconRespite;
      row.iconColor = positive;
      break;
    }
    case RelationRowKind::Cladan:
    {
      // MVP-013: a swatch of the Cladan's colour; its respite is drawn at the row's end.
      const std::string name = CladanName(relation.site, _strings, _locale);
      const std::array arguments{Neuron::FormatArgument{"cladan", std::string_view(name)}, Neuron::FormatArgument{"anger", relation.value}};
      row.text = _strings.Format(STRING_ANGER, arguments, _locale);
      row.color = relation.value > 0 ? negative : text;
      row.icon = UiImage::HudMeter;
      row.iconColor = TeamColor(relation.site);
      break;
    }
    case RelationRowKind::Grievance:
    {
      // Indented under its Cladan, with its cause's icon; a removed cause fades faster (MVP-052).
      const std::array arguments{Neuron::FormatArgument{"cause", Selector(CAUSE_SELECTORS, relation.detail)},
                                 Neuron::FormatArgument{"weight", relation.value},
                                 Neuron::FormatArgument{"removed", std::string_view(relation.state != 0 ? "yes" : "no")}};
      row.text = _strings.Format(STRING_GRIEVANCE, arguments, _locale);
      row.icon = CAUSE_ICONS.at(std::min<std::size_t>(relation.detail, CAUSE_ICONS.size() - 1));
      row.iconIndent = icon + padding;
      row.indent = 2.0f * (icon + padding);
      break;
    }
    case RelationRowKind::ClaimsHeading:
      row.text = _strings.Format(STRING_CLAIMS, {}, _locale);
      row.indent = 0.0f;
      break;
    case RelationRowKind::Claim:
    {
      const std::string name = CladanName(relation.site, _strings, _locale);
      const std::string area = AreaName(relation.detail, _strings, _locale);
      const std::array arguments{Neuron::FormatArgument{"claimant", std::string_view(name)},
                                 Neuron::FormatArgument{"area", std::string_view(area)},
                                 Neuron::FormatArgument{"state", Selector(CLAIM_SELECTORS, relation.state)}};
      row.text = _strings.Format(STRING_CLAIM, arguments, _locale);
      row.icon = UiImage::IconClaim;
      row.iconColor = TeamColor(relation.site);
      break;
    }
    case RelationRowKind::Claimable:
    {
      // ADR-167: an area the settlement may claim now, its button at the row's end.
      const std::string area = AreaName(relation.detail, _strings, _locale);
      const std::array arguments{Neuron::FormatArgument{"area", std::string_view(area)}};
      row.text = _strings.Format(STRING_CLAIMABLE, arguments, _locale);
      row.icon = UiImage::IconClaim;
      row.iconColor = Rgba(hud.palette.text, hud.disabledAlpha);
      break;
    }
    case RelationRowKind::More:
    {
      const std::array arguments{Neuron::FormatArgument{"count", relation.value}};
      row.text = _strings.Format(STRING_MORE, arguments, _locale);
      break;
    }
    }
  }
  return rows;
}

std::vector<HudRow> SettlementHud::ResultPanelRows(const Neuron::StringTable& _strings, const Neuron::MessageLocale& _locale) const
{
  std::vector<HudRow> rows;
  if (m_resultRows.empty() || m_model.results.empty())
  {
    return rows;
  }
  const auto& hud = m_presentation.mvp.hud;
  const std::array<float, 4> text = Rgba(hud.palette.text);
  const BattleResultNotice& result = m_model.results.front();
  const std::string attacker = CladanName(result.attacker, _strings, _locale);
  const std::string defender = CladanName(result.defender, _strings, _locale);
  const std::string winner = result.attackerWon ? attacker : defender;
  const std::string area = result.area.has_value() ? AreaName(*result.area, _strings, _locale) : std::string();
  const auto name = [](const std::string& _name) { return std::string_view(_name); };
  for (const ResultRow& resultRow : m_resultRows)
  {
    HudRow& row = rows.emplace_back();
    row.color = text;
    switch (resultRow.kind)
    {
    case ResultRowKind::Title:
    {
      const std::array arguments{Neuron::FormatArgument{"outcome", std::string_view(resultRow.value != 0 ? "won" : "lost")},
                                 Neuron::FormatArgument{"kind", std::string_view(result.area.has_value() ? "claim" : "raid")},
                                 Neuron::FormatArgument{"attacker", name(attacker)}, Neuron::FormatArgument{"defender", name(defender)},
                                 Neuron::FormatArgument{"area", name(area)}};
      row.text = _strings.Format(STRING_RESULT_TITLE, arguments, _locale);
      row.color = resultRow.value != 0 ? Rgba(hud.palette.positive) : Rgba(hud.palette.negative);
      break;
    }
    case ResultRowKind::End:
    {
      const std::array arguments{Neuron::FormatArgument{"end", Selector(END_SELECTORS, static_cast<std::uint8_t>(resultRow.value))},
                                 Neuron::FormatArgument{"minutes", resultRow.second / 60},
                                 Neuron::FormatArgument{"seconds", resultRow.second % 60}};
      row.text = _strings.Format(STRING_RESULT_END, arguments, _locale);
      break;
    }
    case ResultRowKind::Lost:
    {
      const std::array arguments{Neuron::FormatArgument{"attacker", name(attacker)}, Neuron::FormatArgument{"defender", name(defender)},
                                 Neuron::FormatArgument{"attacker_lost", resultRow.value},
                                 Neuron::FormatArgument{"defender_lost", resultRow.second}};
      row.text = _strings.Format(STRING_RESULT_LOST, arguments, _locale);
      break;
    }
    case ResultRowKind::Stores:
    {
      const std::array arguments{Neuron::FormatArgument{"attacker", name(attacker)}, Neuron::FormatArgument{"stores", resultRow.value}};
      row.text = _strings.Format(STRING_RESULT_STORES, arguments, _locale);
      break;
    }
    case ResultRowKind::Essence:
    {
      const std::array arguments{Neuron::FormatArgument{"defender", name(defender)}, Neuron::FormatArgument{"essence", resultRow.value}};
      row.text = _strings.Format(STRING_RESULT_ESSENCE, arguments, _locale);
      break;
    }
    case ResultRowKind::Area:
    {
      const std::array arguments{Neuron::FormatArgument{"area", name(area)}, Neuron::FormatArgument{"winner", name(winner)}};
      row.text = _strings.Format(STRING_RESULT_AREA, arguments, _locale);
      break;
    }
    case ResultRowKind::Grievance:
    {
      const std::array arguments{Neuron::FormatArgument{"defender", name(defender)}, Neuron::FormatArgument{"attacker", name(attacker)},
                                 Neuron::FormatArgument{"weight", resultRow.value}};
      row.text = _strings.Format(STRING_RESULT_GRIEVANCE, arguments, _locale);
      break;
    }
    case ResultRowKind::Respite:
    {
      const std::array arguments{Neuron::FormatArgument{"defender", name(defender)}, Neuron::FormatArgument{"minutes", resultRow.value}};
      row.text = _strings.Format(STRING_RESULT_RESPITE, arguments, _locale);
      break;
    }
    case ResultRowKind::Renown:
    {
      const std::array arguments{Neuron::FormatArgument{"winner", name(winner)}, Neuron::FormatArgument{"renown", resultRow.value}};
      row.text = _strings.Format(STRING_RESULT_RENOWN, arguments, _locale);
      break;
    }
    case ResultRowKind::Close:
    {
      const std::array arguments{Neuron::FormatArgument{"more", resultRow.value}};
      row.text = _strings.Format(STRING_RESULT_CLOSE, arguments, _locale);
      row.color = Rgba(hud.palette.text, hud.disabledAlpha);
      break;
    }
    }
  }
  return rows;
}

HudRow SettlementHud::NoticeRow(const HudNotice& _notice, double _alpha, const Neuron::StringTable& _strings,
                                const Neuron::MessageLocale& _locale) const
{
  const auto& hud = m_presentation.mvp.hud;
  HudRow row;
  row.indent = Units(hud.iconSize) + Units(hud.padding);
  row.iconColor = Rgba(hud.palette.text, _alpha);
  // ADR-168: another battle's public line, its winner and loser named.
  if (_notice.result.has_value())
  {
    const BattleResultNotice& result = *_notice.result;
    const std::string winner = CladanName(result.attackerWon ? result.attacker : result.defender, _strings, _locale);
    const std::string loser = CladanName(result.attackerWon ? result.defender : result.attacker, _strings, _locale);
    const std::string area = result.area.has_value() ? AreaName(*result.area, _strings, _locale) : std::string();
    const std::array arguments{Neuron::FormatArgument{"winner", std::string_view(winner)},
                               Neuron::FormatArgument{"loser", std::string_view(loser)},
                               Neuron::FormatArgument{"kind", std::string_view(result.area.has_value() ? "claim" : "raid")},
                               Neuron::FormatArgument{"area", std::string_view(area)}};
    row.text = _strings.Format(STRING_RESULT_PUBLIC, arguments, _locale);
    row.color = Rgba(hud.palette.text, _alpha);
    row.icon = UiImage::IconBattle;
    return row;
  }
  const NoticeLook look = NoticeLookOf(_notice.notice);
  row.icon = look.icon;
  row.color = look.positive ? Rgba(hud.palette.positive, _alpha) : Rgba(hud.palette.negative, _alpha);
  if (_notice.notice.kind == NoticeKind::StructureCompleted)
  {
    const std::array arguments{Neuron::FormatArgument{"kind", KindSelector(_notice.notice.structure)}};
    row.text = _strings.Format(STRING_COMPLETED, arguments, _locale);
    return row;
  }
  // ADR-166: M3's kinds, naming the other settlement and, for a claim, its area (the notice's detail).
  const std::string other = _notice.notice.other.has_value() ? CladanName(*_notice.notice.other, _strings, _locale) : std::string();
  const bool claim = _notice.notice.kind == NoticeKind::ClaimMade || _notice.notice.kind == NoticeKind::ClaimWithdrawn ||
                     _notice.notice.kind == NoticeKind::ClaimLost || _notice.notice.kind == NoticeKind::ClaimWon;
  const std::string area = claim ? AreaName(_notice.notice.detail, _strings, _locale) : std::string();
  const std::array arguments{Neuron::FormatArgument{"kind", look.kind}, Neuron::FormatArgument{"other", std::string_view(other)},
                             Neuron::FormatArgument{"area", std::string_view(area)}};
  row.text = _strings.Format(STRING_NOTICE, arguments, _locale);
  return row;
}

std::array<HudRow, 2> SettlementHud::BuildRows(std::size_t _index, const Neuron::StringTable& _strings,
                                               const Neuron::MessageLocale& _locale) const
{
  const auto& hud = m_presentation.mvp.hud;
  std::array<HudRow, 2> rows;
  const std::array name{Neuron::FormatArgument{"kind", KindSelector(BUILD_KINDS[_index])}};
  rows[0].text = _strings.Format(STRING_BUILD, name, _locale);
  rows[0].color = Rgba(hud.palette.text);
  // MVP-034: red while the stores cannot pay it.
  const std::int64_t cost = m_model.buildCostsMilliRu[_index];
  const std::array costArguments{Neuron::FormatArgument{"cost", WholeRu(cost)}};
  rows[1].text = _strings.Format(STRING_COST, costArguments, _locale);
  const bool affordable = m_model.settlement.has_value() && static_cast<std::int64_t>(m_model.settlement->storesMilliRu) >= cost;
  rows[1].color = affordable ? Rgba(hud.palette.positive) : Rgba(hud.palette.negative);
  return rows;
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
  const float icon = Units(hud.iconSize);
  const float least = Units(hud.panelWidth);
  // ADR-175: with a text system given, each panel is as wide as its widest row needs, and its plate's frame lies around
  // its content; else it keeps its least width.
  const bool measured = m_textSystem != nullptr && m_strings != nullptr && m_locale != nullptr;
  const auto plateOf = [&](const Neuron::NineSlice& _plate, float _least, float _content, std::size_t _rows)
  {
    const std::array<float, 2> size = PlateSize(_plate, _content + 2.0f * padding, 2.0f * padding + row * static_cast<float>(_rows));
    return std::array<float, 2>{std::max(_least, size[0]), size[1]};
  };

  // The settlement panel, top left: the fixed rows, then a heading and a row for each idle reason with Workers.
  std::size_t idleRows = 1;
  if (m_model.status.has_value())
  {
    idleRows += static_cast<std::size_t>(std::ranges::count_if(m_model.status->idle, [](std::uint32_t _count) { return _count > 0; }));
  }
  m_panelRows = m_model.status.has_value() ? FIXED_ROWS + idleRows : 3;
  const std::array<float, 2> panel =
    plateOf(m_plates.accent, least, measured ? Widest(SettlementRows(*m_strings, *m_locale)) : 0.0f, m_panelRows);
  m_panel = Neuron::UiRect::FromSize(margin, margin, panel[0], panel[1]);
  m_panelInner = Inside(m_plates.accent, m_panel, false);
  m_zones.push_back({HudZone::Settlement, 0, m_panel});
  // ADR-153: each priority's lowering and raising arrows at its row's right end, once the status gives the shares.
  if (m_model.status.has_value())
  {
    for (std::size_t task = 0; task < m_arrows.size(); ++task)
    {
      const float top = std::round(m_panelInner.top + padding + row * static_cast<float>(PRIORITY_ROW + task) + (row - icon) * 0.5f);
      const float raiseLeft = m_panelInner.right - padding - icon;
      m_arrows[task][0] = Neuron::UiRect::FromSize(raiseLeft - padding - icon, top, icon, icon);
      m_arrows[task][1] = Neuron::UiRect::FromSize(raiseLeft, top, icon, icon);
      m_zones.push_back({HudZone::PriorityLower, static_cast<std::uint8_t>(task), m_arrows[task][0]});
      m_zones.push_back({HudZone::PriorityRaise, static_cast<std::uint8_t>(task), m_arrows[task][1]});
    }
    // O106 (ADR-178): the growth row's hold button at its right end.
    const float top = std::round(m_panelInner.top + padding + row * static_cast<float>(GROWTH_ROW) + (row - icon) * 0.5f);
    m_holdButton = Neuron::UiRect::FromSize(m_panelInner.right - padding - icon, top, icon, icon);
    m_zones.push_back({HudZone::GrowthHold, 0, m_holdButton});
  }

  // The construction bar, bottom centre, and the queue's row above it; ADR-175: each entry as wide as the widest name or
  // cost needs inside the wider frame of its two plates.
  m_buttonWidth = Units(hud.barButtonWidth);
  if (measured)
  {
    std::vector<HudRow> entries;
    for (std::size_t index = 0; index < BUILD_KINDS.size(); ++index)
    {
      const std::array<HudRow, 2> rows = BuildRows(index, *m_strings, *m_locale);
      entries.insert(entries.end(), rows.begin(), rows.end());
    }
    const float scale = PlatePixels(true);
    const auto frame = [&](const Neuron::NineSlice& _plate)
    { return std::round(_plate.slices[0] * scale) + std::round(_plate.slices[2] * scale); };
    m_buttonWidth =
      std::max(m_buttonWidth, std::max(frame(m_plates.accent), frame(m_plates.neutral)) + 3.0f * padding + icon + Widest(entries));
  }
  const float barHeight = Units(hud.barHeight);
  const float gap = Units(CARD_GAP);
  const float barWidth = m_buttonWidth * static_cast<float>(BUILD_KINDS.size()) + gap * static_cast<float>(BUILD_KINDS.size() - 1);
  const float barLeft = std::round((width - barWidth) * 0.5f);
  const float barTop = height - margin - barHeight;
  for (std::size_t index = 0; index < BUILD_KINDS.size(); ++index)
  {
    m_build[index] =
      Neuron::UiRect::FromSize(barLeft + static_cast<float>(index) * (m_buttonWidth + gap), barTop, m_buttonWidth, barHeight);
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

  // The notification cards, top right, newest first; ADR-175: as wide as the widest card's line needs.
  m_cardWidth = Units(hud.cardWidth);
  if (!m_model.notices.empty())
  {
    if (measured)
    {
      std::vector<HudRow> cards;
      cards.reserve(m_model.notices.size());
      for (const HudNotice& notice : m_model.notices)
      {
        cards.push_back(NoticeRow(notice, 1.0, *m_strings, *m_locale));
      }
      m_cardWidth = std::max(m_cardWidth, 2.0f * padding + Widest(cards));
    }
    const float cardHeight = Units(hud.cardHeight);
    const auto cards = static_cast<float>(m_model.notices.size());
    m_zones.push_back(
      {HudZone::Notices, 0,
       Neuron::UiRect::FromSize(width - margin - m_cardWidth, margin, m_cardWidth, cards * cardHeight + (cards - 1.0f) * gap)});
  }

  // The world's panel, bottom left.
  // ADR-169: under the seed, once the valley's status has come, the time left, the standing's heading and its rows.
  const std::size_t worldRows =
    1 + (m_model.pace == WorldPace::AlwaysOn ? 1 : 0) + (m_model.valley.has_value() ? 2 + m_model.valley->renown.size() : 0);
  const std::array<float, 2> world = plateOf(m_plates.accent, least, measured ? Widest(WorldRows(*m_strings, *m_locale)) : 0.0f, worldRows);
  // ADR-175: a bottom panel that would reach under the construction bar stands above it instead.
  const auto bottomOf = [&](float _left, float _right)
  { return _right + gap > barLeft && _left - gap < barLeft + barWidth ? barTop - gap : height - margin; };
  m_world = Neuron::UiRect::FromSize(margin, bottomOf(margin, margin + world[0]) - world[1], world[0], world[1]);
  m_worldInner = Inside(m_plates.accent, m_world, false);
  m_zones.push_back({HudZone::World, 0, m_world});
  // O110 (ADR-192): the always-on row's button at its right end, which opens the latest away report.
  if (m_model.pace == WorldPace::AlwaysOn)
  {
    const float top = std::round(m_worldInner.top + padding + row * static_cast<float>(ALWAYS_ON_ROW) + (row - icon) * 0.5f);
    m_awayButton = Neuron::UiRect::FromSize(m_worldInner.right - padding - icon, top, icon, icon);
    m_zones.push_back({HudZone::AwayReport, 0, m_awayButton});
  }

  // ADR-166: the relations panel, bottom right, once the valley's status has come.
  m_relationRows.clear();
  if (m_model.valley.has_value())
  {
    m_relationRows = RelationRowsOf(*m_model.valley, m_model.settlement->site, m_model.tick, m_model.tickSeconds, m_model.warningSeconds,
                                    static_cast<std::size_t>(std::max<std::int64_t>(hud.relationsRowsMax, 1)));
    const std::array<float, 2> relations =
      plateOf(m_plates.accent, least, measured ? Widest(RelationPanelRows(*m_strings, *m_locale)) : 0.0f, m_relationRows.size());
    m_relations = Neuron::UiRect::FromSize(
      width - margin - relations[0], bottomOf(width - margin - relations[0], width - margin) - relations[1], relations[0], relations[1]);
    m_relationsInner = Inside(m_plates.accent, m_relations, false);
    m_zones.push_back({HudZone::Relations, 0, m_relations});
    // ADR-167: each row's buttons at its end, the first nearest it, found before the panel under them.
    for (std::size_t place = 0; place < m_relationRows.size() && place < RELATION_ROWS_ADDRESSED; ++place)
    {
      const std::array<RelationAction, 2> actions = RelationActionsOf(m_relationRows[place], m_model.settlement->site);
      for (std::size_t slot = 0; slot < actions.size(); ++slot)
      {
        if (actions[slot] != RelationAction::None)
        {
          m_zones.push_back({HudZone::RelationAction, static_cast<std::uint8_t>(place * 2 + slot), RelationButton(place, slot)});
        }
      }
    }
  }

  // ADR-168: the result card, top centre, while a result of the settlement's battles waits, a row high per row.
  m_resultRows.clear();
  if (!m_model.results.empty())
  {
    m_resultRows = ResultRowsOf(m_model.results.front(), m_model.settlement->site, m_model.tickSeconds, m_model.results.size() - 1);
    const std::array<float, 2> result = plateOf(m_plates.neutral, Units(hud.resultWidth),
                                                measured ? Widest(ResultPanelRows(*m_strings, *m_locale)) : 0.0f, m_resultRows.size());
    m_result = Neuron::UiRect::FromSize(std::round((width - result[0]) * 0.5f), margin, result[0], result[1]);
    m_resultInner = Inside(m_plates.neutral, m_result, false);
    m_zones.push_back({HudZone::Result, 0, m_result});
  }
}

std::optional<HudZoneRect> SettlementHud::ZoneAt(Neuron::PointPx _position) const noexcept
{
  for (auto zone = m_zones.rbegin(); zone != m_zones.rend(); ++zone)
  {
    if (zone->rect.Contains(static_cast<float>(_position.x), static_cast<float>(_position.y)))
    {
      return *zone;
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
  const float textPixels = TextPixels();
  const std::array<float, 4> text = Rgba(palette.text);
  const std::array<float, 4> positive = Rgba(palette.positive);
  const std::array<float, 4> negative = Rgba(palette.negative);

  const auto image = [&](UiImage _image, const Neuron::UiRect& _rect, const std::array<float, 4>& _color,
                         const std::array<float, 4>& _uv = {0.0f, 0.0f, 1.0f, 1.0f})
  {
    Neuron::AppendQuad(_out, Neuron::UiSource::Image, Neuron::UiBlend::Alpha, static_cast<std::uint16_t>(_image), Quad(_rect, _color, _uv));
  };
  // ADR-175: a plate of the accent or the neutral panel art, at a panel's or a button's scale.
  const auto plate = [&](bool _accent, const Neuron::UiRect& _rect, bool _button)
  {
    const Neuron::NineSlice& slice = _accent ? m_plates.accent : m_plates.neutral;
    Neuron::AppendNineSlice(_out, static_cast<std::uint16_t>(_accent ? UiImage::PanelAccent : UiImage::PanelNeutral), slice, _rect,
                            Neuron::NineSliceScale(slice, _rect, PlatePixels(_button)),
                            _accent ? Rgba(palette.accent) : Rgba(palette.neutral));
  };
  const auto line = [&](std::string_view _line, float _left, float _top, float _height, const std::array<float, 4>& _color,
                        bool _rightAligned = false, float _right = 0.0f)
  {
    const Neuron::TextLine laid = _text.Layout(_line, Neuron::FontRole::Body, textPixels);
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
  // ADR-175: a panel's row, its content running from _left to _right, at _top: its icon, its text, the meter under its
  // text and the text at its end.
  const auto drawRow = [&](const HudRow& _row, float _left, float _right, float _top)
  {
    if (_row.icon.has_value())
    {
      image(*_row.icon, Neuron::UiRect::FromSize(_left + _row.iconIndent, std::round(_top + (row - icon) * 0.5f), icon, icon),
            _row.iconColor);
    }
    const float textHeight = _row.meter.has_value() ? row - meterHeight : row;
    line(_row.text, _left + _row.indent, _top, textHeight, _row.color);
    if (_row.meter.has_value())
    {
      meter(_left + _row.indent, _top + row - meterHeight - 1.0f, _right - _row.trailing - _left - _row.indent, *_row.meter);
    }
    if (!_row.end.empty())
    {
      line(_row.end, 0.0f, _top, textHeight, _row.endColor, true, _right - _row.trailing);
    }
  };

  // ADR-167: the attacks' and claims' icons over their world markers' discs (pass 16), under the panels, in white as the
  // Leader marker's icon is (R-CTRL-386).
  for (const MarkerIconPlace& marker : m_model.markers)
  {
    const float side = std::round(marker.side);
    image(marker.icon == TargetIcon::Claim ? UiImage::IconClaim : UiImage::IconAttack,
          Neuron::UiRect::FromSize(std::round(marker.x - side * 0.5f), std::round(marker.y - side * 0.5f), side, side), text);
  }

  // The settlement panel (UI-460's accent plate), and each priority's arrows (ADR-153), cursor.pointer's art pointing
  // down and flipped for up (UI-076).
  plate(true, m_panel, false);
  {
    const float left = m_panelInner.left + padding;
    const float right = m_panelInner.right - padding;
    float top = m_panelInner.top + padding;
    for (const HudRow& panelRow : SettlementRows(_strings, _locale))
    {
      drawRow(panelRow, left, right, top);
      // O106 (ADR-178): the hold button, on the accent plate while the growth is held.
      if (panelRow.icon == UiImage::IconGrowth && m_model.status.has_value())
      {
        const bool held = GrowthShown(*m_model.status, m_model.verbs) == GrowthState::Held;
        plate(held, m_holdButton, true);
        image(UiImage::IconRespite, m_holdButton, Rgba(palette.text));
      }
      if (panelRow.task.has_value() && *panelRow.task < m_arrows.size() && m_model.status.has_value())
      {
        const std::size_t task = *panelRow.task;
        const std::array<std::uint8_t, 3> shares = m_model.verbs.shares.value_or(m_model.status->priorities);
        const std::array<bool, 2> movable{shares[task] > 0, shares[task] < SHARE_MAX};
        for (std::size_t arrow = 0; arrow < movable.size(); ++arrow)
        {
          const Neuron::UiRect& rect = m_arrows[task][arrow];
          plate(false, rect, true);
          const std::array<float, 4> uv =
            arrow == 1 ? std::array<float, 4>{0.0f, 1.0f, 1.0f, 0.0f} : std::array<float, 4>{0.0f, 0.0f, 1.0f, 1.0f};
          image(UiImage::ArrowPointer, rect, Rgba(palette.text, movable[arrow] ? 1.0 : hud.disabledAlpha), uv);
        }
      }
      top += row;
    }
  }

  // MVP-032, MVP-034: the construction bar, each structure with its cost, red while the stores cannot pay it; ADR-153:
  // the entry being placed on the accent plate.
  for (std::size_t index = 0; index < BUILD_KINDS.size(); ++index)
  {
    const Neuron::UiRect& button = m_build[index];
    const bool placing = m_model.verbs.placing == BUILD_KINDS[index];
    plate(placing, button, true);
    const Neuron::UiRect inner = Inside(placing ? m_plates.accent : m_plates.neutral, button, true);
    image(KindIcon(BUILD_KINDS[index]),
          Neuron::UiRect::FromSize(inner.left + padding, std::round(inner.top + (inner.Height() - icon) * 0.5f), icon, icon), text);
    const float left = inner.left + 2.0f * padding + icon;
    const float half = std::round(inner.Height() * 0.5f);
    const std::array<HudRow, 2> entry = BuildRows(index, _strings, _locale);
    line(entry[0].text, left, inner.top, half, entry[0].color);
    line(entry[1].text, left, inner.top + half, inner.Height() - half, entry[1].color);
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
      plate(false, cell, true);
      const float inset = std::round((site - icon) * 0.5f);
      image(KindIcon(queued.kind), Neuron::UiRect::FromSize(cell.left + inset, cell.top + inset - meterHeight, icon, icon), text);
      meter(cell.left + 2.0f, cell.bottom - meterHeight - 2.0f, site - 4.0f, static_cast<double>(queued.progressPermille) / 1000.0);
      left += site + gap;
    }
  }

  // ADR-153: above the queue, the server's last refusal of a placement with its reason, fading as a card does; else,
  // while placing, the refusal the client foresees at the pointer, or the structure being placed.
  {
    const ReplicatedSettlement& own = *m_model.settlement;
    const auto centered = [&](std::string_view _id, std::span<const Neuron::FormatArgument> _arguments, const std::array<float, 4>& _color)
    {
      const std::string formatted = _strings.Format(_id, _arguments, _locale);
      const Neuron::TextLine laid = _text.Layout(formatted, Neuron::FontRole::Body, textPixels);
      Neuron::TextStyle style;
      style.color = _color;
      const float lineTop = m_queue.top - Units(CARD_GAP) - row;
      Neuron::AppendText(_out, laid, std::round((static_cast<float>(m_metrics.client.width) - laid.widthPixels) * 0.5f),
                         Neuron::RowTextTop(lineTop, row, laid.heightPixels), style);
    };
    const HudVerbs& verbs = m_model.verbs;
    const auto faded = [&](double _ageSeconds)
    {
      const double left = hud.noticeSeconds - _ageSeconds;
      return hud.noticeFade > 0.0 ? std::clamp(left / hud.noticeFade, 0.0, 1.0) : 1.0;
    };
    if (verbs.pending.has_value())
    {
      // MVP-101 (ADR-167): the attack being declared, with its cost and whether it is unprovoked, before it is confirmed.
      const PendingAttack& pending = *verbs.pending;
      const std::string target = CladanName(pending.target, _strings, _locale);
      const std::string area = pending.area.has_value() ? AreaName(*pending.area, _strings, _locale) : std::string();
      const std::array arguments{Neuron::FormatArgument{"kind", std::string_view(pending.area.has_value() ? "claim" : "raid")},
                                 Neuron::FormatArgument{"target", std::string_view(target)},
                                 Neuron::FormatArgument{"area", std::string_view(area)},
                                 Neuron::FormatArgument{"leaders", static_cast<std::int64_t>(pending.leaders)},
                                 Neuron::FormatArgument{"cost", WholeRu(pending.costMilliRu)},
                                 Neuron::FormatArgument{"unprovoked", std::string_view(pending.unprovoked ? "yes" : "no")}};
      centered(STRING_DECLARE, arguments, static_cast<std::int64_t>(own.storesMilliRu) >= pending.costMilliRu ? text : negative);
    }
    else if (verbs.conflictRefusal.has_value() && verbs.conflictRefusalAgeSeconds < hud.noticeSeconds)
    {
      const auto [kind, reason] = *verbs.conflictRefusal;
      const std::array arguments{Neuron::FormatArgument{"kind", Selector(CONFLICT_SELECTORS, static_cast<std::size_t>(kind))},
                                 Neuron::FormatArgument{"reason", ConflictReasonSelector(kind, reason)}};
      centered(STRING_CONFLICT_REFUSED, arguments, Rgba(palette.negative, faded(verbs.conflictRefusalAgeSeconds)));
    }
    else if (verbs.refusal.has_value() && verbs.refusalAgeSeconds < hud.noticeSeconds)
    {
      const std::array arguments{Neuron::FormatArgument{"reason", RefusalSelector(*verbs.refusal)}};
      centered(STRING_REFUSED, arguments, Rgba(palette.negative, faded(verbs.refusalAgeSeconds)));
    }
    else if (verbs.placing.has_value() && verbs.foreseen.has_value())
    {
      const std::array arguments{Neuron::FormatArgument{"reason", RefusalSelector(*verbs.foreseen)}};
      centered(STRING_REFUSED, arguments, negative);
    }
    else if (verbs.placing.has_value())
    {
      const std::array arguments{Neuron::FormatArgument{"kind", KindSelector(*verbs.placing)}};
      centered(STRING_PLACING, arguments, text);
    }
  }

  // MVP-050, MVP-102 (ADR-166): the relations panel, bottom right.
  if (!m_relationRows.empty())
  {
    plate(true, m_relations, false);
    const float left = m_relationsInner.left + padding;
    const float right = m_relationsInner.right - padding;
    float rowTop = m_relationsInner.top + padding;
    const std::vector<HudRow> rows = RelationPanelRows(_strings, _locale);
    for (std::size_t place = 0; place < m_relationRows.size() && place < rows.size(); ++place)
    {
      const RelationRow& relation = m_relationRows[place];
      drawRow(rows[place], left, right, rowTop);
      // MVP-047: a Cladan under a respite has the respite's icon at its row's end.
      if (relation.kind == RelationRowKind::Cladan && relation.state != 0)
      {
        image(UiImage::IconRespite, RelationButton(place, 1), positive);
      }
      // ADR-167: the row's buttons, the one of the attack being declared on the accent plate.
      const std::array<RelationAction, 2> actions = RelationActionsOf(relation, m_model.settlement->site);
      for (std::size_t slot = 0; slot < actions.size() && place < RELATION_ROWS_ADDRESSED; ++slot)
      {
        if (actions[slot] == RelationAction::None)
        {
          continue;
        }
        const Neuron::UiRect button = RelationButton(place, slot);
        const bool pending = m_model.verbs.pending.has_value() && m_model.verbs.pending->button == place * 2 + slot;
        plate(pending, button, true);
        // A raid on a Cladan under a respite would be refused (MVP-047): its button is dimmed.
        const bool dimmed = actions[slot] == RelationAction::Raid && relation.state != 0;
        image(ActionIcon(actions[slot]), button, Rgba(palette.text, dimmed ? hud.disabledAlpha : 1.0));
      }
      rowTop += row;
    }
  }

  // MVP-102: the notification cards, newest first, fading out over their last mvp.hud.notice_fade.
  if (!m_model.notices.empty())
  {
    const float cardHeight = Units(hud.cardHeight);
    const float gap = Units(CARD_GAP);
    float cardTop = Units(hud.margin);
    for (auto notice = m_model.notices.rbegin(); notice != m_model.notices.rend(); ++notice)
    {
      const double left = hud.noticeSeconds - (m_model.nowSeconds - notice->arrivedSeconds);
      const double alpha = hud.noticeFade > 0.0 ? std::clamp(left / hud.noticeFade, 0.0, 1.0) : 1.0;
      const Neuron::UiRect card = Neuron::UiRect::FromSize(static_cast<float>(m_metrics.client.width) - Units(hud.margin) - m_cardWidth,
                                                           cardTop, m_cardWidth, cardHeight);
      image(UiImage::NotificationCard, card, Rgba(palette.neutral, alpha));
      const HudRow cardRow = NoticeRow(*notice, alpha, _strings, _locale);
      if (cardRow.icon.has_value())
      {
        image(
          *cardRow.icon,
          Neuron::UiRect::FromSize(card.left + padding + cardRow.iconIndent, std::round(card.top + (cardHeight - icon) * 0.5f), icon, icon),
          cardRow.iconColor);
      }
      line(cardRow.text, card.left + padding + cardRow.indent, card.top, cardHeight, cardRow.color);
      cardTop += cardHeight + gap;
    }
  }

  // MVP-005, MVP-060 and MVP-062 (ADR-169): the world's panel, bottom left: its seed, the time left in the run and the
  // standing with each settlement's renown by source.
  plate(true, m_world, false);
  {
    const float left = m_worldInner.left + padding;
    const float right = m_worldInner.right - padding;
    float top = m_worldInner.top + padding;
    std::size_t index = 0;
    for (const HudRow& worldRow : WorldRows(_strings, _locale))
    {
      drawRow(worldRow, left, right, top);
      // O110 (ADR-192): the away report's button, on the accent plate once a report has come, and dim before.
      if (index++ == ALWAYS_ON_ROW && m_model.pace == WorldPace::AlwaysOn)
      {
        plate(m_model.awayReport, m_awayButton, true);
        image(UiImage::IconDeputy, m_awayButton, Rgba(palette.text, m_model.awayReport ? 1.0 : hud.disabledAlpha));
      }
      top += row;
    }
  }

  // MVP-048 (ADR-168): the result card of the settlement's first battle not yet closed, over the rest.
  if (!m_resultRows.empty() && !m_model.results.empty())
  {
    plate(false, m_result, false);
    image(UiImage::ResultCard, m_result, text);
    const float left = m_resultInner.left + padding;
    const float right = m_resultInner.right - padding;
    float top = m_resultInner.top + padding;
    for (const HudRow& resultRow : ResultPanelRows(_strings, _locale))
    {
      drawRow(resultRow, left, right, top);
      top += row;
    }
  }
}

} // namespace Species
