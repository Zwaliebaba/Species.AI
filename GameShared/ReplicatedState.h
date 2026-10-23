#pragma once

#include "BitReader.h"
#include "BitWriter.h"
#include "EffectKind.h"
#include "FigureKind.h"
#include "OrbState.h"
#include "StableId.h"
#include "StructureKind.h"
#include "Vec3.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <variant>

namespace Species
{

// ADR-086 item 2, ADR-111: a replicated position is quantised to 1/16 wu in world coordinates, in fields wide enough
// for every region the world grid holds; a position beyond them is clamped to their range.
inline constexpr double POSITION_STEPS_PER_UNIT = 16.0;
inline constexpr std::uint32_t POSITION_HORIZONTAL_BITS = 24; // signed: ±524,288 wu
inline constexpr std::uint32_t POSITION_VERTICAL_BITS = 20;   // signed: ±32,768 wu
// A facing on the ground is a yaw in 1/1,024 of a turn: 0 faces +z, and a quarter turn, 256, faces +x.
inline constexpr std::uint32_t YAW_BITS = 10;
inline constexpr std::uint32_t YAW_STEPS = std::uint32_t{1} << YAW_BITS;
// A Worker's or a Leader's behaviour state as its replicated task.
inline constexpr std::uint8_t FIGURE_TASK_MAX = 15;
// An effect's timer, and a shockwave ring's size in 1/64 s.
inline constexpr double EFFECT_SIZE_STEPS_PER_SECOND = 64.0;
// ADR-139: a turret's barrel elevation, signed, in the yaw's steps; a structure's manned ports, one bit each; a
// construction site's progress in thousandths; and a settlement's stores in 40 bits of thousandths of a resource unit,
// and its territory's radius.
inline constexpr std::uint32_t PITCH_BITS = 10;
inline constexpr std::size_t STRUCTURE_PORTS_MAX = 16;
inline constexpr std::uint16_t PROGRESS_PERMILLE_MAX = 1'000;
inline constexpr std::uint32_t STORES_BITS = 40;
inline constexpr std::uint32_t SETTLEMENT_RADIUS_BITS = 20; // a territory's radius in 1/16 wu: up to 65,536 wu
// ADR-139: a raster's chunk is CELL_CHUNK_SIDE cells a side, one bit each, so a chunk is 64 bits.
inline constexpr std::int32_t CELL_CHUNK_SIDE = 8;
// ADR-139: the owner id of the replicated objects that are no simulated object: a settlement, and a chunk of its
// territory or its seen layer. No simulation owner takes it (ADR-096).
inline constexpr OwnerId REPLICATION_OWNER_ID = 0xFFFF;

/// A position in steps of 1/16 wu.
struct QuantizedPosition
{
  std::int32_t x = 0;
  std::int32_t y = 0;
  std::int32_t z = 0;

  friend constexpr bool operator==(const QuantizedPosition& _a, const QuantizedPosition& _b) noexcept = default;
};

[[nodiscard]] QuantizedPosition QuantizePosition(const Neuron::Vec3& _position) noexcept;
[[nodiscard]] Neuron::Vec3 PositionOf(const QuantizedPosition& _position) noexcept;
// The yaw of the direction (_x, _z) on the ground; 0 for a direction with no ground part.
[[nodiscard]] std::uint16_t QuantizeYaw(double _x, double _z) noexcept;
[[nodiscard]] double YawRadians(std::uint16_t _yaw) noexcept;

/// A Worker or a Leader as a client sees it (ADR-086 item 3): its position, and its facing, health, task, order and flags,
/// each sent on change only. Its kind never changes; its team changes when it leaves with an expedition or comes back
/// (ADR-161), and the record that carries the change is whole (ADR-165). The flags of its team's selections are its team's own: the
/// server sets them only for the client that commands the team (ADR-111).
struct ReplicatedFigure
{
  std::uint8_t team = 0;
  FigureKind kind = FigureKind::Worker;
  QuantizedPosition position;
  std::uint16_t yaw = 0;
  std::uint16_t health = 0; // R-UNIT-012; a dead figure's is its corpse fade (R-UNIT-026)
  std::uint8_t task = 0;    // its WorkerState or LeaderState
  std::uint8_t order = 0;   // a Leader's LeaderOrder (R-UNIT-164), which chooses its banner (R-UNIT-191)
  bool dead = false;
  bool onGround = false;
  bool underControl = false; // R-UNIT-136: a member of its team's crowd selection
  bool formation = false;    // R-UNIT-164: a Leader in formation mode
  bool selected = false;     // R-SIM-405: its team's selected Leader
  bool loaded = false;       // MVP-031: a Worker carrying a load (ADR-139)
  // O78, ADR-125: a Leader's flag-pointer target, the first point after the start of its Waypoint order's route
  // (R-UNIT-191); none for any other figure or order.
  std::optional<QuantizedPosition> target;

