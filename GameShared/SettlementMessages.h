#pragma once

#include "CommandMessages.h"
#include "GameMessages.h"
#include "ReplicatedState.h"
#include "StableId.h"
#include "StructureKind.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace Species
{

// The settlement's commands and its status (M2 task T8, ADR-136), written by hand in the form the generated codecs will
// take (ADR-062, ADR-093). A command names its settlement by its team, which its sender must command (ADR-087 item 2),
// and a field out of its bounds fails the decoding, so the queue drops the command as malformed.

/// MVP-032: the settlement places a structure of the kind centred at the point, facing away from its Wellspring. A
/// placement refused when it executes has no effect, and its outcome's reason is the PlacementRefusal.
struct PlaceStructureCommand
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::PlaceStructure);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 21; // the 3-byte header, the team, a 3-bit kind and two binary64 values

  std::uint8_t team = 0;
  StructureKind kind = StructureKind::Dwelling;
  GroundPoint position;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

/// MVP-034: the settlement rebuilds its destroyed turret, named by its stable id, for mvp.cost.turret_rebuild_fraction
/// of its cost. A rebuild refused when it executes has no effect, and its outcome's reason is the PlacementRefusal.
struct RebuildTurretCommand
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::RebuildTurret);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 12; // the header, the team and a 64-bit id

  std::uint8_t team = 0;
  StableId turret = 0;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

/// MVP-036: the settlement's three shares in percent, of gathering, construction and guard duty in WorkerTask's order
/// from Gather. Shares that do not sum to 100 are malformed.
struct SetPrioritiesCommand
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::SetPriorities);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 7; // the header, the team and three 7-bit shares

  std::uint8_t team = 0;
  std::array<std::uint8_t, 3> shares{};

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

/// O106 (ADR-178): the settlement holds its growth, or lets it run again. While held, its Wellspring grows no Worker and
/// its growth timer waits where it stands, so the stores fill for building.
struct HoldGrowthCommand
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::HoldGrowth);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 5; // the header, the team and a flag

  std::uint8_t team = 0;
  bool held = false;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

/// MVP-098, PIL-075: a request for the status of the settlement of the team, which its sender must command. The server
/// answers it outside the queue: its outcome names the tick after which the status is taken, and a permitted request
/// is followed by the status of that tick.
struct ReadSettlementStatusCommand
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::ReadSettlementStatus);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 4; // the header and the team

  std::uint8_t team = 0;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

// MVP-024: whether the settlement's Wellspring grows, or why it does not (MVP-102's cause when growth stops); a wire
// value that is never renumbered.
enum class GrowthState : std::uint8_t
{
  Growing,     // its timer counts down to the next orb
  AtLimit,     // the population has reached its limit, which a dwelling raises
  StoresShort, // the stores hold less than an orb's cost
  Held         // O106 (ADR-178): its steward holds it
};

/// A construction site as the status lists it: its stable id, its kind and its progress in thousandths.
struct QueuedSite
{
  StableId id = 0;
  StructureKind kind = StructureKind::Dwelling;
  std::uint16_t progressPermille = 0;
};

// The construction sites a status lists, far above mvp.construction.queue_max.
inline constexpr std::uint64_t STATUS_QUEUE_MAX = 255;

