#include "pch.h"

#include "Check.h"
#include "RegionSimulation.h"

#include <limits>

namespace Species
{

namespace
{

SlicedPool<Figure> MakeFigurePool(const TuningMvp& _tuning)
{
  return SlicedPool<Figure>(static_cast<std::uint32_t>(_tuning.ref.sim.container.ungroupedStep));
}

} // namespace

RegionSimulation::RegionSimulation(std::uint16_t _index, const Neuron::SipHashKey& _simulationKey, std::uint64_t _nextTick,
                                   const TuningMvp& _tuning)
  : m_index(_index),
    m_tuning(&_tuning),
    m_key(_simulationKey),
    m_ids(static_cast<OwnerId>(REGION_OWNER_ID_FIRST + _index)),
    m_commands(_simulationKey, _nextTick),
    m_nextTick(_nextTick)
{
  ENGINE_CHECK(_tuning.ref.sim.updateSlices > 0 && _tuning.ref.sim.container.ungroupedStep > 0);
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

DrawScope RegionSimulation::Draws() noexcept
{
#if ENGINE_DEVELOPMENT
  return {&m_key, m_nextTick, &m_ledger};
#else
  return {&m_key, m_nextTick, nullptr};
#endif
}

std::vector<std::uint32_t> RegionSimulation::SpawnFigures(const Spawn& _spawn)
{
  Team& team = EnsureTeam(_spawn.team);
  std::vector<std::uint32_t> slots;
  slots.reserve(_spawn.count);
  for (std::uint32_t index = 0; index < _spawn.count; ++index)
  {
    // R-SIM-226 steps 1 to 10, in order; the figure's stable id is its serial (R-SIM-108).
    Figure figure;
    figure.id = m_ids.Issue();
    SetKind(figure, _spawn.kind, *m_tuning, Draws());
    figure.position = _spawn.position;
    figure.onGround = false;
    figure.velocity = _spawn.velocity;
    figure.front = SpawnFront(figure, _spawn.velocity, *m_tuning, Draws());
    figure.roamCenter = _spawn.position;
    figure.roamRange = _spawn.roamRange;
    slots.push_back(team.figures.Take(figure));
  }
  return slots;
}

void RegionSimulation::Step(std::uint64_t _tick)
{
  ENGINE_ASSERT(_tick == m_nextTick);
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
  // R-SIM-218 step 2 and R-SIM-221 step 5: each team in index order, its slice share of ungrouped figures in slot order.
  for (std::size_t index = 0; index < m_teams.size(); ++index)
  {
    SlicedPool<Figure>& figures = m_teams[index].figures;
    figures.AdvanceSlice(_slice, slices,
                         [&](std::uint32_t _slot)
                         {
                           Figure& figure = figures.At(_slot);
                           if (!figure.enabled)
                           {
                             return false;
                           }
#if ENGINE_DEVELOPMENT
                           if (m_advanceObserver)
                           {
                             m_advanceObserver(static_cast<std::uint8_t>(index), _slot, _slice);
                           }
#endif
                           return BaseAdvance(figure, *m_tuning);
                         });
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
    const bool read = added.figures.ReadState(_reader, std::numeric_limits<std::uint32_t>::max(),
                                              [](Neuron::BitReader& _in, Figure& _figure) { return ReadFigure(_in, _figure); });
    if (!read)
    {
      return false;
    }
  }
  return true;
}

} // namespace Species
