#include "pch.h"

#include "Check.h"
#include "EssenceOrb.h"
#include "FigureAdvance.h"
#include "Formation.h"
#include "Growth.h"
#include "Profile.h"
#include "RegionCommands.h"
#include "RegionSimulation.h"
#include "SeenLayer.h"
#include "SettlementCommands.h"
#include "StateValues.h"
#include "StructureAdvance.h"
#include "Weapons.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>

namespace Species
{

namespace
{

SlicedPool<Figure> MakeFigurePool(const TuningMvp& _tuning)
{
  return SlicedPool<Figure>(static_cast<std::uint32_t>(_tuning.ref.sim.container.ungroupedStep));
}

// The region's square (ADR-103): the MVP's one region at grid cell mvp.world.region_cell on both axes (ADR-102).
double OriginOf(const TuningMvp& _tuning) noexcept
{
  return static_cast<double>(_tuning.mvp.world.regionCell) * _tuning.wld.region.side;
}

ActorGrid::Desc ActorGridDescOf(const TuningMvp& _tuning)
{
  const auto& grid = _tuning.ref.sim.actorGrid;
  ActorGrid::Desc desc;
  desc.originX = OriginOf(_tuning);
  desc.originZ = OriginOf(_tuning);
  desc.worldSize = _tuning.wld.region.side;
  desc.cellSize = grid.cellSize;
  desc.extraCells = static_cast<std::int32_t>(grid.extraCells);
  desc.footprintFactor = grid.footprintFactor;
  desc.initialCapacity = static_cast<std::uint32_t>(grid.initialCellCapacity);
  desc.firstCapacity = static_cast<std::uint32_t>(grid.firstCellCapacity);
  desc.growthFactor = static_cast<std::uint32_t>(grid.cellGrowthFactor);
  return desc;
}

ObstacleGrid::Desc ObstacleGridDescOf(const TuningMvp& _tuning)
{
  ObstacleGrid::Desc desc;
  desc.originX = OriginOf(_tuning);
  desc.originZ = OriginOf(_tuning);
  desc.worldSize = _tuning.wld.region.side;
  desc.cellSize = _tuning.ref.sim.obstacleGrid.cellSize;
  desc.cellTestFraction = _tuning.ref.sim.obstacleGrid.cellTestFraction;
  return desc;
}

// The figure an entry names among _teams, const or not as _teams is, or null when it no longer resolves.
template <class Teams> auto* ResolveIn(Teams& _teams, const ActorRef& _ref)
{
  using Result = decltype(&_teams[0].figures.At(0));
  if (_ref.team >= _teams.size() || !_teams[_ref.team].figures.Used(_ref.slot))
  {
    return static_cast<Result>(nullptr);
  }
  auto& figure = _teams[_ref.team].figures.At(_ref.slot);
  return figure.id == _ref.id ? &figure : static_cast<Result>(nullptr);
}

// The first state versions whose regions hold the actor grid's capacities and the cache (M1 task T4), the routes (M1
// task T5), the kill counters with the laser-bolt, effect and Essence-orb pools (M1 task T6), and the commanders with
// each team's selection and pointer (M1 task T7).
constexpr std::uint32_t STATE_VERSION_WITH_GRIDS = 4;
constexpr std::uint32_t STATE_VERSION_WITH_ROUTES = 5;
constexpr std::uint32_t STATE_VERSION_WITH_WEAPONS = 6;
constexpr std::uint32_t STATE_VERSION_WITH_LEADERS = 7;
// ... and the AIs and the bot navigation nodes' state (M1 task T8).
constexpr std::uint32_t STATE_VERSION_WITH_BOTS = 8;
// ... and the settlements and the structures (M2 task T4).
constexpr std::uint32_t STATE_VERSION_WITH_SETTLEMENTS = 9;
// A region holds at most one settlement on each of its four sites (MVP-010).
constexpr std::uint64_t SETTLEMENTS_MAX = std::tuple_size_v<decltype(RegionLayout::sites)>;
// WLD-066: the territory events a region holds; the MVP records one contact for each valley border pair at most.
constexpr std::uint64_t CONTACT_EVENTS_MAX = 4095;
// MVP-064: the statistics a region holds, far above a run's: four settlements sampled every 5 minutes for 8 hours, with
// their growth steps, record under 2,000 (ADR-136).
constexpr std::uint64_t STATISTICS_MAX = 65'535;
// The node pool is made whole once, with every node of the graph, and never grows (ADR-107).
constexpr std::uint32_t BOT_NODE_POOL_STEP = 1;

} // namespace

RegionSimulation::RegionSimulation(std::uint16_t _index, const Neuron::SipHashKey& _simulationKey, std::uint64_t _nextTick,
                                   const TuningMvp& _tuning, std::shared_ptr<const RegionTerrain> _terrain,
                                   const StructureRecords* _structures)
  : m_index(_index),
    m_tuning(&_tuning),
    m_ids(static_cast<OwnerId>(REGION_OWNER_ID_FIRST + _index)),
    m_commands(_simulationKey, _nextTick),
    m_nextTick(_nextTick),
    m_terrain(std::move(_terrain)),
    m_originX(OriginOf(_tuning)),
    m_originZ(OriginOf(_tuning)),
    m_side(_tuning.wld.region.side),
    m_draws(_simulationKey),
    m_grid(ActorGridDescOf(_tuning)),
    m_cache({_tuning.ref.sim.gridCache.reuseFraction, static_cast<std::uint64_t>(_tuning.ref.sim.gridCache.maxAge)}),
    m_obstacles(ObstacleGridDescOf(_tuning)),
    m_bolts(static_cast<std::uint32_t>(_tuning.ref.sim.container.laserBoltStep)),
    m_effects(static_cast<std::uint32_t>(_tuning.ref.sim.container.effectStep)),
    m_orbs(static_cast<std::uint32_t>(_tuning.ref.sim.container.orbStep), static_cast<std::uint32_t>(_tuning.ref.sim.container.orbPresize)),
    m_botNodes(BOT_NODE_POOL_STEP),
    m_records(_structures),
    m_structures(static_cast<std::uint32_t>(_tuning.ref.sim.container.structureStep))
{
  ENGINE_CHECK(_tuning.ref.sim.updateSlices > 0 && _tuning.ref.sim.container.ungroupedStep > 0);
  m_draws.SetTick(_nextTick);
  RegisterRegionCommands(*this);
  RegisterSettlementCommands(*this);
  if (m_terrain)
  {
    m_bots = BotGraph::Build(m_terrain->routes.Nodes(), m_terrain->field, _tuning);
    EnsureBotNodes();
  }
}

void RegionSimulation::AttachTerrain(std::shared_ptr<const RegionTerrain> _terrain)
{
  m_terrain = std::move(_terrain);
  m_bots = m_terrain ? BotGraph::Build(m_terrain->routes.Nodes(), m_terrain->field, *m_tuning) : BotGraph{};
  EnsureBotNodes();
  if (m_terrain && m_structures.Count() != 0)
  {
    RebuildObstacles();
  }
  RecomputeTerritories();
  ENGINE_CHECK(m_botNodes.Count() == m_bots.Size(), "region {} holds {} bot nodes for a graph of {}", m_index, m_botNodes.Count(),
               m_bots.Size());
}

void RegionSimulation::EnsureBotNodes()
{
  if (m_botNodes.Count() != 0 || m_bots.Size() == 0)
  {
    return;
  }
  // R-BOT-028: each node draws its phase once, in the step that makes it: here the next tick's.
  m_draws.SetTick(m_nextTick);
  m_botNodes = SlicedPool<BotNode>(BOT_NODE_POOL_STEP, static_cast<std::uint32_t>(m_bots.Size()));
  for (std::uint32_t node = 0; node < m_bots.Size(); ++node)
  {
    const std::uint32_t slot = m_botNodes.Take(MakeBotNode(*this, node));
    ENGINE_ASSERT(slot == node);
  }
}

StableId RegionSimulation::AddAi(std::uint8_t _team)
{
  const StableId id = m_ids.Issue();
  m_draws.SetTick(m_nextTick);
  m_ais.push_back(MakeAi(*this, id, _team));
  SetCommander(_team, id, true);
  return id;
}

RegionSimulation::Team& RegionSimulation::EnsureTeam(std::uint8_t _index)
{
  while (m_teams.size() <= _index)
  {
    m_teams.push_back(Team{MakeFigurePool(*m_tuning)});
  }
  return m_teams[_index];
}

RegionSimulation::Team& RegionSimulation::TeamAt(std::size_t _index)
{
  ENGINE_CHECK_INDEX(_index, m_teams.size());
  return m_teams[_index];
}

const RegionSimulation::Team& RegionSimulation::TeamAt(std::size_t _index) const
{
  ENGINE_CHECK_INDEX(_index, m_teams.size());
  return m_teams[_index];
}

MovementContext RegionSimulation::Movement()
{
  m_draws.SetTick(m_nextTick);
  MovementContext context;
  context.tuning = m_tuning;
  context.terrain = m_terrain ? &m_terrain->field : nullptr;
  context.obstacles = &m_obstacles;
  context.draws = &m_draws;
  context.routes = &m_routes;
  context.minX = m_originX;
  context.minZ = m_originZ;
  context.maxX = m_originX + m_side;
  context.maxZ = m_originZ + m_side;
  return context;
}

std::vector<std::uint32_t> RegionSimulation::SpawnFigures(const Spawn& _spawn)
{
  ENGINE_CHECK(m_terrain != nullptr, "region {} spawns figures with no terrain", m_index);
  const MovementContext context = Movement();
  Team& team = EnsureTeam(_spawn.team);
  std::vector<std::uint32_t> slots;
  slots.reserve(_spawn.count);
  for (std::uint32_t index = 0; index < _spawn.count; ++index)
  {
    // R-SIM-226 steps 1 to 12, in order; the figure's stable id is its serial (R-SIM-108).
    Figure figure;
    figure.id = m_ids.Issue();
    SetKind(figure, _spawn.kind, *m_tuning, m_draws);
    figure.position = SpawnPosition(figure.id, _spawn.position, _spawn.spread, context);
    figure.onGround = false;
    figure.velocity = _spawn.velocity;
    figure.front = SpawnFront(figure, _spawn.velocity, *m_tuning, m_draws);
    figure.roamCenter = _spawn.position;
    figure.roamRange = _spawn.roamRange.value_or(_spawn.spread);
    figure.waypoint = figure.position; // R-UNIT-064, R-UNIT-164
    const std::uint32_t slot = team.figures.Take(figure);
    UpdateRegistration(_spawn.team, slot, team.figures.At(slot));
    slots.push_back(slot);
  }
  return slots;
}

void RegionSimulation::UpdateRegistration(std::uint8_t _team, std::uint32_t _slot, Figure& _figure)
{
  const CellRect cells = m_grid.FootprintOf(_figure.position.x, _figure.position.z, _figure.radius);
  if (cells == _figure.cells)
  {
    return;
  }
  const ActorRef ref{_figure.id, _slot, _team};
  if (!_figure.cells.Empty())
  {
    m_grid.Unregister(ref, _figure.cells);
  }
  m_grid.Register(ref, cells);
  _figure.cells = cells;
}

void RegionSimulation::Unregister(std::uint8_t _team, std::uint32_t _slot, Figure& _figure)
{
  if (!_figure.cells.Empty())
  {
    m_grid.Unregister({_figure.id, _slot, _team}, _figure.cells);
    _figure.cells = {};
  }
}

const Figure* RegionSimulation::Resolve(const ActorRef& _ref) const
{
  return ResolveIn(m_teams, _ref);
}

Figure* RegionSimulation::Resolve(const ActorRef& _ref)
{
  return ResolveIn(m_teams, _ref);
}

std::optional<ActorRef> RegionSimulation::FindFigure(std::uint8_t _team, StableId _id) const
{
  if (_team >= m_teams.size() || _id == 0)
  {
    return std::nullopt;
  }
  const SlicedPool<Figure>& figures = m_teams[_team].figures;
  for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
  {
    if (figures.Used(slot) && figures.At(slot).id == _id)
    {
      return ActorRef{_id, slot, _team};
    }
  }
  return std::nullopt;
}

void RegionSimulation::Select(std::uint8_t _team, const std::optional<ActorRef>& _leader)
{
  Team& team = EnsureTeam(_team);
  team.selectedId = _leader ? _leader->id : 0;
  team.selectedSlot = _leader ? _leader->slot : 0;
  for (std::uint32_t slot = 0; slot < team.figures.Capacity(); ++slot)
  {
    if (team.figures.Used(slot))
    {
      team.figures.At(slot).underControl = false;
    }
  }
  if (const Figure* leader = _leader ? Resolve(*_leader) : nullptr)
  {
    Emit({EffectEventKind::LeaderSelected, _team, leader->id, leader->position, {}, 0.0});
  }
}

std::optional<ActorRef> RegionSimulation::SelectedLeaderRef(std::uint8_t _team) const
{
  if (_team >= m_teams.size() || m_teams[_team].selectedId == 0)
  {
    return std::nullopt;
  }
  const ActorRef ref{m_teams[_team].selectedId, m_teams[_team].selectedSlot, _team};
  const Figure* leader = Resolve(ref);
  return leader != nullptr && leader->kind == FigureKind::Leader && !leader->dead ? std::optional<ActorRef>(ref) : std::nullopt;
}

Figure* RegionSimulation::SelectedLeader(std::uint8_t _team)
{
  const std::optional<ActorRef> ref = SelectedLeaderRef(_team);
  return ref ? Resolve(*ref) : nullptr;
}

const Figure* RegionSimulation::LeaderOf(const Figure& _worker, std::uint8_t _team) const
{
  if (_worker.leaderId == 0)
  {
    return nullptr;
  }
  const Figure* leader = Resolve({_worker.leaderId, _worker.leaderSlot, _team});
  return leader != nullptr && leader->kind == FigureKind::Leader && !leader->dead ? leader : nullptr;
}

Figure* RegionSimulation::LeaderOf(const Figure& _worker, std::uint8_t _team)
{
  if (_worker.leaderId == 0)
  {
    return nullptr;
  }
  Figure* leader = Resolve({_worker.leaderId, _worker.leaderSlot, _team});
  return leader != nullptr && leader->kind == FigureKind::Leader && !leader->dead ? leader : nullptr;
}

bool RegionSimulation::InFormation(const Figure& _worker, std::uint8_t _team) const
{
  const Figure* leader = _worker.kind == FigureKind::Worker ? LeaderOf(_worker, _team) : nullptr;
  return leader != nullptr && InFormationMode(*leader) && SlotOf(*leader, _worker.id).has_value();
}

void RegionSimulation::SetCommander(std::uint8_t _team, Neuron::SenderId _sender, bool _ai)
{
  EnsureTeam(_team);
  if (m_commanders.size() <= _team)
  {
    m_commanders.resize(std::size_t{_team} + 1);
  }
  m_commanders[_team] = {_sender, _ai};
}

std::optional<std::uint8_t> RegionSimulation::TeamCommandedBy(Neuron::SenderId _sender) const noexcept
{
  for (std::size_t team = 0; team < m_commanders.size(); ++team)
  {
    if (_sender != 0 && m_commanders[team].sender == _sender && !m_commanders[team].ai)
    {
      return static_cast<std::uint8_t>(team);
    }
  }
  return std::nullopt;
}

bool RegionSimulation::MayCommand(Neuron::SenderId _sender, std::uint8_t _team, bool _aiOnly) const noexcept
{
  if (_sender == 0 || _team >= m_commanders.size())
  {
    return false;
  }
  const Commander& commander = m_commanders[_team];
  return commander.sender == _sender && (commander.ai || !_aiOnly);
}

TeamMask RegionSimulation::FriendsOf(std::uint8_t _team) noexcept
{
  TeamMask friends;
  friends.set(_team);
  return friends;
}

TeamMask RegionSimulation::EnemiesOf(std::uint8_t _team) noexcept
{
  return ~FriendsOf(_team);
}

std::vector<ActorRef> RegionSimulation::InRange(double _x, double _z, double _range, const TeamMask& _teams) const
{
  return m_grid.InRange(_x, _z, _range, _teams, [this](const ActorRef& _ref) { return Resolve(_ref); });
}

std::optional<ActorRef> RegionSimulation::NearestEnemy(double _x, double _z, double _minRange, double _maxRange, std::uint8_t _team) const
{
  std::optional<ActorRef> nearest;
  double nearestSquared = std::numeric_limits<double>::infinity();
  const double minimumSquared = _minRange * _minRange;
  for (const ActorRef& ref : InRange(_x, _z, _maxRange, EnemiesOf(_team)))
  {
    const Figure* figure = Resolve(ref);
    if (figure == nullptr || figure->dead)
    {
      continue;
    }
    const double dx = figure->position.x - _x;
    const double dz = figure->position.z - _z;
    const double squared = dx * dx + dz * dz;
    if (minimumSquared <= squared && squared < nearestSquared)
    {
      nearest = ref;
      nearestSquared = squared;
    }
  }
  return nearest;
}

std::vector<ActorRef> RegionSimulation::CachedEnemies(double _x, double _z, double _range, std::uint8_t _team)
{
  return m_cache.CachedEnemies(_x, _z, _range, _team, m_nextTick, [&] { return InRange(_x, _z, _range, EnemiesOf(_team)); });
}

const Effect* RegionSimulation::GrenadeAt(std::uint32_t _slot, StableId _id) const
{
  if (!m_effects.Used(_slot))
  {
    return nullptr;
  }
  const Effect& effect = m_effects.At(_slot);
  return effect.id == _id && effect.kind == EffectKind::Grenade ? &effect : nullptr;
}

HealthChange RegionSimulation::Damage(const ActorRef& _ref, std::int32_t _amount, DamageKind _kind)
{
  Figure* figure = Resolve(_ref);
  if (figure == nullptr)
  {
    return {false, false};
  }
  const bool alive = !figure->dead;
  const HealthChange change = ApplyHealthChange(*figure, _amount, _kind, *m_tuning, InFormation(*figure, _ref.team));
  if (alive && _amount < 0)
  {
    Emit({EffectEventKind::FigureHurt, _ref.team, figure->id, figure->position, {}, 0.0});
  }
  if (change.killed)
  {
    Emit({EffectEventKind::FigureDied, _ref.team, figure->id, figure->position, {}, 0.0});
    // R-UNIT-038, R-ECON-067: one orb at the corpse, with half the corpse's velocity.
    SpawnOrb(*this, figure->position, figure->velocity * m_tuning->ref.econ.orb.deathVelocityFactor, _ref.team, figure->id);
  }
  return change;
}

void RegionSimulation::SetOnFire(const ActorRef& _ref)
{
  Figure* figure = Resolve(_ref);
  if (figure == nullptr || figure->kind != FigureKind::Worker || figure->dead || figure->workerState == WorkerState::Burning)
  {
    return;
  }
  figure->workerState = WorkerState::Burning;
  Emit({EffectEventKind::WorkerIgnited, _ref.team, figure->id, figure->position, {}, 0.0});
}

RouteHandle RegionSimulation::BuildRoute(const Neuron::Vec3& _from, const Neuron::Vec3& _to, bool _directRoute, bool _permitClimbing,
                                         bool _noRounding)
{
  ENGINE_CHECK(m_terrain != nullptr, "region {} builds a route with no terrain", m_index);
  return m_routes.Add(m_terrain->routes.BuildRoute(_from, _to, _directRoute, _permitClimbing, _noRounding, m_terrain->field, *m_tuning));
}

const CellRaster* RegionSimulation::TerritoryOf(std::uint8_t _site) const noexcept
{
  for (std::size_t index = 0; index < m_settlements.size() && index < m_territories.size(); ++index)
  {
    if (m_settlements[index].site == _site)
    {
      return &m_territories[index];
    }
  }
  return nullptr;
}

void RegionSimulation::RecomputeTerritory(std::uint8_t _site)
{
  for (std::size_t index = 0; index < m_settlements.size() && index < m_territories.size(); ++index)
  {
    if (m_settlements[index].site == _site)
    {
      m_territories[index] = ComputeTerritory(*this, m_settlements[index]);
    }
  }
}

const CellRaster* RegionSimulation::SeenLayerOf(std::uint8_t _site) const noexcept
{
  for (std::size_t index = 0; index < m_settlements.size() && index < m_seen.size(); ++index)
  {
    if (m_settlements[index].site == _site)
    {
      return &m_seen[index];
    }
  }
  return nullptr;
}

void RegionSimulation::RecordStatistic(const StatisticEntry& _entry)
{
  if (m_statistics.size() < STATISTICS_MAX)
  {
    m_statistics.push_back(_entry);
  }
}

void RegionSimulation::RecordContacts()
{
  for (std::size_t pair = 0; pair < VALLEY_PAIRS.size(); ++pair)
  {
    const std::uint8_t first = VALLEY_PAIRS[pair][0];
    const std::uint8_t second = VALLEY_PAIRS[pair][1];
    if (SettlementAt(first) == nullptr || SettlementAt(second) == nullptr ||
        std::ranges::any_of(m_contacts, [&](const ContactEvent& _event) { return _event.pair == pair; }))
    {
      continue;
    }
    if (ReachesHalfwayLine(*this, first, pair) && ReachesHalfwayLine(*this, second, pair))
    {
      m_contacts.push_back({m_nextTick, ContactEventKind::Contact, first, second, static_cast<std::uint8_t>(pair)});
    }
  }
}

void RegionSimulation::UpdateSeenLayers()
{
  m_seen.clear();
  if (m_terrain == nullptr)
  {
    return;
  }
  for (const Settlement& settlement : m_settlements)
  {
    m_seen.push_back(ComputeSeenLayer(*this, settlement));
  }
  ++m_seenUpdates;
}

void RegionSimulation::RecomputeTerritories()
{
  m_territories.clear();
  if (m_terrain == nullptr)
  {
    return;
  }
  for (const Settlement& settlement : m_settlements)
  {
    m_territories.push_back(ComputeTerritory(*this, settlement));
  }
}

const Settlement* RegionSimulation::SettlementAt(std::uint8_t _site) const noexcept
{
  const auto found = std::ranges::find(m_settlements, _site, &Settlement::site);
  return found == m_settlements.end() ? nullptr : &*found;
}

Structure* RegionSimulation::ResolveStructure(std::uint32_t _slot, StableId _id) noexcept
{
  return _id != 0 && m_structures.Used(_slot) && m_structures.At(_slot).id == _id ? &m_structures.At(_slot) : nullptr;
}

const Structure* RegionSimulation::ResolveStructure(std::uint32_t _slot, StableId _id) const noexcept
{
  return _id != 0 && m_structures.Used(_slot) && m_structures.At(_slot).id == _id ? &m_structures.At(_slot) : nullptr;
}

void RegionSimulation::RebuildObstacles()
{
  m_obstacles = ObstacleGrid(ObstacleGridDescOf(*m_tuning));
  const auto height = [this](double _x, double _z) { return m_terrain ? m_terrain->field.Height(_x, _z) : 0.0; };
  for (std::uint32_t slot = 0; slot < m_structures.Capacity(); ++slot)
  {
    if (!m_structures.Used(slot))
    {
      continue;
    }
    const Structure& structure = m_structures.At(slot);
    ObstacleShape shape = StructureShape(structure, m_records, *m_tuning);
    shape.structure = slot;
    static_cast<void>(m_obstacles.Add(shape, height));
  }
}

std::uint32_t RegionSimulation::AddStructure(StructureKind _kind, std::optional<std::uint8_t> _owner,
                                             std::optional<std::uint32_t> _resourceSite, double _x, double _z, const Neuron::Vec3& _front)
{
  ENGINE_CHECK(m_terrain != nullptr && m_records != nullptr, "region {} builds a structure with no terrain or no records", m_index);
  return InsertStructure(MakeStructure(_kind, _owner, _resourceSite, _x, _z, _front, RecordOf(*m_records, _kind),
                                       CollisionOf(*m_records, _kind), m_terrain->field, *m_tuning));
}

std::uint32_t RegionSimulation::InsertStructure(Structure _structure)
{
  _structure.id = m_ids.Issue();
  const std::uint32_t slot = m_structures.Take(_structure);
  RebuildObstacles(); // R-BLDG-030: the obstacle grid is updated
  return slot;
}

void RegionSimulation::PlaceResourceSites()
{
  ENGINE_CHECK(m_terrain != nullptr, "region {} places resource sites with no terrain", m_index);
  const std::vector<ResourceSitePlace>& places = m_terrain->resourceSites;
  for (std::size_t index = 0; index < places.size(); ++index)
  {
    const ResourceSitePlace& place = places[index];
    const StructureKind kind =
      place.siteClass == ResourceSiteClass::Central ? StructureKind::ResourceSiteCentral : StructureKind::ResourceSite;
    static_cast<void>(AddStructure(kind, std::nullopt, static_cast<std::uint32_t>(index), place.position.x, place.position.z, place.front));
  }
}

const Settlement& RegionSimulation::FoundSettlement(std::uint8_t _site, bool _stewarded)
{
  ENGINE_CHECK(m_terrain != nullptr && m_records != nullptr, "region {} founds a settlement with no terrain or no records", m_index);
  const RegionLayout& layout = m_terrain->layout;
  ENGINE_CHECK(_site < layout.sites.size() && SettlementAt(_site) == nullptr, "site {} of region {} is not a free site", _site, m_index);
  const Neuron::Vec3& center = layout.sites[_site];
  Settlement settlement;
  settlement.site = _site;
  settlement.team = _site;
  settlement.stewarded = _stewarded;
  settlement.cladan.color = _site;
  settlement.territoryRadius = m_tuning->wld.territory.foundingRadius;
  settlement.growthTicks = GrowthIntervalTicks(*m_tuning);
  const std::int64_t step = m_tuning->mvp.stats.populationStep;
  settlement.populationMark = step > 0 ? static_cast<std::uint32_t>(m_tuning->mvp.settlement.startWorkers / step * step) : 0;
  const auto& priority = m_tuning->mvp.priority;
  settlement.priorities = {static_cast<std::uint8_t>(std::lround(priority.defaultGather)),
                           static_cast<std::uint8_t>(std::lround(priority.defaultConstruct)),
                           static_cast<std::uint8_t>(std::lround(priority.defaultGuard))};
  const Neuron::Vec3 outward{center.x - layout.valleyCenter.x, 0.0, center.z - layout.valleyCenter.z};
  settlement.wellspringSlot = AddStructure(StructureKind::Wellspring, settlement.team, std::nullopt, center.x, center.z, outward);
  settlement.wellspringId = m_structures.At(settlement.wellspringSlot).id;
  EnsureTeam(settlement.team);
  const Settlement& founded = m_settlements.emplace_back(std::move(settlement));
  m_territories.push_back(ComputeTerritory(*this, founded));
  for (std::int64_t worker = 0; worker < m_tuning->mvp.settlement.startWorkers; ++worker)
  {
    static_cast<void>(SpawnAtDoor(founded));
  }
  return founded;
}

std::uint32_t RegionSimulation::SpawnAtDoor(const Settlement& _settlement)
{
  ENGINE_CHECK(m_records != nullptr && m_structures.Used(_settlement.wellspringSlot) &&
                 m_structures.At(_settlement.wellspringSlot).id == _settlement.wellspringId,
               "the settlement at site {} of region {} has no Wellspring", _settlement.site, m_index);
  const Structure& wellspring = m_structures.At(_settlement.wellspringSlot);
  const Neuron::SimulationRecord& record = RecordOf(*m_records, StructureKind::Wellspring);
  const std::vector<const Neuron::SimulationSocket*> doors = Neuron::SocketsOf(record, Neuron::SocketKind::Door);
  ENGINE_CHECK(!doors.empty(), "the Wellspring's record has no door socket");
  // R-ECON-043: the jitter's subject is the Worker, whose id is the region's next.
  m_draws.SetTick(m_nextTick);
  const auto& door = m_tuning->ref.econ.spawnNode;
  Neuron::SimRng& draw = m_draws.For(DrawPurpose::DoorJitter, m_ids.Next());
  const double x = draw.NextSigned(2.0 * door.doorJitter);
  const double y = draw.NextSigned(2.0 * door.doorJitter);
  const double z = draw.NextSigned(2.0 * door.doorJitter);
  Spawn spawn;
  spawn.team = _settlement.team;
  spawn.kind = FigureKind::Worker;
  spawn.position = StructurePoint(wellspring, Neuron::RestPositionOf(record, *doors.front()));
  spawn.velocity = (StructureDirection(wellspring, Neuron::RestFrontOf(record, *doors.front())) + Neuron::Vec3{x, y, z}) * door.doorSpeed;
  return SpawnFigures(spawn).front();
}

void RegionSimulation::Step(std::uint64_t _tick)
{
  ENGINE_ASSERT(_tick == m_nextTick);
  m_draws.SetTick(_tick);
  m_events.clear();
  // R-SIM-510: the tick's commands, the tick clock (the region's tick: the world has no game speed), then the slices.
  // The markers divide the step by the objects it advances, for the cost measurement (DBG-045, ADR-110).
  {
    ENGINE_PROFILE_SCOPE("sim.region.commands");
    m_commands.Execute(_tick);
  }
  const auto slices = static_cast<std::uint32_t>(m_tuning->ref.sim.updateSlices);
  for (std::uint32_t slice = 0; slice < slices; ++slice)
  {
    StepSlice(slice);
  }
  {
    ENGINE_PROFILE_SCOPE("sim.ai.step");
    for (AiController& ai : m_ais)
    {
      StepAi(*this, ai);
    }
  }
  {
    // ADR-134: each settlement's own step, in founding order.
    ENGINE_PROFILE_SCOPE("sim.settlements.advance");
    for (Settlement& settlement : m_settlements)
    {
      AdvanceSettlement(*this, settlement);
    }
    RecordContacts();
  }
  {
    ENGINE_PROFILE_SCOPE("sim.settlements.seen");
    UpdateSeenLayers();
  }
  {
    ENGINE_PROFILE_SCOPE("sim.routes.free_unheld");
    FreeUnheldRoutes();
  }
  m_nextTick = _tick + 1;
}

void RegionSimulation::FreeUnheldRoutes()
{
  // The holders the MVP builds: its figures, a Leader's being its order route, and its order markers (R-UNIT-185).
  std::vector<RouteHandle> held;
  for (const Team& team : m_teams)
  {
    for (std::uint32_t slot = 0; slot < team.figures.Capacity(); ++slot)
    {
      if (team.figures.Used(slot) && team.figures.At(slot).route != 0)
      {
        held.push_back(team.figures.At(slot).route);
      }
    }
  }
  for (std::uint32_t slot = 0; slot < m_effects.Capacity(); ++slot)
  {
    if (m_effects.Used(slot) && m_effects.At(slot).route != 0)
    {
      held.push_back(m_effects.At(slot).route);
    }
  }
  std::ranges::sort(held);
  const auto duplicates = std::ranges::unique(held);
  held.erase(duplicates.begin(), duplicates.end());
  m_routes.KeepOnly(held);
}

void RegionSimulation::StepSlice(std::uint32_t _slice)
{
  const auto slices = static_cast<std::uint32_t>(m_tuning->ref.sim.updateSlices);
  const MovementContext context = Movement();
  // R-SIM-218 step 2 and R-SIM-221 step 5: each team in index order, its slice share of ungrouped figures in slot order.
  {
    ENGINE_PROFILE_SCOPE("sim.teams.advance_ungrouped");
    AdvanceTeams(_slice, slices, context);
  }
  // R-SIM-218 step 3: the slice's laser bolts, then its effects; step 5: its Essence orbs.
  {
    ENGINE_PROFILE_SCOPE("sim.laser_bolts.advance");
    m_bolts.AdvanceSlice(_slice, slices, [&](std::uint32_t _slot) { return AdvanceLaserBolt(*this, m_bolts.At(_slot)); });
  }
  {
    ENGINE_PROFILE_SCOPE("sim.effects.advance");
    m_effects.AdvanceSlice(_slice, slices, [&](std::uint32_t _slot) { return AdvanceEffect(*this, m_effects.At(_slot)); });
  }
  // Step 4: the slice's structures; the obstacle grid follows a removal (DEC-095). The bot navigation nodes advance after
  // them, in step 4 as ADR-107 placed them.
  {
    ENGINE_PROFILE_SCOPE("sim.structures.advance");
    bool removed = false;
    m_structures.AdvanceSlice(_slice, slices,
                              [&](std::uint32_t _slot)
                              {
                                const bool remove = AdvanceStructure(*this, m_structures.At(_slot));
                                removed = removed || remove;
                                return remove;
                              });
    if (removed)
    {
      RebuildObstacles();
    }
  }
  {
    ENGINE_PROFILE_SCOPE("sim.bot_nodes.advance");
    m_botNodes.AdvanceSlice(_slice, slices,
                            [&](std::uint32_t _slot)
                            {
                              AdvanceBotNode(*this, _slot, m_botNodes.At(_slot));
                              return false;
                            });
  }
  {
    ENGINE_PROFILE_SCOPE("sim.essence_orbs.advance");
    m_orbs.AdvanceSlice(_slice, slices, [&](std::uint32_t _slot) { return AdvanceOrb(*this, m_orbs.At(_slot)); });
  }
  // R-SIM-218 step 8: the cache's eviction in its slice, after the teams.
  if (_slice == static_cast<std::uint32_t>(m_tuning->ref.sim.gridCache.evictSlice))
  {
    m_cache.Evict(m_nextTick);
  }
}

void RegionSimulation::AdvanceTeams(std::uint32_t _slice, std::uint32_t _slices, const MovementContext& _context)
{
  for (std::size_t index = 0; index < m_teams.size(); ++index)
  {
    const auto team = static_cast<std::uint8_t>(index);
    SlicedPool<Figure>& figures = m_teams[index].figures;
    figures.AdvanceSlice(_slice, _slices,
                         [&](std::uint32_t _slot)
                         {
                           Figure& figure = figures.At(_slot);
                           if (!figure.enabled)
                           {
                             Unregister(team, _slot, figure); // R-SIM-114
                             return false;
                           }
#if ENGINE_DEVELOPMENT
                           if (m_advanceObserver)
                           {
                             m_advanceObserver(team, _slot, _slice);
                           }
#endif
                           const FigureContext figureContext{this, &_context, {figure.id, _slot, team}};
                           if (AdvanceFigure(figure, figureContext))
                           {
                             Unregister(team, _slot, figure);
                             return true;
                           }
                           UpdateRegistration(team, _slot, figure);
                           return false;
                         });
  }
}

void RegionSimulation::WriteState(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(m_index, 16);
  _writer.WriteBits(m_nextTick, 64);
  _writer.WriteBits(m_ids.Next(), 64);
  _writer.WriteBounded(m_commanders.size(), TEAM_SLOTS);
  for (const Commander& commander : m_commanders)
  {
    _writer.WriteBits(commander.sender, 64);
    _writer.WriteBool(commander.ai);
  }
  _writer.WriteBounded(m_ais.size(), TEAM_SLOTS);
  for (const AiController& ai : m_ais)
  {
    WriteAi(_writer, ai);
  }
  m_commands.WriteState(_writer);
  _writer.WriteBounded(m_teams.size(), TEAM_SLOTS);
  for (const Team& team : m_teams)
  {
    team.figures.WriteState(_writer, [](Neuron::BitWriter& _out, const Figure& _figure) { WriteFigure(_out, _figure); });
    _writer.WriteBits(team.kills, 32);
    _writer.WriteBits(team.selectedId, 64);
    _writer.WriteBits(team.selectedSlot, 32);
    WriteVec3(_writer, team.pointer);
  }
  m_grid.WriteState(_writer);
  m_cache.WriteState(_writer);
  m_routes.WriteState(_writer);
  m_bolts.WriteState(_writer, [](Neuron::BitWriter& _out, const LaserBolt& _bolt) { WriteLaserBolt(_out, _bolt); });
  m_effects.WriteState(_writer, [](Neuron::BitWriter& _out, const Effect& _effect) { WriteEffect(_out, _effect); });
  m_orbs.WriteState(_writer, [](Neuron::BitWriter& _out, const EssenceOrb& _orb) { WriteOrb(_out, _orb); });
  m_botNodes.WriteState(_writer, [](Neuron::BitWriter& _out, const BotNode& _node) { WriteBotNode(_out, _node); });
  _writer.WriteBounded(m_settlements.size(), SETTLEMENTS_MAX);
  for (const Settlement& settlement : m_settlements)
  {
    WriteSettlement(_writer, settlement);
  }
  m_structures.WriteState(_writer, [](Neuron::BitWriter& _out, const Structure& _structure) { WriteStructure(_out, _structure); });
  _writer.WriteBounded(m_contacts.size(), CONTACT_EVENTS_MAX);
  for (const ContactEvent& event : m_contacts)
  {
    WriteContactEvent(_writer, event);
  }
  _writer.WriteBounded(m_statistics.size(), STATISTICS_MAX);
  for (const StatisticEntry& entry : m_statistics)
  {
    WriteStatisticEntry(_writer, entry);
  }
}

bool RegionSimulation::ReadState(Neuron::BitReader& _reader, std::uint32_t _stateVersion)
{
  const auto index = static_cast<std::uint16_t>(_reader.ReadBits(16));
  const std::uint64_t nextTick = _reader.ReadBits(64);
  const std::uint64_t nextId = _reader.ReadBits(64);
  if (_reader.Failed() || index != m_index || nextTick != m_nextTick || nextId == 0 || nextId > STABLE_ID_COUNTER_MAX + 1)
  {
    return false;
  }
  m_ids = StableIdCounter(m_ids.Owner(), nextId);
  // The commanders come before the queue, whose commands are checked again against them as they are queued.
  m_commanders.clear();
  if (_stateVersion >= STATE_VERSION_WITH_LEADERS)
  {
    const std::uint64_t commanders = _reader.ReadBounded(TEAM_SLOTS);
    for (std::uint64_t team = 0; team < commanders && !_reader.Failed(); ++team)
    {
      Commander& commander = m_commanders.emplace_back();
      commander.sender = _reader.ReadBits(64);
      commander.ai = _reader.ReadBool();
    }
  }
  m_ais.clear();
  if (_stateVersion >= STATE_VERSION_WITH_BOTS)
  {
    const std::uint64_t ais = _reader.ReadBounded(TEAM_SLOTS);
    for (std::uint64_t ai = 0; ai < ais && !_reader.Failed(); ++ai)
    {
      if (!ReadAi(_reader, m_ais.emplace_back()))
      {
        return false;
      }
    }
  }
  if (_reader.Failed() || !m_commands.ReadState(_reader))
  {
    return false;
  }
  m_teams.clear();
  m_grid = ActorGrid(ActorGridDescOf(*m_tuning));
  m_cache = ActorGridCache({m_tuning->ref.sim.gridCache.reuseFraction, static_cast<std::uint64_t>(m_tuning->ref.sim.gridCache.maxAge)});
  m_routes = RouteStore();
  m_bolts = SlicedPool<LaserBolt>(static_cast<std::uint32_t>(m_tuning->ref.sim.container.laserBoltStep));
  m_effects = SlicedPool<Effect>(static_cast<std::uint32_t>(m_tuning->ref.sim.container.effectStep));
  m_orbs = SlicedPool<EssenceOrb>(static_cast<std::uint32_t>(m_tuning->ref.sim.container.orbStep),
                                  static_cast<std::uint32_t>(m_tuning->ref.sim.container.orbPresize));
  m_botNodes = SlicedPool<BotNode>(BOT_NODE_POOL_STEP);
  m_settlements.clear();
  m_structures = SlicedPool<Structure>(static_cast<std::uint32_t>(m_tuning->ref.sim.container.structureStep));
  // ADR-099, MVP-095: a version-1 region migrates to one with no teams.
  if (_stateVersion == STATE_VERSION_WITHOUT_TEAMS)
  {
    EnsureBotNodes();
    return true;
  }
  const std::uint64_t teamCount = _reader.ReadBounded(TEAM_SLOTS);
  if (_reader.Failed())
  {
    return false;
  }
  for (std::uint64_t team = 0; team < teamCount; ++team)
  {
    Team& added = m_teams.emplace_back(Team{MakeFigurePool(*m_tuning)});
    const bool read =
      added.figures.ReadState(_reader, std::numeric_limits<std::uint32_t>::max(),
                              [&](Neuron::BitReader& _in, Figure& _figure) { return ReadFigure(_in, _figure, _stateVersion, *m_tuning); });
    if (!read)
    {
      return false;
    }
    if (_stateVersion >= STATE_VERSION_WITH_WEAPONS)
    {
      added.kills = static_cast<std::uint32_t>(_reader.ReadBits(32));
    }
    if (_stateVersion >= STATE_VERSION_WITH_LEADERS)
    {
      added.selectedId = _reader.ReadBits(64);
      added.selectedSlot = static_cast<std::uint32_t>(_reader.ReadBits(32));
      if (!ReadVec3(_reader, added.pointer))
      {
        return false;
      }
    }
  }
  if (m_commanders.size() > m_teams.size())
  {
    return false; // a commander of a team the region does not hold
  }
  const bool withGrids = _stateVersion >= STATE_VERSION_WITH_GRIDS;
  if (withGrids && (!m_grid.ReadState(_reader) || !m_cache.ReadState(_reader)))
  {
    return false;
  }
  if (_stateVersion >= STATE_VERSION_WITH_ROUTES && !m_routes.ReadState(_reader))
  {
    return false;
  }
  constexpr std::uint32_t POOL_CAPACITY_MAX = std::numeric_limits<std::uint32_t>::max();
  if (_stateVersion >= STATE_VERSION_WITH_WEAPONS &&
      (!m_bolts.ReadState(_reader, POOL_CAPACITY_MAX, [](Neuron::BitReader& _in, LaserBolt& _bolt) { return ReadLaserBolt(_in, _bolt); }) ||
       !m_effects.ReadState(_reader, POOL_CAPACITY_MAX,
                            [&](Neuron::BitReader& _in, Effect& _effect) { return ReadEffect(_in, _effect, _stateVersion); }) ||
       !m_orbs.ReadState(_reader, POOL_CAPACITY_MAX,
                         [&](Neuron::BitReader& _in, EssenceOrb& _orb) { return ReadOrb(_in, _orb, _stateVersion); })))
  {
    return false;
  }
  // The nodes' state is whole, every node in order, or none; with the terrain attached, one per node of its graph.
  if (_stateVersion >= STATE_VERSION_WITH_BOTS &&
      (!m_botNodes.ReadState(_reader, POOL_CAPACITY_MAX, [](Neuron::BitReader& _in, BotNode& _node) { return ReadBotNode(_in, _node); }) ||
       m_botNodes.Count() != m_botNodes.Capacity() || (m_terrain && m_botNodes.Count() != 0 && m_botNodes.Count() != m_bots.Size())))
  {
    return false;
  }
  // A region before version 9 has no settlement and no structure. A settlement stands on its own site and has its team
  // and its Wellspring; a structure belongs to a team the region holds, or to none.
  if (_stateVersion >= STATE_VERSION_WITH_SETTLEMENTS)
  {
    const std::uint64_t settlements = _reader.ReadBounded(SETTLEMENTS_MAX);
    for (std::uint64_t place = 0; place < settlements && !_reader.Failed(); ++place)
    {
      Settlement settlement;
      if (!ReadSettlement(_reader, settlement) || settlement.site >= SETTLEMENTS_MAX || SettlementAt(settlement.site) != nullptr ||
          settlement.team >= m_teams.size())
      {
        return false;
      }
      m_settlements.push_back(std::move(settlement));
    }
    if (_reader.Failed() || !m_structures.ReadState(_reader, POOL_CAPACITY_MAX,
                                                    [&](Neuron::BitReader& _in, Structure& _structure) {
                                                      return ReadStructure(_in, _structure) &&
                                                             _structure.owner.value_or(0) < std::max<std::size_t>(m_teams.size(), 1);
                                                    }))
    {
      return false;
    }
    for (const Settlement& settlement : m_settlements)
    {
      const std::uint32_t slot = settlement.wellspringSlot;
      if (!m_structures.Used(slot) || m_structures.At(slot).id != settlement.wellspringId ||
          m_structures.At(slot).kind != StructureKind::Wellspring)
      {
        return false;
      }
    }
    // A contact event names two settled sites of its valley border pair.
    m_contacts.resize(_reader.ReadBounded(CONTACT_EVENTS_MAX));
    for (ContactEvent& event : m_contacts)
    {
      if (!ReadContactEvent(_reader, event) || VALLEY_PAIRS[event.pair][0] != event.first || VALLEY_PAIRS[event.pair][1] != event.second ||
          SettlementAt(event.first) == nullptr || SettlementAt(event.second) == nullptr)
      {
        return false;
      }
    }
    // A statistics entry names a settled site.
    m_statistics.resize(_reader.ReadBounded(STATISTICS_MAX));
    for (StatisticEntry& entry : m_statistics)
    {
      if (!ReadStatisticEntry(_reader, entry) || SettlementAt(entry.site) == nullptr)
      {
        return false;
      }
    }
  }
  // An order marker follows a route the region holds, from its first point to its last (R-UNIT-185).
  for (std::uint32_t slot = 0; slot < m_effects.Capacity(); ++slot)
  {
    if (!m_effects.Used(slot))
    {
      continue;
    }
    const Effect& effect = m_effects.At(slot);
    const std::vector<Neuron::Vec3>* route = m_routes.Find(effect.route);
    if (effect.route == 0 ? effect.routePoint != -1
                          : route == nullptr || effect.routePoint < 0 || std::cmp_greater_equal(effect.routePoint, route->size()))
    {
      return false;
    }
  }
  // The entries follow from the figures: their recorded cells from version 4, their positions before it.
  const std::int32_t cells = m_grid.CellsPerAxis();
  for (std::size_t team = 0; team < m_teams.size(); ++team)
  {
    SlicedPool<Figure>& figures = m_teams[team].figures;
    for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
    {
      if (!figures.Used(slot))
      {
        continue;
      }
      Figure& figure = figures.At(slot);
      // A figure follows a route the region holds, from before its first point to its last, or follows none.
      const std::vector<Neuron::Vec3>* route = m_routes.Find(figure.route);
      if (figure.route == 0 ? figure.routePoint != -1
                            : route == nullptr || figure.routePoint < -1 || std::cmp_greater_equal(figure.routePoint, route->size()))
      {
        return false;
      }
      if (std::cmp_greater_equal(figure.nearestNode, m_botNodes.Capacity()))
      {
        return false; // a nearest node the region does not hold
      }
      if (!withGrids && figure.enabled)
      {
        figure.cells = m_grid.SquareOf(figure.position.x, figure.position.z, figure.radius * m_tuning->ref.sim.actorGrid.footprintFactor);
      }
      if (figure.cells.Empty())
      {
        continue;
      }
      if (figure.cells.x0 < 0 || figure.cells.z0 < 0 || figure.cells.x1 >= cells || figure.cells.z1 >= cells)
      {
        return false;
      }
      m_grid.Register({figure.id, slot, static_cast<std::uint8_t>(team)}, figure.cells);
    }
  }
  EnsureBotNodes(); // MVP-095: a region saved before M1 task T8 makes its nodes as a new one does
  if (m_terrain && m_structures.Count() != 0)
  {
    RebuildObstacles();
  }
  RecomputeTerritories();
  return true;
}

} // namespace Species
