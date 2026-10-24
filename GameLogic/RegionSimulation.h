#pragma once

#include "ActorGrid.h"
#include "ActorGridCache.h"
#include "AiController.h"
#include "Battles.h"
#include "BitReader.h"
#include "BitWriter.h"
#include "BotNavigation.h"
#include "Claims.h"
#include "CommandQueue.h"
#include "Deputy.h"
#include "EffectEvent.h"
#include "EssenceOrb.h"
#include "Figure.h"
#include "Movement.h"
#include "ObstacleGrid.h"
#include "Presence.h"
#include "RegionTerrain.h"
#include "Relations.h"
#include "Renown.h"
#include "RouteStore.h"
#include "ScriptedSteward.h"
#include "Settlement.h"
#include "SettlementAi.h"
#include "Statistics.h"
#include "SlicedPool.h"
#include "StableId.h"
#include "StableIdCounter.h"
#include "Structure.h"
#include "StructureRecords.h"
#include "Territory.h"
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
/// (ADR-103); its settlements and its structures (MVP-096, ADR-130), and the battles of M3.
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

  // _terrain may follow later through AttachTerrain. _structures, which outlive the region, are the records it builds
  // structures from (ADR-130); a region made without them founds no settlement and places no resource site. A region is
  // never copied or moved: its owner holds it in place.
  RegionSimulation(std::uint16_t _index, const Neuron::SipHashKey& _simulationKey, std::uint64_t _nextTick, const TuningMvp& _tuning,
                   std::shared_ptr<const RegionTerrain> _terrain = nullptr, const StructureRecords* _structures = nullptr);
  RegionSimulation(const RegionSimulation&) = delete;
  RegionSimulation& operator=(const RegionSimulation&) = delete;

  // One region job of tick _tick (R-SIM-510): its commands, then the update slices 0 to ref.sim.update_slices - 1
  // (R-SIM-511, R-SIM-218), each running the region's teams in index order (R-SIM-221), then its laser bolts, its
  // effects, its structures and then its bot navigation nodes, and its Essence orbs (R-SIM-218 steps 3 to 5);
  // then each AI, whose commands execute at the next tick (ADR-107).
  void Step(std::uint64_t _tick);
  // MVP-063 (ADR-163): a tick of a world whose run has ended: the region's tick advances, its presentation events are
  // cleared, and nothing else changes.
  void Pass(std::uint64_t _tick);

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
  // Who commands _team; nobody for a team that has no commander.
  [[nodiscard]] Commander CommanderOf(std::uint8_t _team) const noexcept
  {
    return _team < m_commanders.size() ? m_commanders[_team] : Commander{};
  }
  // The lowest team that _sender commands as a steward, not an AI; none when it commands none.
  [[nodiscard]] std::optional<std::uint8_t> TeamCommandedBy(Neuron::SenderId _sender) const noexcept;
  // ADR-107: an AI of the region's own makes and commands _team from now on; its sender id is a new stable id of the
  // region. Called between ticks.
  StableId AddAi(std::uint8_t _team);
  // ADR-138: a scripted steward runs the settlement of _team from now on as its AI commander; its sender id is a new
  // stable id of the region. For an AI settlement it also makes the settlement's decisions (ADR-162). Called between
  // ticks.
  StableId AddScriptedSteward(std::uint8_t _team);
  // ADR-156: the scripted stewards of _team stop, and their decisions with them, as a steward takes its settlement.
  // Called between ticks.
  void RemoveScriptedStewards(std::uint8_t _team);
  [[nodiscard]] const std::vector<ScriptedSteward>& ScriptedStewards() const noexcept
  {
    return m_scripted;
  }
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

  // R-SIM-311: a team's friends and enemies. The MVP has no alliance, so each team is its own only friend. A team's
  // enemies are every other team, except that a settlement's team has no other settlement's for one: the settlements
  // are at peace outside battles (PIL-041, ADR-157).
  [[nodiscard]] static TeamMask FriendsOf(std::uint8_t _team) noexcept;
  [[nodiscard]] TeamMask EnemiesOf(std::uint8_t _team) const noexcept;
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
  [[nodiscard]] const ObstacleGrid& Obstacles() const noexcept
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
  // The region's structures (R-SIM-218 step 4), in slot order, and its settlements in the order they were founded
  // (MVP-096, ADR-130).
  [[nodiscard]] const SlicedPool<Structure>& Structures() const noexcept
  {
    return m_structures;
  }
  [[nodiscard]] SlicedPool<Structure>& Structures() noexcept
  {
    return m_structures;
  }
  [[nodiscard]] const std::vector<Settlement>& Settlements() const noexcept
  {
    return m_settlements;
  }
  [[nodiscard]] std::vector<Settlement>& Settlements() noexcept
  {
    return m_settlements;
  }
  // The settlement at local site _site, or null while the site is free.
  [[nodiscard]] const Settlement* SettlementAt(std::uint8_t _site) const noexcept;
  // MVP-026: the territory of the settlement at local site _site, or null while the site is free (ADR-134).
  [[nodiscard]] const CellRaster* TerritoryOf(std::uint8_t _site) const noexcept;
  // The territory of the settlement at _site made again from its radius, as when its radius grows (ADR-134).
  void RecomputeTerritory(std::uint8_t _site);
  // MVP-016 and MVP-097: the seen layer of the settlement at _site as the last step made it, or null before the
  // region's first step and while the site is free; and how many times the region has made its seen layers, once in
  // every step (ADR-134).
  [[nodiscard]] const CellRaster* SeenLayerOf(std::uint8_t _site) const noexcept;
  [[nodiscard]] std::uint64_t SeenLayerUpdates() const noexcept
  {
    return m_seenUpdates;
  }
  // WLD-066: the region's territory events in the order they were recorded (MVP-027).
  [[nodiscard]] const std::vector<ContactEvent>& ContactEvents() const noexcept
  {
    return m_contacts;
  }
  // MVP-050 (ADR-159): the grievances of the region's Cladans, in their order.
  [[nodiscard]] const std::vector<Grievance>& Grievances() const noexcept
  {
    return m_grievances;
  }
  [[nodiscard]] std::vector<Grievance>& Grievances() noexcept
  {
    return m_grievances;
  }
  // MVP-053 (ADR-160): the region's claims in the order they were made, and the areas claims have moved, in their order.
  [[nodiscard]] const std::vector<Claim>& Claims() const noexcept
  {
    return m_claims;
  }
  [[nodiscard]] std::vector<Claim>& Claims() noexcept
  {
    return m_claims;
  }
  [[nodiscard]] const std::vector<AreaHolding>& Holdings() const noexcept
  {
    return m_holdings;
  }
  [[nodiscard]] std::vector<AreaHolding>& Holdings() noexcept
  {
    return m_holdings;
  }
  [[nodiscard]] const std::vector<ClaimEnd>& ClaimEnds() const noexcept
  {
    return m_claimEnds;
  }
  [[nodiscard]] std::vector<ClaimEnd>& ClaimEnds() noexcept
  {
    return m_claimEnds;
  }
  // MVP-072 to MVP-074 (ADR-184): the stewarded settlements' open and latest closed absences, in the order they opened.
  [[nodiscard]] const std::vector<Absence>& Absences() const noexcept
  {
    return m_absences;
  }
  [[nodiscard]] std::vector<Absence>& Absences() noexcept
  {
    return m_absences;
  }
  // MVP-073 (ADR-185): the stewarded settlements' deputies, in the order they were made.
  [[nodiscard]] const std::vector<Deputy>& Deputies() const noexcept
  {
    return m_deputies;
  }
  // The deputy of the settlement of _team, made with a new stable id of the region when it has none. Called between
  // ticks, as its steward goes away (ADR-184).
  Deputy& EnsureDeputy(std::uint8_t _team);
  // WLD-066 (ADR-160): a territory event that is no first contact, a claim's move of a border area.
  void RecordContactEvent(const ContactEvent& _event)
  {
    m_contacts.push_back(_event);
  }
  // MVP-041 to MVP-048 (ADR-161): the region's expeditions in the order declared, its battles in progress in the order
  // started, its battle results in the order they ended, and its respites in the order started.
  [[nodiscard]] const std::vector<Expedition>& Expeditions() const noexcept
  {
    return m_expeditions;
  }
  [[nodiscard]] std::vector<Expedition>& Expeditions() noexcept
  {
    return m_expeditions;
  }
  [[nodiscard]] const std::vector<Battle>& Battles() const noexcept
  {
    return m_battles;
  }
  [[nodiscard]] std::vector<Battle>& Battles() noexcept
  {
    return m_battles;
  }
  [[nodiscard]] const std::vector<BattleResult>& BattleResults() const noexcept
  {
    return m_results;
  }
  [[nodiscard]] std::vector<BattleResult>& BattleResults() noexcept
  {
    return m_results;
  }
  [[nodiscard]] const std::vector<Respite>& Respites() const noexcept
  {
    return m_respites;
  }
  [[nodiscard]] std::vector<Respite>& Respites() noexcept
  {
    return m_respites;
  }
  // MVP-055 to MVP-058 (ADR-162): the decisions of the AI settlements its scripted stewards run, in their order.
  [[nodiscard]] const std::vector<SettlementAi>& SettlementAis() const noexcept
  {
    return m_settlementAis;
  }
  [[nodiscard]] std::vector<SettlementAi>& SettlementAis() noexcept
  {
    return m_settlementAis;
  }
  // ADR-161: the figure at slot _slot of team _from moves to team _to, registered in the actor grid there, and leaves
  // its old team's selection; returns its new slot. The caller remaps what named its old slot.
  std::uint32_t MoveFigure(std::uint8_t _from, std::uint32_t _slot, std::uint8_t _to);
  // ADR-161: the actor grid cache forgets every query, when the teams' enemies change.
  void ForgetCachedEnemies() noexcept
  {
    m_cache.Clear();
  }
  // MVP-064: the server's statistics of the region's settlements in the order they were recorded (ADR-136).
  [[nodiscard]] const std::vector<StatisticEntry>& Statistics() const noexcept
  {
    return m_statistics;
  }
  // An entry appended to the statistics while they hold fewer than STATISTICS_MAX; a full record takes no more.
  void RecordStatistic(const StatisticEntry& _entry);
  // ADR-163: the territory of the settlement at _site has reached the halfway line of valley border pair _pair, as the
  // statistics record.
  [[nodiscard]] bool HalfwayReached(std::uint8_t _site, std::size_t _pair) const noexcept;
  // MVP-061 (ADR-163): the renown of the region's settlements, one for each, in their order.
  [[nodiscard]] const std::vector<SettlementRenown>& Renown() const noexcept
  {
    return m_renown;
  }
  [[nodiscard]] std::vector<SettlementRenown>& Renown() noexcept
  {
    return m_renown;
  }
  // MVP-060 (ADR-163): the tick the world's run ends at, which the world owner gives its regions; never, for a region of
  // no world. In the last mvp.run.quiet_minutes before it, no attack may be declared.
  void SetRunEnd(std::uint64_t _endTick) noexcept
  {
    m_runEndTick = _endTick;
  }
  [[nodiscard]] std::uint64_t RunEndTick() const noexcept
  {
    return m_runEndTick;
  }
  [[nodiscard]] bool InQuietMinutes() const noexcept;
  // The structure in _slot while it still holds that slot with id _id, or null (R-SIM-325's rule for handles).
  [[nodiscard]] Structure* ResolveStructure(std::uint32_t _slot, StableId _id) noexcept;
  [[nodiscard]] const Structure* ResolveStructure(std::uint32_t _slot, StableId _id) const noexcept;
  // The records the region builds structures from, or null (ADR-130).
  [[nodiscard]] const StructureRecords* Records() const noexcept
  {
    return m_records;
  }
  // R-BLDG-029 and R-BLDG-030 for a structure that a rule creates: made at (_x, _z) facing _front from its kind's record
  // and collision asset on the region's terrain (MakeStructure), given the next stable id of the region, inserted into
  // the structure pool and added to the obstacle grid; returns its slot. It needs the terrain and the records.
  std::uint32_t AddStructure(StructureKind _kind, std::optional<std::uint8_t> _owner, std::optional<std::uint32_t> _resourceSite, double _x,
                             double _z, const Neuron::Vec3& _front);
  // R-BLDG-030 for a structure already made: the next stable id of the region, the structure pool and the obstacle grid;
  // returns its slot.
  std::uint32_t InsertStructure(Structure _structure);
  // WLD-080 to WLD-085: a structure for each resource site of the region's terrain, in its order, belonging to no
  // settlement (WLD-084).
  void PlaceResourceSites();
  // MVP-012, MVP-013 and MVP-020: a settlement at the free local site _site, run by a steward or an AI with the temperament
  // _temperament (MVP-057). Its team slot is its site and its Cladan's colour too; its Wellspring stands at the site's centre facing out along the site's
  // outward diagonal; its stores are empty, its territory has the founding radius, and its first
  // mvp.settlement.start_workers Workers leave the Wellspring's door one after another (R-ECON-043). Its draws are the
  // next tick's. It needs the terrain and the records.
  const Settlement& FoundSettlement(std::uint8_t _site, bool _stewarded, Temperament _temperament = Temperament::None);
  // R-ECON-043 for a Wellspring (MVP-021): one Worker of the settlement appears at its Wellspring's door socket with no
  // position spread and the velocity (door front + j) × ref.econ.spawn_node.door_speed, each component of j drawn from
  // U[-ref.econ.spawn_node.door_jitter, +ref.econ.spawn_node.door_jitter), and airborne. Returns its slot.
  std::uint32_t SpawnAtDoor(const Settlement& _settlement);

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
  // ADR-165: a guaranteed event of the valley in the tick being stepped, for the stewards' notices (MVP-102).
  void Raise(const ValleyEvent& _event)
  {
    m_valleyEvents.push_back(_event);
  }
  // The valley events of the last step, in the order the rules raised them; a step clears the list first.
  [[nodiscard]] const std::vector<ValleyEvent>& ValleyEvents() const noexcept
  {
    return m_valleyEvents;
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
  // ADR-163: the halfway lines the entry records reached.
  void NoteHalfway(const StatisticEntry& _entry) noexcept;
  void StepSlice(std::uint32_t _slice);
  // R-SIM-218 step 2: each team's share of slice _slice of _slices, in team order.
  void AdvanceTeams(std::uint32_t _slice, std::uint32_t _slices, const MovementContext& _context);
  // R-SIM-308, DEC-650: after an advance that keeps the figure, its entries follow its cells; a figure that asks to be
  // removed or is disabled leaves the grid (R-SIM-114).
  void UpdateRegistration(std::uint8_t _team, std::uint32_t _slot, Figure& _figure);
  void Unregister(std::uint8_t _team, std::uint32_t _slot, Figure& _figure);
  // DEC-651 resolved by O71: at the end of the tick, the routes that no figure holds are freed.
  void FreeUnheldRoutes();
  // ADR-107: the node states of the bot navigation graph, each with its phase draw, when the region holds none.
  void EnsureBotNodes();
  // R-SIM-321, DEC-095, ADR-131: the obstacle grid made again from every structure in slot order, its shape its collision
  // asset's triangles at its transform; derived data, so made whenever a structure is added or removed and when a
  // restored region's terrain is attached.
  void RebuildObstacles();
  // ADR-134: every settlement's territory made again; derived data, made when a settlement is founded, when a restored
  // region's terrain is attached and after a snapshot is read.
  void RecomputeTerritories();
  // MVP-027 and WLD-064: a contact event for each valley border pair whose two settlements both reach its halfway line
  // and have no contact yet; the event is recorded once, and the relations read whether a pair is in contact now
  // (ADR-159).
  void RecordContacts();
  // MVP-097: every settlement's seen layer, made once per step.
  void UpdateSeenLayers();

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
  const StructureRecords* m_records;
  SlicedPool<Structure> m_structures;
  std::vector<Settlement> m_settlements;
  std::vector<CellRaster> m_territories;                                  // by settlement, derived, never state
  std::vector<CellRaster> m_seen;                                         // by settlement, derived, never state
  std::uint64_t m_seenUpdates = 0;                                        // never state
  std::vector<ContactEvent> m_contacts;                                   // WLD-066, state from version 9
  std::vector<StatisticEntry> m_statistics;                               // MVP-064, state from version 9
  std::vector<ScriptedSteward> m_scripted;                                // ADR-138, state from version 10
  std::vector<Grievance> m_grievances;                                    // MVP-050 (ADR-159), state from version 11
  std::vector<Claim> m_claims;                                            // MVP-053 (ADR-160), state from version 12
  std::vector<AreaHolding> m_holdings;                                    // MVP-053 (ADR-160), state from version 12
  std::vector<Expedition> m_expeditions;                                  // MVP-041 (ADR-161), state from version 13
  std::vector<Battle> m_battles;                                          // MVP-043 (ADR-161), state from version 13
  std::vector<BattleResult> m_results;                                    // MVP-048 (ADR-161), state from version 13
  std::vector<Respite> m_respites;                                        // MVP-047 (ADR-161), state from version 13
  std::vector<SettlementAi> m_settlementAis;                              // MVP-055 (ADR-162), state from version 14
  std::vector<SettlementRenown> m_renown;                                 // MVP-061 (ADR-163), state from version 15
  std::vector<ClaimEnd> m_claimEnds;                                      // O102 (DEC-1008), state from version 16
  std::vector<Absence> m_absences;                                        // MVP-072 (ADR-184), state from version 19
  std::vector<Deputy> m_deputies;                                         // MVP-073 (ADR-185), state from version 20
  std::uint64_t m_runEndTick = std::numeric_limits<std::uint64_t>::max(); // the world owner's (ADR-163), not region state
  std::uint16_t m_halfwayReached = 0;      // a bit for each site and pair, from the statistics (ADR-163), not region state
  std::vector<EffectEvent> m_events;       // presentation, never state
  std::vector<ValleyEvent> m_valleyEvents; // the stewards' notices (ADR-165), never state
#if ENGINE_DEVELOPMENT
  AdvanceObserver m_advanceObserver;
#endif
};

} // namespace Species
