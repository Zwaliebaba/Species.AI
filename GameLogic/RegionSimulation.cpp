#include "pch.h"

#include "Check.h"
#include "FigureAdvance.h"
#include "RegionSimulation.h"

#include <limits>
#include <utility>

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

// The first state version whose regions hold the actor grid's capacities and the cache (M1 task T4).
constexpr std::uint32_t STATE_VERSION_WITH_GRIDS = 4;

} // namespace

RegionSimulation::RegionSimulation(std::uint16_t _index, const Neuron::SipHashKey& _simulationKey, std::uint64_t _nextTick,
                                   const TuningMvp& _tuning, std::shared_ptr<const RegionTerrain> _terrain)
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
    m_obstacles(ObstacleGridDescOf(_tuning))
{
  ENGINE_CHECK(_tuning.ref.sim.updateSlices > 0 && _tuning.ref.sim.container.ungroupedStep > 0);
  m_draws.SetTick(_nextTick);
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

void RegionSimulation::Step(std::uint64_t _tick)
{
  ENGINE_ASSERT(_tick == m_nextTick);
  m_draws.SetTick(_tick);
  // R-SIM-510: the tick's commands, the tick clock (the region's tick: the world has no game speed), then the slices.
  m_commands.Execute(_tick);
  const auto slices = static_cast<std::uint32_t>(m_tuning->ref.sim.updateSlices);
  for (std::uint32_t slice = 0; slice < slices; ++slice)
  {
    StepSlice(slice);
  }
  m_nextTick = _tick + 1;
}

void RegionSimulation::StepSlice(std::uint32_t _slice)
{
  const auto slices = static_cast<std::uint32_t>(m_tuning->ref.sim.updateSlices);
  const MovementContext context = Movement();
  // R-SIM-218 step 2 and R-SIM-221 step 5: each team in index order, its slice share of ungrouped figures in slot order.
  for (std::size_t index = 0; index < m_teams.size(); ++index)
  {
    const auto team = static_cast<std::uint8_t>(index);
    SlicedPool<Figure>& figures = m_teams[index].figures;
    figures.AdvanceSlice(_slice, slices,
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
                           if (AdvanceFigure(figure, context))
                           {
                             Unregister(team, _slot, figure);
                             return true;
                           }
                           UpdateRegistration(team, _slot, figure);
                           return false;
                         });
  }
  // R-SIM-218 step 8: the cache's eviction in its slice, after the teams.
  if (_slice == static_cast<std::uint32_t>(m_tuning->ref.sim.gridCache.evictSlice))
  {
    m_cache.Evict(m_nextTick);
  }
}

void RegionSimulation::WriteState(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(m_index, 16);
  _writer.WriteBits(m_nextTick, 64);
  _writer.WriteBits(m_ids.Next(), 64);
  m_commands.WriteState(_writer);
  _writer.WriteBounded(m_teams.size(), TEAM_SLOTS);
  for (const Team& team : m_teams)
  {
    team.figures.WriteState(_writer, [](Neuron::BitWriter& _out, const Figure& _figure) { WriteFigure(_out, _figure); });
  }
  m_grid.WriteState(_writer);
  m_cache.WriteState(_writer);
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
  if (!m_commands.ReadState(_reader))
  {
    return false;
  }
  m_teams.clear();
  m_grid = ActorGrid(ActorGridDescOf(*m_tuning));
  m_cache = ActorGridCache({m_tuning->ref.sim.gridCache.reuseFraction, static_cast<std::uint64_t>(m_tuning->ref.sim.gridCache.maxAge)});
  // ADR-099, MVP-095: a version-1 region migrates to one with no teams.
  if (_stateVersion == STATE_VERSION_WITHOUT_TEAMS)
  {
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
  }
  const bool withGrids = _stateVersion >= STATE_VERSION_WITH_GRIDS;
  if (withGrids && (!m_grid.ReadState(_reader) || !m_cache.ReadState(_reader)))
  {
    return false;
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
  return true;
}

} // namespace Species