  friend bool operator==(const ReplicatedFigure& _a, const ReplicatedFigure& _b) noexcept = default;
};

/// An object of the region's effect pool (R-CRATE-400): a grenade, a shockwave ring, an order marker or a trail mark.
/// Its timer is a grenade's fuse, a ring's active tick and a marker's fade; its size a ring's, in 1/64 s.
struct ReplicatedEffect
{
  std::uint8_t team = 0;
  EffectKind kind = EffectKind::Grenade;
  QuantizedPosition position;
  std::uint16_t yaw = 0;
  std::uint16_t timer = 0;
  std::uint16_t size = 0;

  friend bool operator==(const ReplicatedEffect& _a, const ReplicatedEffect& _b) noexcept = default;
};

/// An Essence orb (R-UNIT-301): its position, and its state and that state's timer, which scale its glow (R-UNIT-331).
/// At M1 an orb is sent every tick it changes, as a figure is (ADR-111).
struct ReplicatedOrb
{
  std::uint8_t team = 0;
  QuantizedPosition position;
  OrbState state = OrbState::Birth;
  std::uint16_t timer = 0;

  friend bool operator==(const ReplicatedOrb& _a, const ReplicatedOrb& _b) noexcept = default;
};

/// A structure as a client sees it (M2 task T11, ADR-139): its kind, the settlement team it belongs to (none for a resource
/// site, WLD-084), its place and front, whether it is built and active (MVP-033 to MVP-035), its construction progress,
/// which of its ports have their Worker (R-BLDG-017), and a turret's team, health and aim (R-BLDG-408, R-BLDG-416,
/// R-BLDG-422). Its kind, owner, place and front never change.
struct ReplicatedStructure
{
  std::optional<std::uint8_t> owner;
  StructureKind kind = StructureKind::Wellspring;
  QuantizedPosition position;
  std::uint16_t yaw = 0;
  bool built = true;
  bool active = true;
  std::uint16_t progress = 0;    // a construction site's work done, in thousandths of the work it needs
  std::uint16_t mannedPorts = 0; // bit i: port i has its Worker; ports beyond STRUCTURE_PORTS_MAX are not sent
  std::optional<std::uint8_t> turretTeam;
  std::uint16_t turretHealth = 0;
  std::uint16_t aimYaw = 0;  // a turret head's front
  std::int16_t aimPitch = 0; // the barrels' up vector's tilt from vertical towards the head's front, in yaw steps

  friend bool operator==(const ReplicatedStructure& _a, const ReplicatedStructure& _b) noexcept = default;
};

/// A settlement as a client sees it (ADR-139): its local site, its team and its Cladan's colour (MVP-013), its
/// territory's radius (MVP-026), and, for the client whose steward commands it, its stores and their capacity
/// (MVP-030) and its population and limit (MVP-022); another client sees those at 0.
struct ReplicatedSettlement
{
  std::uint8_t site = 0;
  std::uint8_t team = 0;
  std::uint8_t color = 0;
  std::uint64_t storesMilliRu = 0;
  std::uint64_t capacityMilliRu = 0;
  std::uint16_t population = 0;
  std::uint16_t limit = 0;
  std::uint32_t radius = 0; // in 1/16 wu, as a position

  friend bool operator==(const ReplicatedSettlement& _a, const ReplicatedSettlement& _b) noexcept = default;
};

// The rasters of a settlement that a client receives (ADR-139); the values are on the wire.
enum class CellLayer : std::uint8_t
{
  Territory, // MVP-026, in cells of mvp.territory.cell_size
  Seen       // MVP-016, in cells of mvp.vision.cell_size, to its commander only (MVP-097)
};

/// CELL_CHUNK_SIDE by CELL_CHUNK_SIDE cells of a settlement's territory or seen layer (ADR-139), over its region's
/// square from the square's corner of least x and z: the cells from (chunkX, chunkZ) times CELL_CHUNK_SIDE, bit
/// (z × CELL_CHUNK_SIDE + x) for the cell x across and z up. A chunk with no cell held is not sent.
struct ReplicatedCells
{
  CellLayer layer = CellLayer::Territory;
  std::uint8_t site = 0;
  std::uint8_t chunkX = 0;
  std::uint8_t chunkZ = 0;
  std::uint64_t cells = 0;

  friend constexpr bool operator==(const ReplicatedCells& _a, const ReplicatedCells& _b) noexcept = default;
};

// The classes of replicated object, in the order of the variant's alternatives; the values are on the wire.
using ReplicatedState =
  std::variant<ReplicatedFigure, ReplicatedEffect, ReplicatedOrb, ReplicatedStructure, ReplicatedSettlement, ReplicatedCells>;
inline constexpr std::uint8_t REPLICATED_CLASS_MAX = 5;

// ADR-139: the stable ids of the replicated objects that are no simulated object, under REPLICATION_OWNER_ID: a
// settlement's by its local site, and a chunk's by its layer, site and place, so that each sorts after every simulated
// object and every object keeps its id for its life.
[[nodiscard]] constexpr StableId SettlementObjectId(std::uint8_t _site) noexcept
{
  return MakeStableId(REPLICATION_OWNER_ID, _site);
}
[[nodiscard]] constexpr StableId CellChunkObjectId(CellLayer _layer, std::uint8_t _site, std::uint8_t _chunkX,
                                                   std::uint8_t _chunkZ) noexcept
{
  return MakeStableId(REPLICATION_OWNER_ID, (std::uint64_t{1} << 32) | (static_cast<std::uint64_t>(_layer) << 24) |
                                              (std::uint64_t{_site} << 16) | (std::uint64_t{_chunkZ} << 8) | _chunkX);
}

/// One object of a client's interest set (ADR-086 item 1) under its stable id.
struct ReplicatedObject
{
  StableId id = 0;
  ReplicatedState state;