/// MVP-098: a settlement's status as the server reads it after a tick, for the HUD and a check-in (MVP-102, PIL-075):
/// its stores and their capacity, its population and limit, its growth, its priorities, how many of the Workers that
/// follow no Leader hold each task, its idle Workers by reason (MVP-037), its inactive structures and its upkeep an hour
/// (MVP-035), its strength (MVP-038), and its construction sites in the order they are built (MVP-032).
struct SettlementStatus
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::SettlementStatus);
  static constexpr std::uint8_t VERSION = 2;            // 2: GrowthState::Held (ADR-178), in the same two bits
  static constexpr std::size_t FIXED_BITS = 711;        // every field but the sites, with the sites' count
  static constexpr std::size_t SITE_BITS = 64 + 3 + 10; // an id, a kind and a progress up to 1,000
  static constexpr std::size_t MAX_BYTES = 3 + (FIXED_BITS + STATUS_QUEUE_MAX * SITE_BITS + 7) / 8;

  std::uint64_t tick = 0;
  std::uint8_t team = 0;
  std::int64_t storesMilliRu = 0;
  std::int64_t capacityMilliRu = 0;
  std::uint32_t population = 0;
  std::uint32_t populationLimit = 0;
  GrowthState growth = GrowthState::Growing;
  std::uint32_t growthTicks = 0; // to the next orb while it grows
  std::array<std::uint8_t, 3> priorities{};
  std::array<std::uint32_t, 3> tasks{}; // in WorkerTask's order from Gather
  std::array<std::uint32_t, 4> idle{};  // in IdleReason's order from NoFreePort
  std::uint32_t inactiveStructures = 0;
  std::int64_t upkeepMilliRuPerHour = 0;
  std::int64_t strengthMilliRu = 0;
  std::vector<QueuedSite> queue;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

// MVP-102: what a notice tells its steward; a wire value that is never renumbered. M2 has structures completed; M3 adds
// contact, claims, the warning, battles, the respite and the run (ADR-165).
enum class NoticeKind : std::uint8_t
{
  StructureCompleted = 0, // MVP-033: a construction site of the steward's settlement is built; the subject is the structure
  Contact = 1,            // MVP-027: the settlement came into contact with its valley neighbour; the detail is their pair
  ClaimMade = 2,          // MVP-053: a claim by or on the settlement; the subject is the claim, the detail its area
  ClaimWithdrawn = 3,     // the claim's claimant withdrew it
  ClaimLost = 4,          // the claim's claimant lost its battle
  ClaimWon = 5,           // the claim's claimant won its battle, or its wait held the central resource site
  AttackDeclared = 6,     // MVP-042: the warning of an attack by or on the settlement; the subject is the expedition, the
                          // detail 1 for a claim battle
  AttackEnded = 7,        // the expedition ended before its battle: withdrawn, or unfought at the run's end (MVP-060)
  BattleStarted = 8,      // MVP-043: the expedition's battle started; the position is its objective
  BattleEnded = 9,        // MVP-045: its battle ended; the detail is 1 when the attacker won, and a BattleResultNotice follows
  RespiteStarted = 10,    // MVP-047: the settlement's respite started; the subject is the expedition of the battle it lost
  RunQuiet = 11,          // MVP-060: the run's last mvp.run.quiet_minutes began
  RunEnded = 12           // MVP-063: the run ended, and the world is read-only
};

inline constexpr std::uint64_t NOTICE_KIND_MAX = static_cast<std::uint64_t>(NoticeKind::RunEnded);

/// ADR-084 item 5, MVP-102 (ADR-139, ADR-165): a guaranteed event for the HUD's notifications, on the reliable channel
/// beside the command outcomes, to the steward who commands the team, in the order the region raised them: its tick, its
/// kind, the team, its subject by stable id (0 for a kind that names none: contact and the run), the subject's structure
/// kind and position, the other settlement concerned by its local site, whether the steward's settlement acted (the
/// claimant or the attacker; true for contact) and the kind's detail.
struct SettlementNotice
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::SettlementNotice);
  static constexpr std::uint8_t VERSION = 2;
  static constexpr std::size_t MAX_BYTES = 31; // the header, the tick, the kind, the team, the subject, the structure's kind and
                                               // a whole position, the other site, the flag and the detail

  std::uint64_t tick = 0;
  NoticeKind kind = NoticeKind::StructureCompleted;
  std::uint8_t team = 0;
  StableId subject = 0;
  StructureKind structure = StructureKind::Dwelling;
  QuantizedPosition position;
  std::optional<std::uint8_t> other; // a local site, 0 to 3
  bool ours = false;
  std::uint8_t detail = 0;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

} // namespace Species
