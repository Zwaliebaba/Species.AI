#pragma once

#include "ActorGrid.h"
#include "ActorGridCache.h"
#include "AiController.h"
#include "BitReader.h"
#include "BitWriter.h"
#include "BotNavigation.h"
#include "CommandQueue.h"
#include "EffectEvent.h"
#include "EssenceOrb.h"
#include "Figure.h"
#include "Movement.h"
#include "ObstacleGrid.h"
#include "RegionTerrain.h"
#include "RouteStore.h"
#include "SlicedPool.h"
#include "StableId.h"
#include "StableIdCounter.h"
#include "TickDraws.h"
#include "TuningMvp.h"
#include "Vec3.h"
#include "Weapons.h"

#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace Species
{

/// The owner of one region (ADR-083 item 4, ADR-085 item 1): every mutable value inside the region's square. It steps
/// as one region job and reads no other owner's state (ADR-085 item 2). It holds its teams with their figures, its
/// command queue, its id counter, its tick, and the actor grid, its cache and the obstacle grid over its square
/// (ADR-103); the settlements, structures and battles of M2 and M3 move in here (MVP-096).
class RegionSimulation
{
public:
  /// One team slot of the region (R-SIM-221): its ungrouped figures, the only actors of the MVP (R-UNIT-022), its kill
  /// counter (R-UNIT-032), its selection, a Leader by stable id and slot that resolves only while the Leader holds the
  /// slot (R-SIM-401), and the pointer position of its latest control state (R-SIM-442).
  struct Team
  {
    SlicedPool<Figure> figures;
    std::uint32_t kills = 0;
    StableId selectedId = 0; // 0: nothing selected
    std::uint32_t selectedSlot = 0;
    Neuron::Vec3 pointer;
  };

  /// Who commands a team (ADR-087 item 2, ADR-106): the sender whose commands for the team are permitted, and whether
  /// that sender is an AI, which alone may send the AI's own command types (DEC-122).
  struct Commander
  {
    Neuron::SenderId sender = 0; // 0: nobody
    bool ai = false;
  };

  // A region's team slots are numbered by a byte (R-SIM-221).
  static constexpr std::size_t TEAM_SLOTS = std::size_t{std::numeric_limits<std::uint8_t>::max()} + 1;
  // A region's state as a snapshot of WORLD_STATE_VERSION 1 holds it, before its teams existed (ADR-099).
  static constexpr std::uint32_t STATE_VERSION_WITHOUT_TEAMS = 1;

  // _terrain may follow later through AttachTerrain. A region is never copied or moved: its owner holds it in place.
  RegionSimulation(std::uint16_t _index, const Neuron::SipHashKey& _simulationKey, std::uint64_t _nextTick, const TuningMvp& _tuning,
                   std::shared_ptr<const RegionTerrain> _terrain = nullptr);
  RegionSimulation(const RegionSimulation&) = delete;
  RegionSimulation& operator=(const RegionSimulation&) = delete;

  // One region job of tick _tick (R-SIM-510): its commands, then the update slices 0 to ref.sim.update_slices - 1
  // (R-SIM-511, R-SIM-218), each running the region's teams in index order (R-SIM-221), then its laser bolts, its
  // effects, its bot navigation nodes, the only structures before M2, and its Essence orbs (R-SIM-218 steps 3 to 5);
  // then each AI, whose commands execute at the next tick (ADR-107).
  void Step(std::uint64_t _tick);

  // The team slots 0 to _index, made when they do not exist yet.
  Team& EnsureTeam(std::uint8_t _index);
  [[nodiscard]] std::size_t TeamCount() const noexcept
  {
    return m_teams.size();
  }
  [[nodiscard]] Team& TeamAt(std::size_t _index);
  [[nodiscard]] const Team& TeamAt(std::size_t _index) const;

  struct Spawn
  {
    std::uint8_t team = 0;
    FigureKind kind = FigureKind::Worker;
    Neuron::Vec3 position;
    Neuron::Vec3 velocity;
    std::uint32_t count = 1;
    double spread = 0.0;
    std::optional<double> roamRange; // none: the spread (R-SIM-226 step 9)
  };
  // R-SIM-226: creates the figures one after another into the team's ungrouped figures, each at the spawn-position
  // search's point around the position (R-SIM-368) and registered in the actor grid, and returns their slots. Its
  // draws are the next tick's: inside a step, the tick being stepped. It needs the region's terrain.
  std::vector<std::uint32_t> SpawnFigures(const Spawn& _spawn);

  // The figure an actor-grid entry names, or null when it no longer resolves (R-SIM-325's rule for handles).
  [[nodiscard]] const Figure* Resolve(const ActorRef& _ref) const;
  [[nodiscard]] Figure* Resolve(const ActorRef& _ref);
  // The figure of _team whose stable id is _id, found by a scan of the team's slots, or none (R-SIM-449: a command
  // names a figure by stable id).
  [[nodiscard]] std::optional<ActorRef> FindFigure(std::uint8_t _team, StableId _id) const;
  // R-SIM-405 for the MVP's selections: _leader, or nothing, becomes the team's selected actor, and every Worker of the
  // team leaves the crowd selection (R-UNIT-137). Selecting a Leader raises leader.selected.
  void Select(std::uint8_t _team, const std::optional<ActorRef>& _leader);
  // The team's selected Leader while it resolves and is alive, or null.
  [[nodiscard]] Figure* SelectedLeader(std::uint8_t _team);
  [[nodiscard]] std::optional<ActorRef> SelectedLeaderRef(std::uint8_t _team) const;
  // The live Leader that _worker of _team follows, or null.
  [[nodiscard]] Figure* LeaderOf(const Figure& _worker, std::uint8_t _team);
  [[nodiscard]] const Figure* LeaderOf(const Figure& _worker, std::uint8_t _team) const;
  // R-UNIT-183: _worker of _team holds a slot of the Leader it follows, and that Leader is in formation mode (ADR-106).
  [[nodiscard]] bool InFormation(const Figure& _worker, std::uint8_t _team) const;
  // ADR-106: _sender commands _team from now on; an AI commander may also send the AI's own commands. Called between
  // ticks, as a connection is (ADR-095).
  void SetCommander(std::uint8_t _team, Neuron::SenderId _sender, bool _ai);
  // ADR-107: an AI of the region's own makes and commands _team from now on; its sender id is a new stable id of the
  // region. Called between ticks.
  StableId AddAi(std::uint8_t _team);
  [[nodiscard]] const std::vector<AiController>& Ais() const noexcept
  {
    return m_ais;
  }
  // The bot navigation graph of the region's terrain (ADR-107), empty with no terrain, and the state of its nodes in
  // node order.
  [[nodiscard]] const BotGraph& Bots() const noexcept
  {
    return m_bots;
  }
  [[nodiscard]] const SlicedPool<BotNode>& BotNodes() const noexcept
  {
    return m_botNodes;
  }
  // The arrival check of a battle command (ADR-087 item 2): _sender commands _team, and is an AI when _aiOnly.
  [[nodiscard]] bool MayCommand(Neuron::SenderId _sender, std::uint8_t _team, bool _aiOnly) const noexcept;

  // R-SIM-311: a team's friends and enemies. The MVP has no alliance, so each team is its own only friend.
  [[nodiscard]] static TeamMask FriendsOf(std::uint8_t _team) noexcept;
  [[nodiscard]] static TeamMask EnemiesOf(std::uint8_t _team) noexcept;
  // R-SIM-309 and R-SIM-311: the exact query over the teams of _teams.
  [[nodiscard]] std::vector<ActorRef> InRange(double _x, double _z, double _range, const TeamMask& _teams) const;
  // R-SIM-311: the nearest live enemy of _team with _minRange² ≤ d² < the best so far, within _maxRange.
  [[nodiscard]] std::optional<ActorRef> NearestEnemy(double _x, double _z, double _minRange, double _maxRange, std::uint8_t _team) const;
  // R-SIM-316: the Worker threat search's cached enemy query, at the tick being stepped.
  [[nodiscard]] std::vector<ActorRef> CachedEnemies(double _x, double _z, double _range, std::uint8_t _team);
  [[nodiscard]] const ActorGrid& Grid() const noexcept
  {
    return m_grid;
  }
  [[nodiscard]] const ActorGridCache& Cache() const noexcept
  {
    return m_cache;
  }
  [[nodiscard]] ObstacleGrid& Obstacles() noexcept
  {
    return m_obstacles;
  }
  // R-SIM-345 and R-SIM-352: a route over the region's land route graph, stored under the next handle (ADR-104). It
  // needs the region's terrain, and it is freed at the end of the next step that ends with no figure holding it.
  RouteHandle BuildRoute(const Neuron::Vec3& _from, const Neuron::Vec3& _to, bool _directRoute, bool _permitClimbing = true,
                         bool _noRounding = false);
  [[nodiscard]] const RouteStore& Routes() const noexcept
  {
    return m_routes;
  }
  // The movement rules' view of the region at its next tick: inside a step, the tick being stepped (ADR-103).
  [[nodiscard]] MovementContext Movement();

  // The laser-bolt, effect and Essence-orb pools (R-SIM-125, R-SIM-124, R-SIM-126), which the weapon and orb rules
  // create into and the region advances after its teams in each slice (ADR-105).
  [[nodiscard]] SlicedPool<LaserBolt>& LaserBolts() noexcept
  {
    return m_bolts;
  }
  [[nodiscard]] const SlicedPool<LaserBolt>& LaserBolts() const noexcept
  {
    return m_bolts;
  }
  [[nodiscard]] SlicedPool<Effect>& Effects() noexcept
  {
    return m_effects;
  }
  [[nodiscard]] const SlicedPool<Effect>& Effects() const noexcept
  {
    return m_effects;
  }
  [[nodiscard]] SlicedPool<EssenceOrb>& Orbs() noexcept
  {
    return m_orbs;
  }
  [[nodiscard]] const SlicedPool<EssenceOrb>& Orbs() const noexcept
  {
    return m_orbs;
  }
  // The grenade in effect slot _slot while it still holds that slot with id _id (a Worker's grenade threat), or null.
  [[nodiscard]] const Effect* GrenadeAt(std::uint32_t _slot, StableId _id) const;
  // A new stable id of the region's counter, for a bolt, an effect or an orb (ADR-083 item 5).
  [[nodiscard]] StableId IssueId()
  {
    return m_ids.Issue();
  }
  // The draws of the tick being stepped (ADR-103).
  [[nodiscard]] TickDraws& Draws() noexcept
  {
    return m_draws;
  }
  [[nodiscard]] const TickDraws& Draws() const noexcept
  {
    return m_draws;
  }
  // R-UNIT-026 and R-UNIT-038 for the figure an entry names: its health change with its hurt and death events, and on a
  // kill its one Essence orb (MVP-023). Kill credit is the caller's. A figure that no longer resolves takes nothing.
  HealthChange Damage(const ActorRef& _ref, std::int32_t _amount, DamageKind _kind);
  // R-UNIT-121: sets the Worker an entry names on fire; the MVP has no Transport, Shield or Rogue faction to refuse it.
  void SetOnFire(const ActorRef& _ref);
  // ADR-036, ADR-105: a presentation event of the tick being stepped.
  void Emit(const EffectEvent& _event)
  {
    m_events.push_back(_event);
  }
  // The presentation events of the last step, in the order the rules emitted them; a step clears the list first.
  [[nodiscard]] const std::vector<EffectEvent>& Events() const noexcept
  {
    return m_events;
  }

  // Every mutable value of the region, canonically (ADR-083 item 4): what its hash covers and its snapshot holds.
  void WriteState(Neuron::BitWriter& _writer) const;
  // The values WriteState wrote in a snapshot of _stateVersion, into a region made for the same index and tick; false
  // when they do not read whole. A version-1 region has no teams; a region before version 4 has no grid history, and its
  // figures register at their positions.
  [[nodiscard]] bool ReadState(Neuron::BitReader& _reader, std::uint32_t _stateVersion);

  [[nodiscard]] std::uint16_t Index() const noexcept
  {
    return m_index;
  }
  [[nodiscard]] OwnerId Owner() const noexcept
  {
    return m_ids.Owner();
  }
  // The next tick to step: the region's tick clock.
  [[nodiscard]] std::uint64_t NextTick() const noexcept
  {
    return m_nextTick;
  }
  [[nodiscard]] Neuron::CommandQueue& Commands() noexcept
  {
    return m_commands;
  }
  [[nodiscard]] const Neuron::CommandQueue& Commands() const noexcept
  {
    return m_commands;
  }
  [[nodiscard]] const StableIdCounter& Ids() const noexcept
  {
    return m_ids;
  }
  [[nodiscard]] const TuningMvp& Tuning() const noexcept
  {
    return *m_tuning;
  }
  // The region's terrain (ADR-102): built from the world seed outside every tick and never changed (MVP-011), and
  // derived rather than state, so neither the hash nor the snapshot holds it. None until the world server attaches it.
  // Attaching it builds the bot navigation graph, and makes the nodes' state when the region holds none (ADR-107).
  void AttachTerrain(std::shared_ptr<const RegionTerrain> _terrain);
  [[nodiscard]] const RegionTerrain* Terrain() const noexcept
  {
    return m_terrain.get();
  }

#if ENGINE_DEVELOPMENT
  // A test's view of the update order: called before each figure's advance with its team, slot and slice.
  using AdvanceObserver = std::function<void(std::uint8_t, std::uint32_t, std::uint32_t)>;
  void ObserveAdvances(AdvanceObserver _observer)
  {
    m_advanceObserver = std::move(_observer);
  }
#endif

private:
  void StepSlice(std::uint32_t _slice);
  // R-SIM-308, DEC-650: after an advance that keeps the figure, its entries follow its cells; a figure that asks to be
  // removed or is disabled leaves the grid (R-SIM-114).
  void UpdateRegistration(std::uint8_t _team, std::uint32_t _slot, Figure& _figure);
  void Unregister(std::uint8_t _team, std::uint32_t _slot, Figure& _figure);
  // DEC-651 resolved by O71: at the end of the tick, the routes that no figure holds are freed.
  void FreeUnheldRoutes();
  // ADR-107: the node states of the bot navigation graph, each with its phase draw, when the region holds none.
  void EnsureBotNodes();

  std::uint16_t m_index;
  const TuningMvp* m_tuning;
  StableIdCounter m_ids;
  std::vector<Commander> m_commanders; // by team index; state, read before the queue that the commanders permit
  Neuron::CommandQueue m_commands;
  std::uint64_t m_nextTick;
  std::vector<Team> m_teams;
  std::shared_ptr<const RegionTerrain> m_terrain;
  double m_originX;
  double m_originZ;
  double m_side;
  TickDraws m_draws;
  ActorGrid m_grid;
  ActorGridCache m_cache;
  ObstacleGrid m_obstacles;
  RouteStore m_routes;
  SlicedPool<LaserBolt> m_bolts;
  SlicedPool<Effect> m_effects;
  SlicedPool<EssenceOrb> m_orbs;
  BotGraph m_bots; // derived from the terrain, never state
  SlicedPool<BotNode> m_botNodes;
  std::vector<AiController> m_ais;
  std::vector<EffectEvent> m_events; // presentation, never state
#if ENGINE_DEVELOPMENT
  AdvanceObserver m_advanceObserver;
#endif
};

} // namespace Species
