#include "pch.h"

#include "Battles.h"
#include "Claims.h"
#include "CommandMessages.h"
#include "Formation.h"
#include "MessageCodec.h"
#include "Profile.h"
#include "RegionSimulation.h"
#include "RelationMessages.h"
#include "Relations.h"
#include "ScriptedSteward.h"
#include "SeenLayer.h"
#include "SettlementAi.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>

namespace Species
{

namespace
{

// A region's local sites are 0 to 3 (WLD-040).
constexpr std::uint64_t LOCAL_SITE_MAX = 3;
// The most Leaders a muster holds (O98) and its largest wish: bounds for the reader.
constexpr std::uint64_t MUSTER_LEADERS_MAX = EXPEDITION_LEADERS_MAX;
constexpr double SECONDS_PER_MINUTE = 60.0;

template <class T> void Send(RegionSimulation& _region, ScriptedSteward& _steward, const T& _command)
{
  static_cast<void>(_region.Commands().Submit(_steward.id, _steward.nextSequence++, Neuron::EncodeMessage(_command)));
}

GroundPoint Ground(const Neuron::Vec3& _point) noexcept
{
  return {_point.x, _point.z};
}

const Settlement* SettlementOf(const RegionSimulation& _region, std::uint8_t _team) noexcept
{
  const auto found = std::ranges::find(_region.Settlements(), _team, &Settlement::team);
  return found == _region.Settlements().end() ? nullptr : &*found;
}

Neuron::Vec3 WellspringOf(const RegionSimulation& _region, const Settlement& _settlement)
{
  const Structure* wellspring = _region.ResolveStructure(_settlement.wellspringSlot, _settlement.wellspringId);
  return wellspring != nullptr ? wellspring->position : _region.Terrain()->layout.sites[_settlement.site];
}

bool Defends(const RegionSimulation& _region, std::uint8_t _site) noexcept
{
  return std::ranges::any_of(_region.Battles(), [_site](const Battle& _battle) { return _battle.defender == _site; });
}

// A fresh battle core of the steward's for _team, its first main cycle drawn as an AI's is (R-BOT-009).
AiController FreshCore(RegionSimulation& _region, const ScriptedSteward& _steward, std::uint8_t _team)
{
  return MakeAi(_region, _steward.id, _team);
}

// MVP-049 and MVP-058: each battle core steps while its side fights; a core starts fresh with each battle.
void StepCores(RegionSimulation& _region, ScriptedSteward& _steward, SettlementAi& _ai, const Settlement& _settlement)
{
  if (Defends(_region, _settlement.site))
  {
    if (!_ai.defending)
    {
      _ai.defense = FreshCore(_region, _steward, _settlement.team);
      _ai.defending = true;
    }
    StepBattleCore(_region, _ai.defense, _steward.nextSequence, std::nullopt);
  }
  else
  {
    _ai.defending = false;
  }
  const Expedition* expedition = ExpeditionOf(_region, _settlement.site);
  const Battle* battle = expedition != nullptr ? BattleOf(_region, expedition->id) : nullptr;
  if (battle != nullptr)
  {
    if (!_ai.attacking)
    {
      _ai.attack = FreshCore(_region, _steward, ExpeditionTeamOf(_settlement.site));
      _ai.attacking = true;
    }
    StepBattleCore(_region, _ai.attack, _steward.nextSequence, battle->objective);
  }
  else
  {
    _ai.attacking = false;
  }
}

// The living, undemoted Leaders of _team, in slot order.
std::vector<const Figure*> LeadersOf(const RegionSimulation& _region, std::uint8_t _team)
{
  std::vector<const Figure*> leaders;
  const SlicedPool<Figure>& figures = _region.TeamAt(_team).figures;
  for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
  {
    if (figures.Used(slot) && figures.At(slot).kind == FigureKind::Leader && !figures.At(slot).dead && !figures.At(slot).demoted)
    {
      leaders.push_back(&figures.At(slot));
    }
  }
  return leaders;
}

// The living Workers of _team that follow one of _leaders.
std::uint32_t FollowersOf(const RegionSimulation& _region, std::uint8_t _team, const std::vector<StableId>& _leaders)
{
  std::uint32_t followers = 0;
  const SlicedPool<Figure>& figures = _region.TeamAt(_team).figures;
  for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
  {
    const bool follows = figures.Used(slot) && figures.At(slot).kind == FigureKind::Worker && !figures.At(slot).dead &&
                         figures.At(slot).leaderId != 0 && std::ranges::find(_leaders, figures.At(slot).leaderId) != _leaders.end();
    followers += follows ? 1U : 0U;
  }
  return followers;
}

// The Workers that follow _leader.
std::uint32_t FollowersOfOne(const RegionSimulation& _region, std::uint8_t _team, StableId _leader)
{
  return FollowersOf(_region, _team, {_leader});
}

// MVP-057 and DEC-1006: the area the AI settlement's temperament claims next, if any.
std::optional<std::uint8_t> ClaimToMake(const RegionSimulation& _region, const Settlement& _settlement)
{
  const std::uint8_t site = _settlement.site;
  switch (_settlement.cladan.temperament)
  {
  case Temperament::Bold:
    // The central resource site, from its first decision.
    if (!CheckClaim(_region, site, CLAIM_AREA_CENTRAL))
    {
      return CLAIM_AREA_CENTRAL;
    }
    return std::nullopt;
  case Temperament::Territorial:
    // Every half of its border areas that its neighbour holds.
    for (std::uint8_t area = 0; area < CLAIM_AREA_CENTRAL; ++area)
    {
      if (!CheckClaim(_region, site, area))
      {
        return area;
      }
    }
    return std::nullopt;
  case Temperament::Wary:
  {
    // Only once attacked: the halves its attackers hold.
    std::array<bool, LOCAL_SITE_MAX + 1> attackers{};
    for (const Grievance& grievance : _region.Grievances())
    {
      if (grievance.holder == site && grievance.cause == GrievanceCause::Attack && grievance.against <= LOCAL_SITE_MAX)
      {
        attackers[grievance.against] = true;
      }
    }
    for (const BattleResult& result : _region.BattleResults())
    {
      if (result.defender == site)
      {
        attackers[result.attacker] = true;
      }
    }
    for (std::uint8_t area = 0; area < CLAIM_AREA_CENTRAL; ++area)
    {
      const std::optional<std::uint8_t> holder = HolderOf(_region, area);
      if (holder && *holder <= LOCAL_SITE_MAX && attackers[*holder] && !CheckClaim(_region, site, area))
      {
        return area;
      }
    }
    return std::nullopt;
  }
  case Temperament::None:
    break;
  }
  return std::nullopt;
}

// MVP-055: the oldest of the settlement's Open or Contested claims that an attack on _target would settle, or 0.
StableId ClaimAgainst(const RegionSimulation& _region, std::uint8_t _site, std::uint8_t _target)
{
  for (const Claim& claim : _region.Claims())
  {
    if (claim.claimant != _site || claim.state == ClaimState::Waiting)
    {
      continue;
    }
    const std::optional<std::uint8_t> holder = HolderOf(_region, claim.area);
    const bool rival = !holder && std::ranges::any_of(_region.Claims(), [&](const Claim& _other)
                                                      { return _other.area == claim.area && _other.claimant == _target; });
    if (holder == std::optional<std::uint8_t>(_target) || rival)
    {
      return claim.id;
    }
  }
  return 0;
}

struct Candidate
{
  std::uint8_t target = 0;
  StableId claim = 0; // 0 for a raid
  std::int64_t angerMicro = 0;
};

// MVP-055 and PIL-025: the attack the settlement may make on _target: a raid while its anger reaches its threshold, a
// claim battle for its claim on what the target holds, as its temperament prefers (MVP-057); none for neither, and none
// in the run's quiet minutes (MVP-060).
std::optional<Candidate> CandidateOf(const RegionSimulation& _region, const Settlement& _settlement, std::uint8_t _target)
{
  const std::optional<double> threshold = AngerThresholdOf(_region.Tuning(), _settlement.cladan.temperament);
  if (!threshold || _target == _settlement.site || _region.SettlementAt(_target) == nullptr || UnderRespite(_region, _target) ||
      _region.InQuietMinutes())
  {
    return std::nullopt;
  }
  const std::int64_t anger = AngerMicro(_region, _settlement.site, _target);
  const bool raid = anger > 0 && static_cast<double>(anger) >= *threshold * static_cast<double>(WEIGHT_MICRO);
  const StableId claim = ClaimAgainst(_region, _settlement.site, _target);
  if (!raid && claim == 0)
  {
    return std::nullopt;
  }
  const bool prefersRaid = _settlement.cladan.temperament == Temperament::Bold;
  return Candidate{_target, (prefersRaid && raid) || claim == 0 ? 0 : claim, anger};
}

// The Workers the muster promotes: the living Workers of the team that follow no Leader nearest its Wellspring.
std::vector<StableId> PromotionsOf(const RegionSimulation& _region, const Settlement& _settlement, std::size_t _count)
{
  const Neuron::Vec3 wellspring = WellspringOf(_region, _settlement);
  std::vector<std::pair<double, StableId>> candidates;
  const SlicedPool<Figure>& figures = _region.TeamAt(_settlement.team).figures;
  for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
  {
    if (!figures.Used(slot))
    {
      continue;
    }
    const Figure& worker = figures.At(slot);
    if (worker.kind == FigureKind::Worker && !worker.dead && !worker.promoted && worker.leaderId == 0 &&
        worker.workerState != WorkerState::Burning)
    {
      candidates.emplace_back(Neuron::HorizontalDistance(worker.position, wellspring), worker.id);
    }
  }
  std::ranges::sort(candidates);
  std::vector<StableId> chosen;
  for (std::size_t place = 0; place < candidates.size() && chosen.size() < _count; ++place)
  {
    chosen.push_back(candidates[place].second);
  }
  return chosen;
}

std::uint64_t MusterTicks(const TuningMvp& _tuning) noexcept
{
  return static_cast<std::uint64_t>(
    std::max<std::int64_t>(1, std::llround(_tuning.mvp.ai.musterMinutes * SECONDS_PER_MINUTE / _tuning.ref.sim.tickPeriod)));
}

// A new muster against the settlement the AI is angriest with among those it may attack and can field enough against.
void BeginMuster(RegionSimulation& _region, ScriptedSteward& _steward, SettlementAi& _ai, const Settlement& _settlement)
{
  const TuningMvp& tuning = _region.Tuning();
  std::vector<Candidate> candidates;
  for (const Settlement& other : _region.Settlements())
  {
    if (const std::optional<Candidate> candidate = CandidateOf(_region, _settlement, other.site))
    {
      candidates.push_back(*candidate);
    }
  }
  // PIL-025: the one it holds most against first, the lower site on a tie.
  std::ranges::stable_sort(candidates, [](const Candidate& _a, const Candidate& _b) { return _a.angerMicro > _b.angerMicro; });
  const std::uint32_t fieldable = FieldableWorkers(_region, _settlement);
  const std::int64_t cost = MilliRuOf(tuning.mvp.conflict.attackCostRu);
  for (const Candidate& candidate : candidates)
  {
    const double seen = static_cast<double>(SeenWorkersOf(_region, _settlement.site, candidate.target));
    const auto need = std::max<std::uint32_t>(1, static_cast<std::uint32_t>(std::ceil(tuning.mvp.ai.attackMinShare * seen)));
    if (fieldable < need)
    {
      continue;
    }
    if (_settlement.storesMilliRu < cost)
    {
      return; // it waits for its stores
    }
    Muster muster;
    muster.target = candidate.target;
    muster.claim = candidate.claim;
    muster.size = fieldable;
    muster.need = need;
    muster.startTick = _region.NextTick();
    const auto formation = static_cast<std::uint32_t>(std::max<std::int64_t>(1, tuning.ref.sim.team.formationSizeUpgrade));
    muster.leadersWanted =
      static_cast<std::uint8_t>(std::min<std::uint32_t>(EXPEDITION_LEADERS_MAX, (fieldable + formation - 1) / formation));
    for (const StableId worker : PromotionsOf(_region, _settlement, muster.leadersWanted))
    {
      Send(_region, _steward, PromoteWorkerCommand{_settlement.team, worker});
    }
    _ai.muster = muster;
    return;
  }
}

// The muster's next step: its Leaders adopted and formed up, then its declaration once enough Workers follow them, or its
// end when it can no longer be made or its time runs out (ADR-162). Returns the Leaders a declaration names.
std::vector<StableId> AdvanceMuster(RegionSimulation& _region, ScriptedSteward& _steward, SettlementAi& _ai, Muster& _muster,
                                    const Settlement& _settlement)
{
  const TuningMvp& tuning = _region.Tuning();
  Muster& muster = _muster;
  // Its Leaders: those still alive, then the team's others in slot order, as many as it wants.
  const std::vector<const Figure*> leaders = LeadersOf(_region, _settlement.team);
  std::erase_if(muster.leaders,
                [&](StableId _id) { return std::ranges::none_of(leaders, [_id](const Figure* _leader) { return _leader->id == _id; }); });
  for (const Figure* leader : leaders)
  {
    if (muster.leaders.size() < muster.leadersWanted && std::ranges::find(muster.leaders, leader->id) == muster.leaders.end())
    {
      muster.leaders.push_back(leader->id);
    }
  }
  const bool still = CandidateOf(_region, _settlement, muster.target).has_value() &&
                     (muster.claim == 0 || ClaimAgainst(_region, _settlement.site, muster.target) == muster.claim);
  const bool late = _region.NextTick() >= muster.startTick + MusterTicks(tuning);
  const std::uint32_t followers = FollowersOf(_region, _settlement.team, muster.leaders);
  if (still && !muster.leaders.empty() && (followers >= muster.size || (late && followers >= muster.need)))
  {
    // Its Leaders in adoption order while their Workers stay within mvp.expedition.size_max.
    DeclareAttackCommand declaration{_settlement.team, {}, muster.target, muster.claim};
    std::size_t named = 0;
    std::uint32_t sent = 0;
    for (const StableId leader : muster.leaders)
    {
      const std::uint32_t own = FollowersOfOne(_region, _settlement.team, leader);
      if (named < declaration.leaders.size() && std::int64_t{sent} + own <= tuning.mvp.expedition.sizeMax)
      {
        declaration.leaders[named++] = leader;
        sent += own;
      }
    }
    Send(_region, _steward, declaration);
    _ai.muster.reset();
    return {declaration.leaders.begin(), declaration.leaders.begin() + static_cast<std::ptrdiff_t>(named)};
  }
  if (!still || late)
  {
    _ai.muster.reset();
    return {};
  }
  // Each of its Leaders in formation where it stands, facing the target, so that Workers on guard duty join it
  // (MVP-041); and the Workers still to promote.
  const Settlement* target = _region.SettlementAt(muster.target);
  for (const Figure* leader : leaders)
  {
    if (std::ranges::find(muster.leaders, leader->id) != muster.leaders.end() && !InFormationMode(*leader) && target != nullptr)
    {
      Send(_region, _steward,
           AiOrderLeaderCommand{_settlement.team, leader->id, LeaderOperation::FormUp, false, Ground(WellspringOf(_region, *target))});
    }
  }
  if (muster.leaders.size() < muster.leadersWanted)
  {
    for (const StableId worker : PromotionsOf(_region, _settlement, muster.leadersWanted - muster.leaders.size()))
    {
      Send(_region, _steward, PromoteWorkerCommand{_settlement.team, worker});
    }
  }
  return {};
}

// MVP-055 to MVP-057: one decision of the AI settlement.
void Decide(RegionSimulation& _region, ScriptedSteward& _steward, SettlementAi& _ai, const Settlement& _settlement)
{
  // Its claim, one a decision.
  if (const std::optional<std::uint8_t> area = ClaimToMake(_region, _settlement))
  {
    Send(_region, _steward, MakeClaimCommand{_settlement.team, *area});
  }
  const bool out = ExpeditionOf(_region, _settlement.site) != nullptr;
  const bool defending = Defends(_region, _settlement.site);
  if (_ai.muster && (out || defending))
  {
    _ai.muster.reset();
  }
  std::vector<StableId> declared;
  if (_ai.muster)
  {
    declared = AdvanceMuster(_region, _steward, _ai, *_ai.muster, _settlement);
  }
  else if (!out && !defending)
  {
    BeginMuster(_region, _steward, _ai, _settlement);
  }
  // Its Leaders outside its muster, and outside a defense, go back to work (ADR-162).
  if (defending)
  {
    return;
  }
  for (const Figure* leader : LeadersOf(_region, _settlement.team))
  {
    const bool mustered = _ai.muster && std::ranges::find(_ai.muster->leaders, leader->id) != _ai.muster->leaders.end();
    if (!mustered && std::ranges::find(declared, leader->id) == declared.end())
    {
      Send(_region, _steward, DemoteLeaderCommand{_settlement.team, leader->id});
    }
  }
}

} // namespace

bool SettlementAi::operator==(const SettlementAi& _other) const = default;

SettlementAi MakeSettlementAi(const ScriptedSteward& _steward, std::uint8_t _expeditionTeam)
{
  SettlementAi ai;
  ai.team = _steward.team;
  ai.defense.id = _steward.id;
  ai.defense.team = _steward.team;
  ai.attack.id = _steward.id;
  ai.attack.team = _expeditionTeam;
  return ai;
}

std::optional<double> AngerThresholdOf(const TuningMvp& _tuning, Temperament _temperament) noexcept
{
  switch (_temperament)
  {
  case Temperament::Bold:
    return _tuning.mvp.temperament.boldThreshold;
  case Temperament::Territorial:
    return _tuning.mvp.temperament.territorialThreshold;
  case Temperament::Wary:
    return _tuning.mvp.temperament.waryThreshold;
  case Temperament::None:
    break;
  }
  return std::nullopt;
}

std::uint32_t SeenWorkersOf(const RegionSimulation& _region, std::uint8_t _viewer, std::uint8_t _target)
{
  const Settlement* viewer = _region.SettlementAt(_viewer);
  if (viewer == nullptr)
  {
    return 0;
  }
  const Sight sight = Sight::OfTeam(_region, viewer->team);
  std::uint32_t seen = 0;
  for (const std::uint8_t team : {_target, ExpeditionTeamOf(_target)})
  {
    if (team >= _region.TeamCount())
    {
      continue;
    }
    const SlicedPool<Figure>& figures = _region.TeamAt(team).figures;
    for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
    {
      const bool counted = figures.Used(slot) && figures.At(slot).kind == FigureKind::Worker && !figures.At(slot).dead &&
                           sight.Sees(figures.At(slot).position.x, figures.At(slot).position.z);
      seen += counted ? 1U : 0U;
    }
  }
  return seen;
}

std::uint32_t FieldableWorkers(const RegionSimulation& _region, const Settlement& _settlement)
{
  if (_settlement.team >= _region.TeamCount())
  {
    return 0;
  }
  std::int64_t workers = 0;
  const SlicedPool<Figure>& figures = _region.TeamAt(_settlement.team).figures;
  for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
  {
    workers += figures.Used(slot) && figures.At(slot).kind == FigureKind::Worker && !figures.At(slot).dead ? 1 : 0;
  }
  const auto& tuning = _region.Tuning().mvp;
  return static_cast<std::uint32_t>(std::clamp<std::int64_t>(workers - tuning.protection.workerFloor, 0, tuning.expedition.sizeMax));
}

void StepSettlementAi(RegionSimulation& _region, ScriptedSteward& _steward, SettlementAi& _ai)
{
  const Settlement* settlement = SettlementOf(_region, _ai.team);
  if (settlement == nullptr || settlement->cladan.temperament == Temperament::None || _region.Terrain() == nullptr)
  {
    return;
  }
  {
    ENGINE_PROFILE_SCOPE("sim.settlement_ai.cores");
    StepCores(_region, _steward, _ai, *settlement);
  }
  if (--_ai.decisionTicks > 0)
  {
    return;
  }
  ENGINE_PROFILE_SCOPE("sim.settlement_ai.decide");
  _ai.decisionTicks = static_cast<std::int32_t>(std::max<std::int64_t>(1, _region.Tuning().mvp.ai.decisionTicks));
  Decide(_region, _steward, _ai, *settlement);
}

void WriteSettlementAi(Neuron::BitWriter& _writer, const SettlementAi& _ai)
{
  _writer.WriteBits(_ai.team, 8);
  _writer.WriteSigned(_ai.decisionTicks, 32);
  _writer.WriteBool(_ai.muster.has_value());
  if (_ai.muster)
  {
    const Muster& muster = *_ai.muster;
    _writer.WriteBounded(muster.target, LOCAL_SITE_MAX);
    _writer.WriteBits(muster.claim, 64);
    _writer.WriteBits(muster.size, 32);
    _writer.WriteBits(muster.need, 32);
    _writer.WriteBits(muster.startTick, 64);
    _writer.WriteBounded(muster.leadersWanted, MUSTER_LEADERS_MAX);
    _writer.WriteBounded(muster.leaders.size(), MUSTER_LEADERS_MAX);
    for (const StableId leader : muster.leaders)
    {
      _writer.WriteBits(leader, 64);
    }
  }
  _writer.WriteBool(_ai.defending);
  WriteAi(_writer, _ai.defense);
  _writer.WriteBool(_ai.attacking);
  WriteAi(_writer, _ai.attack);
}

bool ReadSettlementAi(Neuron::BitReader& _reader, SettlementAi& _ai)
{
  _ai.team = static_cast<std::uint8_t>(_reader.ReadBits(8));
  _ai.decisionTicks = static_cast<std::int32_t>(_reader.ReadSigned(32));
  _ai.muster.reset();
  if (_reader.ReadBool())
  {
    Muster muster;
    muster.target = static_cast<std::uint8_t>(_reader.ReadBounded(LOCAL_SITE_MAX));
    muster.claim = _reader.ReadBits(64);
    muster.size = static_cast<std::uint32_t>(_reader.ReadBits(32));
    muster.need = static_cast<std::uint32_t>(_reader.ReadBits(32));
    muster.startTick = _reader.ReadBits(64);
    muster.leadersWanted = static_cast<std::uint8_t>(_reader.ReadBounded(MUSTER_LEADERS_MAX));
    muster.leaders.resize(_reader.ReadBounded(MUSTER_LEADERS_MAX));
    for (StableId& leader : muster.leaders)
    {
      leader = _reader.ReadBits(64);
    }
    _ai.muster = muster;
  }
  _ai.defending = _reader.ReadBool();
  bool whole = ReadAi(_reader, _ai.defense);
  _ai.attacking = _reader.ReadBool();
  whole = ReadAi(_reader, _ai.attack) && whole;
  return !_reader.Failed() && whole && _ai.decisionTicks > 0;
}

} // namespace Species
