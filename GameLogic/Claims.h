#pragma once

#include "BitReader.h"
#include "BitWriter.h"
#include "RelationMessages.h"
#include "StableId.h"

#include <cstdint>
#include <expected>
#include <optional>

namespace Species
{

class RegionSimulation;
struct Structure;

// The first state version whose regions hold claims, moved areas and territory events of more than one kind (M3 task T5,
// ADR-160).
inline constexpr std::uint32_t STATE_VERSION_WITH_CLAIMS = 12;
// The first state version whose regions hold the tick each area's last claim ended (O102, DEC-1008).
inline constexpr std::uint32_t STATE_VERSION_WITH_CLAIM_COOLDOWN = 16;

/// MVP-053 (ADR-160): a claim of the settlement at site `claimant` on an area of the region (CLAIM_AREA_CENTRAL or a
/// half of a valley border area), with the tick it was made. Its stable id is the subject of the grievance it gives the
/// area's holder (MVP-051). A region holds its claims in the order they were made.
struct Claim
{
  StableId id = 0;
  std::uint8_t claimant = 0;
  std::uint8_t area = 0;
  std::uint64_t madeTick = 0;
  ClaimState state = ClaimState::Open;

  bool operator==(const Claim&) const = default;
};

/// MVP-053 (ADR-160): an area a claim has moved: a half of a valley border area that its own side's settlement no
/// longer holds, or the central resource site; and the site of the settlement that holds it. A region holds them in
/// the order of their areas, one for each.
struct AreaHolding
{
  std::uint8_t area = 0;
  std::uint8_t holder = 0;

  bool operator==(const AreaHolding&) const = default;
};

/// O102 (DEC-1008): the tick an area's last claim ended, by any end, from which no claim may be made on the area for
/// mvp.claim.cooldown_minutes. A region holds them in the order of their areas, one for each area a claim has ended on.
struct ClaimEnd
{
  std::uint8_t area = 0;
  std::uint64_t tick = 0;

  bool operator==(const ClaimEnd&) const = default;
};

// The valley border pair of a half, its index in VALLEY_PAIRS, and the site whose half it is.
[[nodiscard]] constexpr std::size_t PairOfArea(std::uint8_t _area) noexcept
{
  return _area / 2U;
}
[[nodiscard]] std::uint8_t OwnSiteOfArea(std::uint8_t _area) noexcept;

// MVP-053 and WLD-063: the settlement that holds the area: the one a claim moved it to, else for a half its own side's
// settlement once its territory has entered it; none for the central resource site that no claim has taken.
[[nodiscard]] std::optional<std::uint8_t> HolderOf(const RegionSimulation& _region, std::uint8_t _area);
// The settlement a claim moved the area to, or none.
[[nodiscard]] std::optional<std::uint8_t> MovedTo(const RegionSimulation& _region, std::uint8_t _area) noexcept;

// MVP-053: the area's resource site, the one in the half or the central resource site, or none.
[[nodiscard]] const Structure* ResourceSiteOfArea(const RegionSimulation& _region, std::uint8_t _area);

// O102 (DEC-1008): the first tick a claim may be made on the area again, mvp.claim.cooldown_minutes after its last claim
// ended; none when no claim on it has ended.
[[nodiscard]] std::optional<std::uint64_t> CooldownEndOf(const RegionSimulation& _region, std::uint8_t _area);
// MVP-053, WLD-086: why the settlement at site _claimant may not claim the area now, or none.
[[nodiscard]] std::optional<ClaimRefusal> CheckClaim(const RegionSimulation& _region, std::uint8_t _claimant, std::uint8_t _area);
// MVP-053 (ADR-160): the claim, which CheckClaim permits. A claim on a held area is Open and gives its holder's Cladan a
// grievance against the claimant (MVP-051); one on the central resource site that nobody holds is Waiting, or, with
// every other claim on it, Contested.
[[nodiscard]] std::expected<StableId, ClaimRefusal> MakeClaim(RegionSimulation& _region, std::uint8_t _claimant, std::uint8_t _area);
// MVP-053 and PIL-122: the claimant withdraws its claim, which removes its grievance's cause; a contested claim left
// alone on the central resource site waits again, and is held once mvp.claim.uncontested_minutes have passed since it
// was made.
[[nodiscard]] std::optional<ClaimRefusal> WithdrawClaim(RegionSimulation& _region, std::uint8_t _claimant, StableId _claim);
// MVP-053 and PIL-121 (ADR-160): a claim battle settles the claim (M3 task T6). A claimant that won holds the area, a
// half's move recorded as a territory event (WLD-066) and the territories made again; a claim on the central resource
// site that won ends every other claim on it. The claim ends either way, and its grievance's cause with it; a contested
// claim that lost leaves a lone rival waiting again.
void SettleClaim(RegionSimulation& _region, StableId _claim, bool _claimantWon);
// MVP-053 (ADR-160): the claims' step: a Waiting claim held mvp.claim.uncontested_minutes after it was made holds the
// central resource site, and ends.
void AdvanceClaims(RegionSimulation& _region, std::uint64_t _tick);

// MVP-054 (ADR-160): the settlement at site _from sends tribute from its stores to the settlement at site _to, whose
// Cladan holds the grievance against it of the cause and the subject. Each mvp.relation.tribute_ru_per_weight eases it
// by one weight unit, at the rate of the recipient's temperament (MVP-057), and no more is paid than eases it whole;
// the recipient keeps what its stores have room for. Returns what was paid.
[[nodiscard]] std::expected<std::int64_t, TributeRefusal> SendTribute(RegionSimulation& _region, std::uint8_t _from, std::uint8_t _to,
                                                                      GrievanceCause _cause, std::uint64_t _subject,
                                                                      std::int64_t _amountMilliRu);

// The claim and the holding canonically (ADR-081 item 7), and the values the writer wrote; false when they do not read
// whole.
void WriteClaim(Neuron::BitWriter& _writer, const Claim& _claim);
[[nodiscard]] bool ReadClaim(Neuron::BitReader& _reader, Claim& _claim);
void WriteAreaHolding(Neuron::BitWriter& _writer, const AreaHolding& _holding);
[[nodiscard]] bool ReadAreaHolding(Neuron::BitReader& _reader, AreaHolding& _holding);
void WriteClaimEnd(Neuron::BitWriter& _writer, const ClaimEnd& _end);
[[nodiscard]] bool ReadClaimEnd(Neuron::BitReader& _reader, ClaimEnd& _end);

} // namespace Species