  friend bool operator==(const ReplicatedObject& _a, const ReplicatedObject& _b) = default;
};

// What a record says of its object; the values are on the wire.
enum class RecordKind : std::uint8_t
{
  Delta,  // the fields that changed against the update's baseline
  Whole,  // every field: an object that enters, or one with no baseline the client still holds
  Removed // the object left the interest set
};

/// One object's record in a state update (ADR-086 item 3). A delta holds the new value of each field that changed, the
/// others at their defaults, and the position as the move from the baseline's; the class rides on every record, so an
/// update decodes with no baseline and the replica applies it to one.
struct StateRecord
{
  StableId id = 0;
  RecordKind kind = RecordKind::Whole;
  ReplicatedState state;
  std::uint8_t changed = 0; // a delta's changed fields, bit i for field i of its class
  QuantizedPosition move;   // a delta's new position minus its baseline's

  friend bool operator==(const StateRecord& _a, const StateRecord& _b) = default;
};

// The fields of each class that a delta can carry, by bit.
inline constexpr std::uint8_t FIGURE_FIELD_POSITION = 1;
inline constexpr std::uint8_t FIGURE_FIELD_YAW = 2;
inline constexpr std::uint8_t FIGURE_FIELD_HEALTH = 4;
inline constexpr std::uint8_t FIGURE_FIELD_TASK = 8;
inline constexpr std::uint8_t FIGURE_FIELD_FLAGS = 16;
inline constexpr std::uint8_t FIGURE_FIELD_ORDER = 32;
inline constexpr std::uint8_t FIGURE_FIELD_TARGET = 64;
inline constexpr std::uint8_t EFFECT_FIELD_POSITION = 1;
inline constexpr std::uint8_t EFFECT_FIELD_YAW = 2;
inline constexpr std::uint8_t EFFECT_FIELD_TIMER = 4;
inline constexpr std::uint8_t ORB_FIELD_POSITION = 1;
inline constexpr std::uint8_t ORB_FIELD_STATE = 2;
inline constexpr std::uint8_t ORB_FIELD_TIMER = 4;
inline constexpr std::uint8_t STRUCTURE_FIELD_FLAGS = 1; // built and active
inline constexpr std::uint8_t STRUCTURE_FIELD_PROGRESS = 2;
inline constexpr std::uint8_t STRUCTURE_FIELD_PORTS = 4;
inline constexpr std::uint8_t STRUCTURE_FIELD_TURRET = 8; // its team and health
inline constexpr std::uint8_t STRUCTURE_FIELD_AIM = 16;
inline constexpr std::uint8_t SETTLEMENT_FIELD_STORES = 1;     // the stores and their capacity
inline constexpr std::uint8_t SETTLEMENT_FIELD_POPULATION = 2; // the population and its limit
inline constexpr std::uint8_t SETTLEMENT_FIELD_RADIUS = 4;
inline constexpr std::uint8_t CELLS_FIELD_CELLS = 1;

// The record that brings a client holding _baseline, or nothing, to _state: a whole record without a baseline, a delta
// of the fields that changed, or none when nothing changed. A baseline of another class, or of a figure of another
// team, gives a whole record.
[[nodiscard]] std::optional<StateRecord> MakeRecord(StableId _id, const ReplicatedState& _state, const ReplicatedState* _baseline);
// A removal of _id.
[[nodiscard]] StateRecord RemovalRecord(StableId _id) noexcept;
// The state a whole or delta record gives: a delta needs its baseline, of its own class; none without one.
[[nodiscard]] std::optional<ReplicatedState> ApplyRecord(const StateRecord& _record, const ReplicatedState* _baseline);

// A record on the wire: its id, the first of an update whole and each later one as the gap from the one before, then
// its kind, its class and its fields. The bits it takes are counted exactly, so the sender fills a datagram to its
// limit and no further.
[[nodiscard]] std::size_t RecordBits(const StateRecord& _record, std::optional<StableId> _previous);
void WriteRecord(Neuron::BitWriter& _writer, const StateRecord& _record, std::optional<StableId> _previous);
// False when the record does not read whole, or names an id that does not follow _previous.
[[nodiscard]] bool ReadRecord(Neuron::BitReader& _reader, StateRecord& _record, std::optional<StableId> _previous);

} // namespace Species
