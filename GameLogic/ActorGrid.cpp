#include "pch.h"

#include "ActorGrid.h"
#include "Check.h"

#include <cmath>

namespace Species
{

ActorGrid::ActorGrid(const Desc& _desc)
  : m_desc(_desc),
    m_cellsPerAxis(static_cast<std::int32_t>(std::trunc(_desc.worldSize / _desc.cellSize)) + _desc.extraCells)
{
  ENGINE_CHECK(_desc.cellSize > 0.0 && m_cellsPerAxis > 0 && _desc.firstCapacity > 0 && _desc.growthFactor > 1);
}

std::int32_t ActorGrid::ClampedCellOf(double _coordinate, double _origin) const noexcept
{
  // R-SIM-305: truncation toward zero (DEC-241 item 1), then the clamp of the footprint and query squares.
  const double cell = std::trunc((_coordinate - _origin) / m_desc.cellSize);
  return static_cast<std::int32_t>(std::clamp(cell, 0.0, static_cast<double>(m_cellsPerAxis - 1)));
}

CellRect ActorGrid::FootprintOf(double _x, double _z, double _radius) const noexcept
{
  if (_radius <= 0.0)
  {
    // R-SIM-306: an on-map position, which the world clamp keeps every figure at.
    ENGINE_ASSERT(_x >= m_desc.originX && _x <= m_desc.originX + m_desc.worldSize && _z >= m_desc.originZ &&
                  _z <= m_desc.originZ + m_desc.worldSize);
    const std::int32_t cellX = ClampedCellOf(_x, m_desc.originX);
    const std::int32_t cellZ = ClampedCellOf(_z, m_desc.originZ);
    return {cellX, cellZ, cellX, cellZ};
  }
  return SquareOf(_x, _z, m_desc.footprintFactor * _radius);
}

CellRect ActorGrid::SquareOf(double _x, double _z, double _half) const noexcept
{
  return {ClampedCellOf(_x - _half, m_desc.originX), ClampedCellOf(_z - _half, m_desc.originZ), ClampedCellOf(_x + _half, m_desc.originX),
          ClampedCellOf(_z + _half, m_desc.originZ)};
}

ActorGrid::Layer& ActorGrid::LayerOf(std::uint8_t _team)
{
  if (m_layers.size() <= _team)
  {
    m_layers.resize(std::size_t{_team} + 1);
  }
  Layer& layer = m_layers[_team];
  if (layer.empty())
  {
    layer.resize(static_cast<std::size_t>(m_cellsPerAxis) * static_cast<std::size_t>(m_cellsPerAxis), Cell{{}, m_desc.initialCapacity});
  }
  return layer;
}

void ActorGrid::Register(const ActorRef& _ref, const CellRect& _cells)
{
  Layer& layer = LayerOf(_ref.team);
  for (std::int32_t cellX = _cells.x0; cellX <= _cells.x1; ++cellX)
  {
    for (std::int32_t cellZ = _cells.z0; cellZ <= _cells.z1; ++cellZ)
    {
      Cell& cell = layer[IndexOf(cellX, cellZ)];
      if (cell.entries.size() >= cell.capacity)
      {
        // R-SIM-307 item 1: 0 grows to the first capacity, and every later growth multiplies it.
        cell.capacity = cell.capacity == 0 ? m_desc.firstCapacity : cell.capacity * m_desc.growthFactor;
      }
      const auto at = std::ranges::lower_bound(cell.entries, _ref.id, {}, &ActorRef::id);
      cell.entries.insert(at, _ref);
    }
  }
}

void ActorGrid::Unregister(const ActorRef& _ref, const CellRect& _cells)
{
  Layer& layer = LayerOf(_ref.team);
  for (std::int32_t cellX = _cells.x0; cellX <= _cells.x1; ++cellX)
  {
    for (std::int32_t cellZ = _cells.z0; cellZ <= _cells.z1; ++cellZ)
    {
      // R-SIM-307 item 3: every entry of the handle; an unregistration that finds none is a defect (R-SIM-308).
      std::vector<ActorRef>& entries = layer[IndexOf(cellX, cellZ)].entries;
      const auto removed = std::erase(entries, _ref);
      ENGINE_ASSERT(removed > 0);
    }
  }
}

std::uint32_t ActorGrid::CountInCells(double _x, double _z, double _range, const TeamMask& _teams) const
{
  std::unordered_set<StableId> counted;
  ForEachCell(_x, _z, _range, _teams,
              [&counted](const Cell& _cell)
              {
                for (const ActorRef& entry : _cell.entries)
                {
                  counted.insert(entry.id);
                }
              });
  return static_cast<std::uint32_t>(counted.size());
}

bool ActorGrid::AnyInCells(double _x, double _z, double _range, const TeamMask& _teams) const
{
  bool any = false;
  ForEachCell(_x, _z, _range, _teams, [&any](const Cell& _cell) { any = any || _cell.capacity > 0; });
  return any;
}

std::uint32_t ActorGrid::CapacityAt(std::uint8_t _team, std::int32_t _cellX, std::int32_t _cellZ) const noexcept
{
  if (_team >= m_layers.size() || m_layers[_team].empty())
  {
    return m_desc.initialCapacity;
  }
  return m_layers[_team][IndexOf(_cellX, _cellZ)].capacity;
}

std::vector<ActorRef> ActorGrid::EntriesAt(std::uint8_t _team, std::int32_t _cellX, std::int32_t _cellZ) const
{
  if (_team >= m_layers.size() || m_layers[_team].empty())
  {
    return {};
  }
  return m_layers[_team][IndexOf(_cellX, _cellZ)].entries;
}

void ActorGrid::WriteState(Neuron::BitWriter& _writer) const
{
  const std::size_t cells = static_cast<std::size_t>(m_cellsPerAxis) * static_cast<std::size_t>(m_cellsPerAxis);
  _writer.WriteBounded(m_layers.size(), TEAM_SLOTS_MAX);
  for (const Layer& layer : m_layers)
  {
    const auto grown = static_cast<std::size_t>(
      std::ranges::count_if(layer, [this](const Cell& _cell) { return _cell.capacity != m_desc.initialCapacity; }));
    _writer.WriteBounded(grown, cells);
    for (std::size_t index = 0; index < layer.size(); ++index)
    {
      if (layer[index].capacity != m_desc.initialCapacity)
      {
        _writer.WriteBounded(index, cells - 1);
        _writer.WriteBits(layer[index].capacity, 32);
      }
    }
  }
}

bool ActorGrid::ReadState(Neuron::BitReader& _reader)
{
  const std::size_t cells = static_cast<std::size_t>(m_cellsPerAxis) * static_cast<std::size_t>(m_cellsPerAxis);
  m_layers.clear();
  const std::uint64_t layers = _reader.ReadBounded(TEAM_SLOTS_MAX);
  if (_reader.Failed())
  {
    return false;
  }
  for (std::uint64_t team = 0; team < layers; ++team)
  {
    const std::uint64_t grown = _reader.ReadBounded(cells);
    if (_reader.Failed())
    {
      return false;
    }
    if (grown == 0)
    {
      m_layers.emplace_back();
      continue;
    }
    Layer& layer = LayerOf(static_cast<std::uint8_t>(team));
    for (std::uint64_t n = 0; n < grown; ++n)
    {
      const std::uint64_t index = _reader.ReadBounded(cells - 1);
      const auto capacity = static_cast<std::uint32_t>(_reader.ReadBits(32));
      if (_reader.Failed())
      {
        return false;
      }
      layer[index].capacity = capacity;
    }
  }
  return true;
}

} // namespace Species
