#pragma once

#include "BitReader.h"
#include "BitWriter.h"
#include "EffectKind.h"
#include "FigureKind.h"
#include "OrbState.h"
#include "StableId.h"
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
inline constexpr std::uint8_t FIGURE_TASK_MAX = 7;
// An effect's timer, and a shockwave ring's size in 1/64 s.
inline constexpr double EFFECT_SIZE_STEPS_PER_SECOND = 64.0;

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
/// each sent on change only. Its team and kind never change. The flags of its team's selections are its team's own: the
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

// The classes of replicated object, in the order of the variant's alternatives; the values are on the wire.
using ReplicatedState = std::variant<ReplicatedFigure, ReplicatedEffect, ReplicatedOrb>;
inline constexpr std::uint8_t REPLICATED_CLASS_MAX = 2;

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

// The record that brings a client holding _baseline, or nothing, to _state: a whole record without a baseline, a delta
// of the fields that changed, or none when nothing changed. A baseline of another class gives a whole record.
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
