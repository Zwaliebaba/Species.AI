#include "pch.h"

#include "SettlementVerbs.h"

#include "StructureCost.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>

namespace Species
{

namespace
{

constexpr std::size_t TASKS = 3;
constexpr std::int64_t SHARE_TOTAL = 100; // MVP-036: the shares sum to 100 %

bool SumsToTotal(const std::array<std::uint8_t, 3>& _shares) noexcept
{
  return std::int64_t{_shares[0]} + _shares[1] + _shares[2] == SHARE_TOTAL;
}

// The conflict command's kind, for its refusal; none for a command of M2's.
std::optional<ConflictKind> ConflictKindOf(const SettlementCommand& _command) noexcept
{
  if (std::holds_alternative<MakeClaimCommand>(_command) || std::holds_alternative<WithdrawClaimCommand>(_command))
  {
    return ConflictKind::Claim;
  }
  if (std::holds_alternative<DeclareAttackCommand>(_command) || std::holds_alternative<WithdrawAttackCommand>(_command))
  {
    return ConflictKind::Attack;
  }
  if (std::holds_alternative<SendTributeCommand>(_command))
  {
    return ConflictKind::Tribute;
  }
  return std::nullopt;
}

} // namespace

std::vector<StableId> ExpeditionLeadersOf(const StateReplica::View& _latest, std::uint8_t _team, std::optional<StableId> _selected)
{
  std::vector<StableId> leaders;
  for (const auto& [id, state] : _latest)
  {
    const auto* figure = std::get_if<ReplicatedFigure>(&state);
    if (figure != nullptr && figure->kind == FigureKind::Leader && figure->team == _team && !figure->dead)
    {
      leaders.push_back(id);
    }
  }
  // The view is in id order; the selected Leader leads.
  if (const auto selected = std::ranges::find(leaders, _selected.value_or(0)); _selected.has_value() && selected != leaders.end())
  {
    std::rotate(leaders.begin(), selected, std::next(selected));
  }
  leaders.resize(std::min(leaders.size(), EXPEDITION_LEADERS_MAX));
  return leaders;
}

std::optional<PendingAttack> AttackOf(const RelationRow& _row, std::uint8_t _button, const ConflictView& _view, const TuningMvp& _tuning)
{
  if (!_view.valley.has_value())
  {
    return std::nullopt;
  }
  const ValleyStatus& valley = *_view.valley;
  PendingAttack attack;
  attack.button = _button;
  attack.leaders = _view.leaders.size();
  if (_row.kind == RelationRowKind::Cladan)
  {
    attack.target = _row.site;
    // PIL-063: unprovoked with no grievance of its own against the target and no claim on what the target holds.
    std::int64_t angerMicro = 0;
    for (const GrievanceLine& grievance : valley.grievances)
    {
      angerMicro += grievance.holder == _view.site && grievance.against == _row.site ? grievance.weightMicro : 0;
    }
    const bool claimed = std::ranges::any_of(valley.claims,
                                             [&](const ClaimLine& _claim)
                                             {
                                               return _claim.claimant == _view.site && _claim.area < VALLEY_AREAS &&
                                                      valley.areas[_claim.area].holder == std::optional<std::uint8_t>(_row.site);
                                             });
    attack.unprovoked = angerMicro == 0 && !claimed;
  }
  else if (_row.kind == RelationRowKind::Claim && _row.site == _view.site && _row.detail < VALLEY_AREAS)
  {
    // ADR-161: an open claim's battle is fought against the area's holder, a contested one's against a rival claimant.
    attack.area = _row.detail;
    std::optional<std::uint8_t> target = valley.areas[_row.detail].holder;
    if (_row.state == static_cast<std::uint8_t>(ClaimState::Contested) || !target.has_value() || *target == _view.site)
    {
      const auto rival = std::ranges::find_if(valley.claims, [&](const ClaimLine& _claim)
                                              { return _claim.area == _row.detail && _claim.claimant != _view.site; });
      target = rival != valley.claims.end() ? std::optional<std::uint8_t>(rival->claimant) : std::nullopt;
    }
    if (!target.has_value())
    {
      return std::nullopt;
    }
    attack.target = *target;
  }
  else
  {
    return std::nullopt;
  }
  const double factor = attack.unprovoked ? _tuning.pil.conflict.unprovokedCostFactor : 1.0;
  attack.costMilliRu = std::llround(_tuning.mvp.conflict.attackCostRu * factor * 1000.0);
  return attack;
}

std::array<std::uint8_t, 3> ShiftPriority(std::array<std::uint8_t, 3> _shares, std::size_t _task, std::int64_t _deltaPercent) noexcept
{
  if (_task >= TASKS || _deltaPercent == 0)
  {
    return _shares;
  }
  const std::size_t low = std::min((_task + 1) % TASKS, (_task + 2) % TASKS);
  const std::size_t high = std::max((_task + 1) % TASKS, (_task + 2) % TASKS);
  std::array<std::int64_t, 3> share{_shares[0], _shares[1], _shares[2]};
  if (_deltaPercent > 0)
  {
    // A raise takes half from each other task, the larger giving the odd point, and the rest from one that runs out.
    const std::int64_t amount = std::min(_deltaPercent, share[low] + share[high]);
    const std::size_t first = share[high] > share[low] ? high : low;
    const std::size_t second = first == low ? high : low;
    std::int64_t fromFirst = (amount + 1) / 2;
    std::int64_t fromSecond = amount / 2;
    if (fromSecond > share[second])
    {
      fromFirst += fromSecond - share[second];
      fromSecond = share[second];
    }
    if (fromFirst > share[first])
    {
      fromSecond += fromFirst - share[first];
      fromFirst = share[first];
    }
    share[first] -= fromFirst;
    share[second] -= fromSecond;
    share[_task] += amount;
  }
  else
  {
    // A lower gives half to each other task, the smaller taking the odd point.
    const std::int64_t amount = std::min(_deltaPercent < -SHARE_TOTAL ? SHARE_TOTAL : -_deltaPercent, share[_task]);
    const std::size_t first = share[high] < share[low] ? high : low;
    const std::size_t second = first == low ? high : low;
    share[first] += (amount + 1) / 2;
    share[second] += amount / 2;
    share[_task] -= amount;
  }
  return {static_cast<std::uint8_t>(share[0]), static_cast<std::uint8_t>(share[1]), static_cast<std::uint8_t>(share[2])};
}

PlacementView PlacementViewOf(const StateReplica::View& _latest, std::optional<std::uint8_t> _team,
                              const std::optional<SettlementStatus>& _status)
{
  PlacementView view;
  if (!_team.has_value())
  {
    return view;
  }
  for (const auto& [id, state] : _latest)
  {
    if (const auto* settlement = std::get_if<ReplicatedSettlement>(&state); settlement != nullptr && settlement->team == *_team)
    {
      view.settlement = *settlement;
    }
    else if (const auto* structure = std::get_if<ReplicatedStructure>(&state);
             structure != nullptr && structure->kind == StructureKind::Wellspring && structure->owner == _team)
    {
      view.wellspring = PositionOf(structure->position);
    }
  }
  view.dwellings = TeamDwellings(_latest, *_team);
  if (view.settlement.has_value() && _status.has_value() && _status->team == *_team)
  {
    view.status = _status;
  }
  return view;
}

std::optional<PlacementRefusal> PredictPlacement(const PlacementView& _view, const StewardAreas& _areas, StructureKind _kind,
                                                 const Neuron::Vec3& _point, const TuningMvp& _tuning)
{
  if (!_view.settlement.has_value())
  {
    return PlacementRefusal::NotBuildable;
  }
  if (_view.status.has_value() && std::cmp_greater_equal(_view.status->queue.size(), _tuning.mvp.construction.queueMax))
  {
    return PlacementRefusal::QueueFull;
  }
  if (!_areas.InTerritory(_point.x, _point.z))
  {
    return PlacementRefusal::OutsideTerritory;
  }
  // The server measures from the footprint's edge, so a point inside the ring is refused whatever the footprint.
  if (_view.wellspring.has_value() && Neuron::HorizontalDistance(_point, *_view.wellspring) < _tuning.mvp.construction.centerRingRadius)
  {
    return PlacementRefusal::InCenterRing;
  }
  if (std::cmp_less(_view.settlement->storesMilliRu, StructureCostMilliRu(_tuning, _kind, _view.dwellings)))
  {
    return PlacementRefusal::CannotAfford;
  }
  return std::nullopt;
}

SettlementVerbs::SettlementVerbs(const TuningMvp& _tuning, const TuningPresentation& _presentation) noexcept
  : m_tuning(_tuning),
    m_presentation(_presentation)
{
}

void SettlementVerbs::Reset() noexcept
{
  m_placing.reset();
  m_foreseen.reset();
  m_ghost.reset();
  m_placements.clear();
  m_refusal.reset();
  m_refusalAgeSeconds = 0.0;
  m_shares.reset();
  m_pending.reset();
  m_conflicts.clear();
  m_conflictRefusal.reset();
}

SettlementVerbResult SettlementVerbs::Update(const SettlementFrame& _frame, std::optional<std::uint8_t> _team, const PlacementView& _view,
                                             const StewardAreas& _areas)
{
  SettlementVerbResult result;
  // The server's refusal ages on the real clock, and the shares sent show until a status of their tick holds them.
  if (m_refusal.has_value())
  {
    m_refusalAgeSeconds += std::max(_frame.realStepSeconds, 0.0);
    if (m_refusalAgeSeconds >= m_presentation.mvp.hud.noticeSeconds)
    {
      m_refusal.reset();
    }
  }
  if (m_shares.has_value() && m_shares->tick.has_value() && _view.status.has_value() && _view.status->tick >= *m_shares->tick)
  {
    m_shares.reset();
  }
  if (m_conflictRefusal.has_value())
  {
    m_conflictRefusalAgeSeconds += std::max(_frame.realStepSeconds, 0.0);
    if (m_conflictRefusalAgeSeconds >= m_presentation.mvp.hud.noticeSeconds)
    {
      m_conflictRefusal.reset();
    }
  }
  m_foreseen.reset();
  m_ghost.reset();
  if (!_team.has_value() || !_view.settlement.has_value())
  {
    m_placing.reset();
    m_pending.reset();
    return result;
  }
  const std::uint8_t team = *_team;
  // UI-461: a press over the HUD reaches no world verb; the secondary press leaves placing there too.
  if (_frame.hudZone.has_value())
  {
    result.tookPrimary = _frame.primaryDown;
    result.tookSecondary = _frame.secondaryDown;
    if (_frame.primaryDown)
    {
      PressHud(*_frame.hudZone, team, _view, _frame, result);
    }
    else if (_frame.secondaryDown)
    {
      m_placing.reset();
      m_pending.reset();
    }
    return result;
  }
  // ADR-167: the secondary press drops the attack being declared, and reaches no battle verb.
  if (m_pending.has_value() && _frame.secondaryDown && !m_placing.has_value())
  {
    m_pending.reset();
    result.tookSecondary = true;
    return result;
  }
  if (!m_placing.has_value())
  {
    return result;
  }
  // O82: placing takes both presses before the battle verbs, and the secondary press leaves it.
  result.tookPrimary = _frame.primaryDown;
  result.tookSecondary = _frame.secondaryDown;
  if (_frame.secondaryDown)
  {
    m_placing.reset();
    return result;
  }
  if (!_frame.ground.has_value())
  {
    return result;
  }
  const Neuron::Vec3& ground = *_frame.ground;
  m_foreseen = PredictPlacement(_view, _areas, *m_placing, ground, m_tuning);
  // MVP-032 (ADR-132): the structure faces away from its Wellspring, +x where it stands on it (R-BLDG-014).
  StructureGhost ghost{*m_placing, ground, std::numbers::pi * 0.5, team};
  if (_view.wellspring.has_value() && Neuron::HorizontalDistance(ground, *_view.wellspring) > 0.0)
  {
    ghost.yawRadians = std::atan2(ground.x - _view.wellspring->x, ground.z - _view.wellspring->z);
  }
  m_ghost = ghost;
  // ADR-153: the press sends what the client foresees refused too, so that the reason shown is the server's.
  if (_frame.primaryDown)
  {
    PlaceStructureCommand command;
    command.team = team;
    command.kind = *m_placing;
    command.position = {ground.x, ground.z};
    result.commands.emplace_back(command);
  }
  return result;
}

void SettlementVerbs::PressHud(const HudZoneRect& _zone, std::uint8_t _team, const PlacementView& _view, const SettlementFrame& _frame,
                               SettlementVerbResult& _result)
{
  // ADR-167: any press over the HUD but on the declaring button drops the attack being declared.
  if (_zone.zone != HudZone::RelationAction || !m_pending.has_value() || m_pending->button != _zone.index)
  {
    m_pending.reset();
  }
  switch (_zone.zone)
  {
  case HudZone::RelationAction:
    PressRelation(_zone.index, _team, _frame, _result);
    break;
  case HudZone::Build:
    // MVP-101: an entry enters placing its structure, and leaves it when it is the one being placed.
    if (_zone.index < BUILD_KINDS.size())
    {
      const StructureKind kind = BUILD_KINDS[_zone.index];
      m_placing = m_placing == kind ? std::nullopt : std::optional<StructureKind>(kind);
    }
    break;
  case HudZone::PriorityRaise:
  case HudZone::PriorityLower:
  {
    // MVP-036: the shares move from those last sent, or the status's.
    if (!_view.status.has_value() || _zone.index >= TASKS)
    {
      break;
    }
    const std::array<std::uint8_t, 3> from = m_shares.has_value() ? m_shares->shares : _view.status->priorities;
    const std::int64_t step = m_presentation.mvp.hud.priorityStep;
    const std::array<std::uint8_t, 3> to = ShiftPriority(from, _zone.index, _zone.zone == HudZone::PriorityRaise ? step : -step);
    if (to != from && SumsToTotal(to))
    {
      SetPrioritiesCommand command;
      command.team = _team;
      command.shares = to;
      _result.commands.emplace_back(command);
    }
    break;
  }
  case HudZone::Result:
    ++m_resultsClosed; // ADR-168: the press closes the result the card shows
    break;
  case HudZone::Settlement:
  case HudZone::Queue:
  case HudZone::Notices:
  case HudZone::Relations:
  case HudZone::World:
    break;
  }
}

void SettlementVerbs::PressRelation(std::uint8_t _button, std::uint8_t _team, const SettlementFrame& _frame, SettlementVerbResult& _result)
{
  const std::size_t place = _button / 2U;
  if (place >= _frame.relations.size())
  {
    return;
  }
  const RelationRow& row = _frame.relations[place];
  const RelationAction action = RelationActionsOf(row, _frame.conflict.site)[_button % 2U];
  switch (action)
  {
  case RelationAction::Raid:
  case RelationAction::ClaimBattle:
  {
    // MVP-101: the first press shows the attack with its cost; a second on the same button declares it.
    if (m_pending.has_value() && m_pending->button == _button)
    {
      DeclareAttackCommand command;
      command.team = _team;
      std::copy_n(_frame.conflict.leaders.begin(), std::min(_frame.conflict.leaders.size(), command.leaders.size()),
                  command.leaders.begin());
      command.target = m_pending->target;
      command.claim = action == RelationAction::ClaimBattle ? row.subject : 0;
      _result.commands.emplace_back(command);
      m_pending.reset();
    }
    else
    {
      m_pending = AttackOf(row, _button, _frame.conflict, m_tuning);
      m_placing.reset(); // the secondary press then drops the attack, as it leaves placing
    }
    break;
  }
  case RelationAction::WithdrawClaim:
    _result.commands.emplace_back(WithdrawClaimCommand{_team, row.subject});
    break;
  case RelationAction::WithdrawAttack:
    _result.commands.emplace_back(WithdrawAttackCommand{_team});
    break;
  case RelationAction::Tribute:
  {
    // MVP-054: the stores are offered, of which the server takes only what eases the grievance whole at its holder's
    // rate (MVP-057, ADR-160), which the client does not know; it refuses an empty offer.
    SendTributeCommand command;
    command.team = _team;
    command.to = row.site;
    command.cause = row.detail;
    command.subject = row.subject;
    command.amountMilliRu = std::max<std::int64_t>(_frame.conflict.storesMilliRu, 0);
    _result.commands.emplace_back(command);
    break;
  }
  case RelationAction::Claim:
    _result.commands.emplace_back(MakeClaimCommand{_team, row.detail});
    break;
  case RelationAction::None:
    break;
  }
}

void SettlementVerbs::Sent(std::uint64_t _sequence, const SettlementCommand& _command)
{
  if (std::holds_alternative<PlaceStructureCommand>(_command))
  {
    m_placements.push_back(_sequence);
  }
  else if (const auto* priorities = std::get_if<SetPrioritiesCommand>(&_command); priorities != nullptr)
  {
    m_shares = SentShares{priorities->shares, _sequence, std::nullopt};
  }
  else if (const std::optional<ConflictKind> kind = ConflictKindOf(_command))
  {
    m_conflicts.emplace_back(_sequence, *kind);
  }
}

void SettlementVerbs::Reported(const CommandReport& _report)
{
  // ADR-167: a conflict command's refusal with its reason, which its kind's refusals name.
  if (const auto conflict = std::ranges::find(m_conflicts, _report.sequence, &std::pair<std::uint64_t, ConflictKind>::first);
      conflict != m_conflicts.end())
  {
    if (_report.result == CommandResult::Applied)
    {
      m_conflictRefusal.reset();
    }
    else
    {
      m_conflictRefusal = std::pair{conflict->second, _report.reason};
      m_conflictRefusalAgeSeconds = 0.0;
    }
    m_conflicts.erase(conflict);
    return;
  }
  if (const auto placement = std::ranges::find(m_placements, _report.sequence); placement != m_placements.end())
  {
    m_placements.erase(placement);
    // MVP-101 (ADR-136): a refusal's reason travels in its outcome; one with no placement reason shows as not buildable.
    if (_report.result == CommandResult::Applied)
    {
      m_refusal.reset();
    }
    else
    {
      const bool known =
        _report.reason >= static_cast<std::uint8_t>(PlacementRefusal::NotBuildable) && _report.reason <= PLACEMENT_REFUSAL_MAX;
      m_refusal = known ? static_cast<PlacementRefusal>(_report.reason) : PlacementRefusal::NotBuildable;
      m_refusalAgeSeconds = 0.0;
    }
    return;
  }
  if (m_shares.has_value() && m_shares->sequence == _report.sequence)
  {
    if (_report.result == CommandResult::Applied)
    {
      m_shares->tick = _report.tick;
    }
    else
    {
      m_shares.reset();
    }
  }
}

HudVerbs SettlementVerbs::ForHud() const
{
  HudVerbs verbs;
  verbs.placing = m_placing;
  verbs.foreseen = m_foreseen;
  verbs.refusal = m_refusal;
  verbs.refusalAgeSeconds = m_refusalAgeSeconds;
  if (m_shares.has_value())
  {
    verbs.shares = m_shares->shares;
  }
  verbs.pending = m_pending;
  verbs.conflictRefusal = m_conflictRefusal;
  verbs.conflictRefusalAgeSeconds = m_conflictRefusalAgeSeconds;
  verbs.resultsClosed = m_resultsClosed;
  return verbs;
}

} // namespace Species
