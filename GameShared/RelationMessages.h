#pragma once

#include "GameMessages.h"
#include "StableId.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace Species
{

// The relations' commands (M3 task T5, ADR-160), written by hand in the form the generated codecs will take (ADR-062,
// ADR-093), as the settlement's are (ADR-136). A command names its settlement by its team, which its sender must
// command (ADR-087 item 2), and a field out of its bounds fails the decoding, so the queue drops the command as
// malformed.

// MVP-053 and WLD-086: the areas of a region that a claim names. Each half of a valley border area is its pair's index
// in VALLEY_PAIRS times two plus its side, the half nearer that side's site; the valley's central resource site follows.
inline constexpr std::uint8_t CLAIM_AREA_CENTRAL = 8;
inline constexpr std::uint64_t CLAIM_AREA_MAX = CLAIM_AREA_CENTRAL;

// MVP-053: why a claim or a withdrawal is refused, checked in this order; the reason of its command's outcome, a wire
// value that is never renumbered.
enum class ClaimRefusal : std::uint8_t
{
  NoSettlement = 1,   // the team is no settlement's
  NotClaimable = 2,   // a half of a valley border area that is not its valley neighbour's pair, or the claimant's own
  NotHeld = 3,        // a half that its neighbour does not hold
  AlreadyHeld = 4,    // an area the claimant holds
  AlreadyClaimed = 5, // an area the claimant already claims
  NoSuchClaim = 6     // a withdrawal that names no claim of the claimant's
};

inline constexpr std::uint8_t CLAIM_REFUSAL_MAX = static_cast<std::uint8_t>(ClaimRefusal::NoSuchClaim);

// MVP-054: why a tribute is refused, checked in this order; the reason of its command's outcome, a wire value that is
// never renumbered.
enum class TributeRefusal : std::uint8_t
{
  NoSettlement = 1, // the team is no settlement's
  NoGrievance = 2,  // the named settlement's Cladan holds no such grievance against the sender
  NoAmount = 3,     // an amount of nothing
  CannotAfford = 4  // the stores do not hold the amount
};

inline constexpr std::uint8_t TRIBUTE_REFUSAL_MAX = static_cast<std::uint8_t>(TributeRefusal::CannotAfford);

// MVP-041 and MVP-042 (ADR-161): why a declared attack or its withdrawal is refused, checked in this order; the reason
// of its command's outcome, a wire value that is never renumbered.
enum class AttackRefusal : std::uint8_t
{
  NoSettlement = 1, // the team is no settlement's
  Underway = 2,     // the settlement already has an expedition out; for a withdrawal, it has none
  NoTarget = 3,     // a target that is no settlement or the attacker's own, or a claim that is not the attacker's open one
  UnderRespite = 4, // the target is under a respite (MVP-047)
  NoLeader = 5,     // no live Leader of the settlement among those named
  TooLarge = 6,     // the Leaders' Workers are more than mvp.expedition.size_max
  CannotAfford = 7, // the stores do not hold the attack's cost
  Quiet = 8         // the run is in its last mvp.run.quiet_minutes (MVP-060, ADR-163)
};

inline constexpr std::uint8_t ATTACK_REFUSAL_MAX = static_cast<std::uint8_t>(AttackRefusal::Quiet);
// O98: an expedition's Leaders, at most.
inline constexpr std::size_t EXPEDITION_LEADERS_MAX = 3;

/// MVP-053: the settlement claims an area: a half of a valley border area that its valley neighbour holds, or the
/// valley's central resource site. A claim refused when it executes has no effect, and its outcome's reason is the
/// ClaimRefusal.
struct MakeClaimCommand
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::MakeClaim);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 5; // the 3-byte header, the team and a 4-bit area

  std::uint8_t team = 0;
  std::uint8_t area = 0;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

/// MVP-053 and PIL-122: the settlement withdraws its claim, named by its stable id, which settles it with no battle.
struct WithdrawClaimCommand
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::WithdrawClaim);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 12; // the header, the team and a 64-bit id

  std::uint8_t team = 0;
  StableId claim = 0;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

/// MVP-041 and MVP-042 (O98, ADR-161): the settlement declares an attack with up to three of its Leaders and the Workers
/// following them, as one expedition: a raid on the settlement at site `target`, or, when `claim` names one of its
/// claims, the claim battle that settles it. A Leader id of 0 names none. A declaration refused when it executes has no
/// effect, and its outcome's reason is the AttackRefusal.
struct DeclareAttackCommand
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::DeclareAttack);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 37; // the header, the team, three 64-bit Leaders, a 2-bit site and a 64-bit claim

  std::uint8_t team = 0;
  std::array<StableId, EXPEDITION_LEADERS_MAX> leaders{};
  std::uint8_t target = 0;
  StableId claim = 0;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

/// MVP-045 and PIL-087 (ADR-161): the settlement withdraws its expedition: before its battle the attack ends with no
/// battle; in its battle, the battle ends as a rout.
struct WithdrawAttackCommand
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::WithdrawAttack);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 4; // the header and the team

  std::uint8_t team = 0;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

/// MVP-054: the settlement sends tribute from its stores to the settlement at site `to`, whose Cladan holds the
/// grievance against it that the cause and the subject name, to ease that grievance. A tribute refused when it executes
/// has no effect, and its outcome's reason is the TributeRefusal.
struct SendTributeCommand
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::SendTribute);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 21; // the header, the team, a 2-bit site, a 2-bit cause and two 64-bit fields

  std::uint8_t team = 0;
  std::uint8_t to = 0;
  std::uint8_t cause = 0; // GrievanceCause's value
  std::uint64_t subject = 0;
  std::int64_t amountMilliRu = 0;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

} // namespace Species
