#pragma once

#include "BitReader.h"
#include "BitWriter.h"
#include "Navigation.h"
#include "TerrainField.h"
#include "TuningMvp.h"
#include "Vec3.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace Species
{

class RegionSimulation;
class Sight;

/// The bot navigation graph of a region (R-BOT-025 to R-BOT-037, ADR-107): its nodes are the navigation nodes of
/// MVP-017 in their order, which stands for the reference's structure order (O70), and its links are built once from the
/// terrain and culled. It is derived from the terrain like the land route graph, so neither the hash nor a snapshot holds
/// it. The MVP has no manual link, no run-time node, no wall, no Barrier Emitter and no zone, so a link records no
/// emitter and the graph never changes.
class BotGraph
{
public:
  // R-BOT-030 and R-BOT-034: every node's links in structure order, then the cull, nodes in structure order.
  [[nodiscard]] static BotGraph Build(const std::vector<NavigationNode>& _nodes, const Neuron::TerrainField& _field,
                                      const TuningMvp& _tuning);

  [[nodiscard]] std::size_t Size() const noexcept
  {
    return m_positions.size();
  }
  [[nodiscard]] const Neuron::Vec3& Position(std::uint32_t _node) const noexcept
  {
    return m_positions[_node];
  }
  // R-BOT-025: a node at or below sea level, which is never a walking destination.
  [[nodiscard]] bool Sea(std::uint32_t _node) const noexcept
  {
    return m_sea[_node] != 0;
  }
  // The nodes a node links to, in link order.
  [[nodiscard]] const std::vector<std::uint32_t>& Links(std::uint32_t _node) const noexcept
  {
    return m_links[_node];
  }
  // R-BOT-052: the node nearest _point, by a scan in structure order, of those walkable from it; none when none is.
  [[nodiscard]] std::optional<std::uint32_t> NearestNode(const Neuron::Vec3& _point, bool _evaluateCliffs, bool _permitSteepDownhill,
                                                         const Neuron::TerrainField& _field, const TuningMvp& _tuning) const;

private:
  std::vector<Neuron::Vec3> m_positions;
  std::vector<std::uint8_t> m_checkCliffs;
  std::vector<std::uint8_t> m_sea;
  std::vector<std::vector<std::uint32_t>> m_links;
};

/// The counts and owner of one node at its last recount, and its recount countdown (table 2.1-A): simulation state,
/// which the bot rules and every Worker's census shortcut read (R-BOT-042, R-BOT-045). The counts hold one entry per team
/// slot the region had at the recount; a later team counts 0. A team's enemies are those of the region's enemy rule
/// (ADR-157), so settlements at peace do not count each other (ADR-158).
struct BotNode
{
  std::int32_t recountTicks = 0;
  std::int32_t owner = -1; // R-BOT-047: the team slot, −1 for no team
  std::vector<std::uint32_t> friends;
  std::vector<std::uint32_t> enemies;
  std::vector<std::uint32_t> idle;

  [[nodiscard]] std::uint32_t FriendsOf(std::size_t _team) const noexcept
  {
    return _team < friends.size() ? friends[_team] : 0;
  }
  [[nodiscard]] std::uint32_t EnemiesOf(std::size_t _team) const noexcept
  {
    return _team < enemies.size() ? enemies[_team] : 0;
  }
  [[nodiscard]] std::uint32_t IdleOf(std::size_t _team) const noexcept
  {
    return _team < idle.size() ? idle[_team] : 0;
  }
};

// R-BOT-028: a node's first countdown, max(1, ⌈NextUnit(ref.bot.node.recount_phase)⌉), drawn once when it is made.
[[nodiscard]] BotNode MakeBotNode(RegionSimulation& _region, std::uint32_t _index);

// R-BOT-041 to R-BOT-047: one advance of node _index: the countdown, and at 0 the recount of every actor in range, each
// Worker's nearest node, and the owner. The MVP has no priority to recalculate.
void AdvanceBotNode(RegionSimulation& _region, std::uint32_t _index, BotNode& _node);

// R-BOT-049: node _index is important when the scan of the structures in slot order finds one with at least one port
// strictly within ref.bot.node.important_radius of it; the MVP has no other kind that makes a node important.
[[nodiscard]] bool Important(const RegionSimulation& _region, std::uint32_t _index);

// R-BOT-050: the live Leaders of _team within _range of _point: every one, or only those with a waypoint order or
// without one.
enum class LeaderCountKind : std::uint8_t
{
  Every,
  Waypoint,
  NotWaypoint
};
[[nodiscard]] std::uint32_t LeaderCount(const RegionSimulation& _region, std::uint8_t _team, const Neuron::Vec3& _point, double _range,
                                        LeaderCountKind _kind);

// MVP-058 (ADR-158): node _index as an AI with _sight reads it: its counts and owner where the sight sees every place
// its census counts, ref.bot.node.range about it, and a node no team owns and none is counted at elsewhere.
[[nodiscard]] const BotNode& SeenBotNode(const RegionSimulation& _region, const Sight& _sight, std::uint32_t _index);
// R-BOT-047 with the region's enemies (ADR-158): _node is owned by a team that is _team's enemy.
[[nodiscard]] bool HeldByEnemy(const RegionSimulation& _region, const BotNode& _node, std::uint8_t _team);

// R-BOT-051: node _index needs defending by _team, as an AI with _sight reads the nodes. The MVP has no tagged zone, so
// only item 2 can hold.
[[nodiscard]] bool NeedsDefending(const RegionSimulation& _region, const Sight& _sight, std::uint32_t _index, std::uint8_t _team);

// The node canonically (ADR-081 item 7), and the values the writer wrote; false when they do not read whole.
void WriteBotNode(Neuron::BitWriter& _writer, const BotNode& _node);
[[nodiscard]] bool ReadBotNode(Neuron::BitReader& _reader, BotNode& _node);

} // namespace Species
