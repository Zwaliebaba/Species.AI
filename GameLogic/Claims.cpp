#include "pch.h"

#include "Claims.h"
#include "Construction.h"
#include "RegionSimulation.h"
#include "Relations.h"
#include "Renown.h"
#include "ReplicatedState.h"
#include "Statistics.h"
#include "Territory.h"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace Species
{

namespace
{

constexpr double SECONDS_PER_MINUTE = 60.0;
// A region's local sites are 0 to 3 (WLD-040).
constexpr std::uint64_t LOCAL_SITE_MAX = 3;

// MVP-057: the rate at which a Cladan of the temperament eases a grievance by tribute: Bold's standard, Territorial's
// half, Wary's twice; the steward's settlement, which has none, the standard.
double TributeRate(Temperament _temperament) noexcept
{
  switch (_temperament)
  {
  case Temperament::Territorial:
    return 0.5;
  case Temperament::Wary:
    return 2.0;
  case Temperament::None:
  case Temperament::Bold:
    break;
  }
  return 1.0;
}

// The area's holding, made or replaced; a half that a claim moves back to its own side's settlement has none.
void Hold(RegionSimulation& _region, std::uint8_t _area, std::uint8_t _holder)
{
  std::vector<AreaHolding>& holdings = _region.Holdings();
  std::erase_if(holdings, [_area](const AreaHolding& _holding) { return _holding.area == _area; });
  if (_area == CLAIM_AREA_CENTRAL || OwnSiteOfArea(_area) != _holder)
  {
    holdings.insert(std::ranges::upper_bound(holdings, _area, {}, &AreaHolding::area), AreaHolding{_area, _holder});
  }
}

// The claim's end: removed, and its grievance's cause with it (PIL-122); a contested claim left alone on the central
// resource site waits again.
void End(RegionSimulation& _region, StableId _claim)
{
  std::erase_if(_region.Claims(), [_claim](const Claim& _held) { return _held.id == _claim; });
  RemoveCause(_region, GrievanceCause::Claim, _claim);
  if (std::ranges::count(_region.Claims(), std::uint8_t{CLAIM_AREA_CENTRAL}, &Claim::area) == 1)
  {
    for (Claim& alone : _region.Claims())
    {
      if (alone.area == CLAIM_AREA_CENTRAL && alone.state == ClaimState::Contested)
      {
        alone.state = ClaimState::Waiting;
      }
    }
  }
}

} // namespace

std::uint8_t OwnSiteOfArea(std::uint8_t _area) noexcept
{
  return VALLEY_PAIRS[PairOfArea(_area)][_area % 2U];
}

std::optional<std::uint8_t> MovedTo(const RegionSimulation& _region, std::uint8_t _area) noexcept
{
  for (const AreaHolding& holding : _region.Holdings())
  {
    if (holding.area == _area)
    {
      return holding.holder;
    }
  }
  return std::nullopt;
}

std::optional<std::uint8_t> HolderOf(const RegionSimulation& _region, std::uint8_t _area)
{
  if (const std::optional<std::uint8_t> moved = MovedTo(_region, _area))
  {
    return moved;
  }
  if (_area == CLAIM_AREA_CENTRAL)
  {
    return std::nullopt;
  }
  // WLD-063: its own side's settlement holds it from the tick its territory enters it.
  const std::uint8_t own = OwnSiteOfArea(_area);
  const CellRaster* territory = _region.TerritoryOf(own);
  const std::optional<CellRaster::ChunkBounds> bounds = territory != nullptr ? territory->HeldChunks() : std::nullopt;
  if (!bounds)
  {
    return std::nullopt;
  }
  for (std::int32_t z = bounds->firstZ * CELL_CHUNK_SIDE; z < (bounds->lastZ + 1) * CELL_CHUNK_SIDE; ++z)
  {
    for (std::int32_t x = bounds->firstX * CELL_CHUNK_SIDE; x < (bounds->lastX + 1) * CELL_CHUNK_SIDE; ++x)
    {
      if (territory->HoldsCell(x, z))
      {
        const Neuron::Vec3 center = territory->CellCenter(x, z);
        if (InBorderHalf(_region, _area, center.x, center.z))
        {
          return own;
        }
      }
    }
  }
  return std::nullopt;
}

std::optional<ClaimRefusal> CheckClaim(const RegionSimulation& _region, std::uint8_t _claimant, std::uint8_t _area)
{
  if (_region.SettlementAt(_claimant) == nullptr || _region.Terrain() == nullptr)
  {
    return ClaimRefusal::NoSettlement;
  }
  if (_area > CLAIM_AREA_MAX)
  {
    return ClaimRefusal::NotClaimable;
  }
  const std::optional<std::uint8_t> holder = HolderOf(_region, _area);
  if (_area != CLAIM_AREA_CENTRAL)
  {
    // WLD-086: only a settlement of the half's pair, and a half that its neighbour holds.
    const auto& sites = VALLEY_PAIRS[PairOfArea(_area)];
    if (sites[0] != _claimant && sites[1] != _claimant)
    {
      return ClaimRefusal::NotClaimable;
    }
    const std::uint8_t neighbor = sites[0] == _claimant ? sites[1] : sites[0];
    if (holder == std::optional<std::uint8_t>(_claimant))
    {
      return ClaimRefusal::AlreadyHeld;
    }
    if (holder != std::optional<std::uint8_t>(neighbor))
    {
      return ClaimRefusal::NotHeld;
    }
  }
  else if (holder == std::optional<std::uint8_t>(_claimant))
  {
    return ClaimRefusal::AlreadyHeld;
  }
  if (std::ranges::any_of(_region.Claims(), [&](const Claim& _claim) { return _claim.claimant == _claimant && _claim.area == _area; }))
  {
    return ClaimRefusal::AlreadyClaimed;
  }
  return std::nullopt;
}

std::expected<StableId, ClaimRefusal> MakeClaim(RegionSimulation& _region, std::uint8_t _claimant, std::uint8_t _area)
{
  if (const std::optional<ClaimRefusal> refusal = CheckClaim(_region, _claimant, _area))
  {
    return std::unexpected(*refusal);
  }
  Claim claim{_region.IssueId(), _claimant, _area, _region.NextTick(), ClaimState::Open};
  const std::optional<std::uint8_t> holder = HolderOf(_region, _area);
  if (holder)
  {
    // MVP-051: a claim on a held area gives its holder's Cladan a grievance against the claimant.
    AddGrievance(_region, *holder, _claimant, GrievanceCause::Claim, claim.id);
  }
  else
  {
    // The central resource site while nobody holds it: waiting alone, contested with every other claim on it.
    bool contested = false;
    for (Claim& other : _region.Claims())
    {
      if (other.area == CLAIM_AREA_CENTRAL)
      {
        other.state = ClaimState::Contested;
        contested = true;
      }
    }
    claim.state = contested ? ClaimState::Contested : ClaimState::Waiting;
  }
  _region.Claims().push_back(claim);
  RecordEvent(_region, _claimant, StatisticKind::ClaimMade, _area);
  return claim.id;
}

std::optional<ClaimRefusal> WithdrawClaim(RegionSimulation& _region, std::uint8_t _claimant, StableId _claim)
{
  const auto found = std::ranges::find(_region.Claims(), _claim, &Claim::id);
  if (_region.SettlementAt(_claimant) == nullptr)
  {
    return ClaimRefusal::NoSettlement;
  }
  if (found == _region.Claims().end() || found->claimant != _claimant)
  {
    return ClaimRefusal::NoSuchClaim;
  }
  // MVP-061 (ADR-163, DEC-1007): the weight its grievance holds when the claim is withdrawn is eased by its claimant, once
  // the claim has stood mvp.claim.uncontested_minutes, so that a claim made only to be withdrawn earns nothing.
  const auto stood = static_cast<std::uint64_t>(
    std::llround(_region.Tuning().mvp.claim.uncontestedMinutes * SECONDS_PER_MINUTE / _region.Tuning().ref.sim.tickPeriod));
  std::int64_t eased = 0;
  for (const Grievance& grievance : _region.Grievances())
  {
    eased += grievance.cause == GrievanceCause::Claim && grievance.subject == _claim && !grievance.causeRemoved ? grievance.weightMicro : 0;
  }
  if (SettlementRenown* renown = RenownAt(_region, _claimant); renown != nullptr && _region.NextTick() >= found->madeTick + stood)
  {
    renown->easedMicro += eased;
  }
  const std::uint8_t area = found->area;
  End(_region, _claim);
  RecordEvent(_region, _claimant, StatisticKind::ClaimWithdrawn, area);
  return std::nullopt;
}

void SettleClaim(RegionSimulation& _region, StableId _claim, bool _claimantWon)
{
  const auto found = std::ranges::find(_region.Claims(), _claim, &Claim::id);
  if (found == _region.Claims().end())
  {
    return;
  }
  const Claim claim = *found;
  End(_region, _claim);
  RecordEvent(_region, claim.claimant, _claimantWon ? StatisticKind::ClaimWon : StatisticKind::ClaimLost, claim.area);
  if (!_claimantWon)
  {
    return;
  }
  if (claim.area == CLAIM_AREA_CENTRAL)
  {
    // The battle at the central resource site settles every claim on it.
    std::vector<Claim> others;
    for (const Claim& other : _region.Claims())
    {
      if (other.area == CLAIM_AREA_CENTRAL)
      {
        others.push_back(other);
      }
    }
    for (const Claim& other : others)
    {
      End(_region, other.id);
      RecordEvent(_region, other.claimant, StatisticKind::ClaimLost, other.area);
    }
    Hold(_region, CLAIM_AREA_CENTRAL, claim.claimant);
    return;
  }
  // WLD-066: the half's move is a territory event, from the settlement that held it to the claimant.
  const auto& sites = VALLEY_PAIRS[PairOfArea(claim.area)];
  const std::uint8_t from = sites[0] == claim.claimant ? sites[1] : sites[0];
  Hold(_region, claim.area, claim.claimant);
  _region.RecordContactEvent(
    {_region.NextTick(), ContactEventKind::AreaMoved, from, claim.claimant, static_cast<std::uint8_t>(PairOfArea(claim.area)), claim.area});
  _region.RecomputeTerritory(sites[0]);
  _region.RecomputeTerritory(sites[1]);
}

void AdvanceClaims(RegionSimulation& _region, std::uint64_t _tick)
{
  const auto waitTicks = static_cast<std::uint64_t>(
    std::llround(_region.Tuning().mvp.claim.uncontestedMinutes * SECONDS_PER_MINUTE / _region.Tuning().ref.sim.tickPeriod));
  std::optional<Claim> held;
  for (const Claim& claim : _region.Claims())
  {
    if (claim.state == ClaimState::Waiting && _tick >= claim.madeTick + waitTicks)
    {
      held = claim;
      break;
    }
  }
  if (held)
  {
    End(_region, held->id);
    Hold(_region, CLAIM_AREA_CENTRAL, held->claimant);
    RecordEvent(_region, held->claimant, StatisticKind::ClaimWon, CLAIM_AREA_CENTRAL);
  }
}

std::expected<std::int64_t, TributeRefusal> SendTribute(RegionSimulation& _region, std::uint8_t _from, std::uint8_t _to,
                                                        GrievanceCause _cause, std::uint64_t _subject, std::int64_t _amountMilliRu)
{
  Settlement* sender = nullptr;
  Settlement* recipient = nullptr;
  for (Settlement& settlement : _region.Settlements())
  {
    sender = settlement.site == _from ? &settlement : sender;
    recipient = settlement.site == _to ? &settlement : recipient;
  }
  if (sender == nullptr)
  {
    return std::unexpected(TributeRefusal::NoSettlement);
  }
  const auto grievance =
    std::ranges::find_if(_region.Grievances(), [&](const Grievance& _held)
                         { return _held.holder == _to && _held.against == _from && _held.cause == _cause && _held.subject == _subject; });
  if (recipient == nullptr || grievance == _region.Grievances().end())
  {
    return std::unexpected(TributeRefusal::NoGrievance);
  }
  if (_amountMilliRu <= 0)
  {
    return std::unexpected(TributeRefusal::NoAmount);
  }
  if (_amountMilliRu > sender->storesMilliRu)
  {
    return std::unexpected(TributeRefusal::CannotAfford);
  }
  // Each tribute_ru_per_weight eases a weight unit at the recipient's rate, and no more is paid than eases it whole.
  const double ruPerWeight = _region.Tuning().mvp.relation.tributeRuPerWeight / TributeRate(recipient->cladan.temperament);
  const auto whole = static_cast<std::int64_t>(
    std::ceil(static_cast<double>(grievance->weightMicro) * ruPerWeight * 1'000.0 / static_cast<double>(WEIGHT_MICRO)));
  const std::int64_t paid = std::min(_amountMilliRu, whole);
  const std::int64_t eased = ruPerWeight > 0.0
                               ? std::llround(static_cast<double>(paid) * static_cast<double>(WEIGHT_MICRO) / (ruPerWeight * 1'000.0))
                               : grievance->weightMicro;
  const std::int64_t before = grievance->weightMicro;
  grievance->weightMicro = std::max<std::int64_t>(0, grievance->weightMicro - eased);
  // MVP-061 (ADR-163): the weight the tribute eased counts towards its sender's renown.
  if (SettlementRenown* renown = RenownAt(_region, _from); renown != nullptr)
  {
    renown->easedMicro += before - grievance->weightMicro;
  }
  static_cast<void>(Withdraw(*sender, paid));
  const std::int64_t capacity = StoreCapacityMilliRu(_region.Tuning(), ActiveCount(_region, recipient->team, StructureKind::Storehouse));
  static_cast<void>(Deposit(*recipient, std::clamp<std::int64_t>(capacity - recipient->storesMilliRu, 0, paid), capacity));
  return paid;
}

void WriteClaim(Neuron::BitWriter& _writer, const Claim& _claim)
{
  _writer.WriteBits(_claim.id, 64);
  _writer.WriteBounded(_claim.claimant, LOCAL_SITE_MAX);
  _writer.WriteBounded(_claim.area, CLAIM_AREA_MAX);
  _writer.WriteBits(_claim.madeTick, 64);
  _writer.WriteBounded(static_cast<std::uint64_t>(_claim.state), CLAIM_STATE_MAX);
}

bool ReadClaim(Neuron::BitReader& _reader, Claim& _claim)
{
  _claim.id = _reader.ReadBits(64);
  _claim.claimant = static_cast<std::uint8_t>(_reader.ReadBounded(LOCAL_SITE_MAX));
  _claim.area = static_cast<std::uint8_t>(_reader.ReadBounded(CLAIM_AREA_MAX));
  _claim.madeTick = _reader.ReadBits(64);
  _claim.state = static_cast<ClaimState>(_reader.ReadBounded(CLAIM_STATE_MAX));
  // Only a claim on the central resource site waits or is contested.
  return !_reader.Failed() && _claim.id != 0 && (_claim.state == ClaimState::Open || _claim.area == CLAIM_AREA_CENTRAL);
}

void WriteAreaHolding(Neuron::BitWriter& _writer, const AreaHolding& _holding)
{
  _writer.WriteBounded(_holding.area, CLAIM_AREA_MAX);
  _writer.WriteBounded(_holding.holder, LOCAL_SITE_MAX);
}

bool ReadAreaHolding(Neuron::BitReader& _reader, AreaHolding& _holding)
{
  _holding.area = static_cast<std::uint8_t>(_reader.ReadBounded(CLAIM_AREA_MAX));
  _holding.holder = static_cast<std::uint8_t>(_reader.ReadBounded(LOCAL_SITE_MAX));
  return !_reader.Failed();
}

} // namespace Species
