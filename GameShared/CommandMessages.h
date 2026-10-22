#pragma once

#include "GameMessages.h"
#include "StableId.h"

#include <cstddef>
#include <cstdint>

namespace Species
{

// The messages of a region's battle commands and their outcomes (M1 task T7, ADR-106), written by hand in the form the
// generated codecs will take (ADR-062, ADR-093). A command names its team and its figures by stable id, never by pool
// slot (R-SIM-449, ADR-087 item 2). A field out of its bounds fails the decoding, so the queue drops the command as
// malformed: a real that is not finite, an id of 0, a negative radius or an operation it does not know.

/// A point on the ground as a command names it: its x and z in world units. The server takes the height from its own
/// terrain, so a command carries no height.
struct GroundPoint
{
  double x = 0.0;
  double z = 0.0;
};

// A ground point as two binary64 values; one that is not finite fails the reader.
void WriteGroundPoint(Neuron::BitWriter& _writer, const GroundPoint& _point);
[[nodiscard]] GroundPoint ReadGroundPoint(Neuron::BitReader& _reader) noexcept;

// What became of one command (ADR-087 item 4): it executed, with an effect or none, or the queue refused it on arrival
// for one of the six reasons of ADR-095. The values travel to the client, so they are never renumbered.
enum class CommandResult : std::uint8_t
{
  Applied,
  NoEffect,
  UnknownType,
  WrongVersion,
  Oversized,
  Malformed,
  NotPermitted,
  Duplicate
};

/// The outcome of one command, sent to its sender on the reliable channel after the tick that executed or refused it
/// (ADR-095): the command's sequence number, as the server counted it and the client counts the same, and its tick.
/// From version 2, the reason a command that executed had no effect, by its type's own values: a placement's
/// PlacementRefusal, or 0 for none (ADR-136).
struct CommandReport
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::CommandReport);
  static constexpr std::uint8_t VERSION = 2;
  static constexpr std::size_t MAX_BYTES = 21; // the 3-byte header, two 64-bit values, a 3-bit result and an 8-bit reason

  std::uint64_t sequence = 0;
  std::uint64_t tick = 0;
  CommandResult result = CommandResult::Applied;
  std::uint8_t reason = 0;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

/// R-SIM-450 `Select`: selects a Leader of the team, or nothing (R-SIM-405).
struct SelectCommand
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::Select);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 13; // the header, the team, a presence bit and a 64-bit id

  std::uint8_t team = 0;
  StableId actor = 0; // 0: select nothing, sent as an absent value

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

/// R-SIM-450 `PromoteWorker`: a Worker of the team becomes a Leader, which is selected (R-UNIT-153).
struct PromoteWorkerCommand
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::PromoteWorker);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 12; // the header, the team and a 64-bit id

  std::uint8_t team = 0;
  StableId actor = 0;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

/// R-SIM-450 `DemoteLeader`: a Leader of the team becomes a Worker, and the selection is cleared (R-UNIT-154).
struct DemoteLeaderCommand
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::DemoteLeader);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 12;

  std::uint8_t team = 0;
  StableId actor = 0;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

/// R-SIM-450 `OrderUnit`: the team's selected Leader takes the point as its waypoint when the order is move-only, and
/// otherwise receives orders to it, with a straight route when the order says so (R-UNIT-175).
struct OrderUnitCommand
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::OrderUnit);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 21; // the header, the team, two flags and two binary64 values

  std::uint8_t team = 0;
  bool straight = false;
  bool moveOnly = false;
  GroundPoint position;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

/// R-SIM-450 `ToggleUnit` for a Leader: it cancels its orders, or enters formation, and is selected (R-UNIT-175).
struct ToggleUnitCommand
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::ToggleUnit);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 12;

  std::uint8_t team = 0;
  StableId actor = 0;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

/// R-SIM-450 `SelectWorkers`: the team's Workers within the circle come under control (R-UNIT-136).
struct SelectWorkersCommand
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::SelectWorkers);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 28; // the header, the team and three binary64 values

  std::uint8_t team = 0;
  GroundPoint center;
  double radius = 0.0; // wu, finite and not negative

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

/// R-SIM-450 `OrderWorkers`: the crowd order of the team's controlled Workers (R-UNIT-140, R-SIM-455).
struct OrderWorkersCommand
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::OrderWorkers);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 21;

  std::uint8_t team = 0;
  bool straight = false;
  GroundPoint position;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

/// R-SIM-450 `ControlState` with the one field of the team command state that the MVP reads: the pointer position,
/// which a selected Leader faces (R-SIM-439, R-SIM-442, R-UNIT-168). Each one replaces the last, so the latest of a
/// tick holds (R-SIM-443).
struct ControlStateCommand
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::ControlState);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 20;

  std::uint8_t team = 0;
  GroundPoint pointer;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

/// The AI's waypoint order to one Worker of its team, with no route (R-UNIT-111, R-BOT-015, R-BOT-086).
struct AiOrderWorkerCommand
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::AiOrderWorker);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 28;

  std::uint8_t team = 0;
  StableId worker = 0;
  GroundPoint position;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

// The Leader operations an AI applies (R-UNIT-169, R-UNIT-171, R-UNIT-172, R-BOT-015). The values travel, so they are
// never renumbered.
enum class LeaderOperation : std::uint8_t
{
  AssignOrders,
  AssignWaypoint,
  FormUp
};

/// The AI's order to one Leader of its team: orders, a waypoint or a formation at the point (R-UNIT-188).
struct AiOrderLeaderCommand
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::AiOrderLeader);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 29;

  std::uint8_t team = 0;
  StableId leader = 0;
  LeaderOperation operation = LeaderOperation::AssignOrders;
  bool straight = false; // read by AssignOrders only
  GroundPoint position;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

/// The AI's fields of one Leader of its team (table 2.3-A): its no-formations flag and, when given, the bot target
/// front it turns towards while it is not walking (R-UNIT-163 step 8), which is never cleared once set.
struct AiLeaderFieldsCommand
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::AiLeaderFields);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 29;

  std::uint8_t team = 0;
  StableId leader = 0;
  bool noFormations = false;
  bool hasFront = false;
  GroundPoint front; // a horizontal direction, read when hasFront is set

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

/// The development command that restarts the M1 sandbox from its seed or a new one (M1 plan §6, ADR-109). It replaces
/// the world between ticks rather than entering a queue, and a server that runs no sandbox, or a Release build, ignores
/// it.
struct SandboxRestartCommand
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::SandboxRestart);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 4; // the 3-byte header and a flag

  bool newSeed = false;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

} // namespace Species
