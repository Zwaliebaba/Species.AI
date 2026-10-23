#include "pch.h"

#include "StewardAreas.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <span>
#include <utility>
#include <variant>

namespace Species
{

namespace
{

// The squared distance, in cells², of a cell with no territory cell in its line: beyond any raster, and finite, so that
// the transform's sums stay numbers.
constexpr double FAR_SQUARED = 1e18;

// Felzenszwalb and Huttenlocher's exact squared Euclidean distance transform of one line of cells: _out[q] is the least
// (q − p)² + _in[p] over every p, from the lower envelope of the parabolas rooted at each p.
void DistanceLine(std::span<const double> _in, std::span<double> _out, std::vector<std::int32_t>& _roots, std::vector<double>& _bounds)
{
  const auto count = static_cast<std::int32_t>(_in.size());
  _roots.assign(_in.size(), 0);
  _bounds.assign(_in.size() + 1, 0.0);
  std::int32_t k = 0;
  _bounds[0] = -std::numeric_limits<double>::infinity();
  _bounds[1] = std::numeric_limits<double>::infinity();
  for (std::int32_t q = 1; q < count; ++q)
  {
    const double rootQ = _in[static_cast<std::size_t>(q)] + static_cast<double>(q) * static_cast<double>(q);
    double meet = 0.0;
    while (true)
    {
      const std::int32_t p = _roots[static_cast<std::size_t>(k)];
      meet =
        (rootQ - (_in[static_cast<std::size_t>(p)] + static_cast<double>(p) * static_cast<double>(p))) / (2.0 * static_cast<double>(q - p));
      if (meet > _bounds[static_cast<std::size_t>(k)] || k == 0)
      {
        break;
      }
      --k;
    }
    ++k;
    _roots[static_cast<std::size_t>(k)] = q;
    _bounds[static_cast<std::size_t>(k)] = meet;
    _bounds[static_cast<std::size_t>(k) + 1] = std::numeric_limits<double>::infinity();
  }
  k = 0;
  for (std::int32_t q = 0; q < count; ++q)
  {
    while (_bounds[static_cast<std::size_t>(k) + 1] < static_cast<double>(q))
    {
      ++k;
    }
    const std::int32_t p = _roots[static_cast<std::size_t>(k)];
    _out[static_cast<std::size_t>(q)] = static_cast<double>(q - p) * static_cast<double>(q - p) + _in[static_cast<std::size_t>(p)];
  }
}

} // namespace

void StewardAreas::Update(const StateReplica::View& _latest, std::optional<std::uint8_t> _team, const RegionLayout& _layout,
                          const TuningMvp& _tuning, std::span<const Field> _fields)
{
  std::optional<std::uint8_t> site;
  if (_team.has_value())
  {
    const auto last = _latest.upper_bound(SettlementObjectId(std::numeric_limits<std::uint8_t>::max()));
    for (auto it = _latest.lower_bound(SettlementObjectId(0)); it != last; ++it)
    {
      if (const auto* settlement = std::get_if<ReplicatedSettlement>(&it->second); settlement != nullptr && settlement->team == *_team)
      {
        site = settlement->site;
        break;
      }
    }
  }
  std::vector<ReplicatedCells> chunks;
  if (site.has_value())
  {
    constexpr std::uint8_t LAST_CHUNK = std::numeric_limits<std::uint8_t>::max();
    const auto last = _latest.upper_bound(CellChunkObjectId(CellLayer::Territory, *site, LAST_CHUNK, LAST_CHUNK));
    for (auto it = _latest.lower_bound(CellChunkObjectId(CellLayer::Territory, *site, 0, 0)); it != last; ++it)
    {
      if (const auto* cells = std::get_if<ReplicatedCells>(&it->second); cells != nullptr)
      {
        chunks.push_back(*cells);
      }
    }
  }
  if (chunks.empty() || !site.has_value())
  {
    Clear();
    return;
  }
  if (chunks == m_chunks && site.value() == m_site && std::ranges::equal(_fields, m_fields))
  {
    return;
  }
  m_chunks = std::move(chunks);
  m_fields.assign(_fields.begin(), _fields.end());
  m_site = site.value();
  Rebuild(_layout, _tuning);
  ++m_revision;
}

void StewardAreas::Clear() noexcept
{
  if (!Known())
  {
    return;
  }
  m_chunks.clear();
  m_fields.clear();
  m_territory.clear();
  m_inside.clear();
  m_cellsPerAxis = 0;
  ++m_revision;
}

void StewardAreas::Rebuild(const RegionLayout& _layout, const TuningMvp& _tuning)
{
  // The region's raster of ADR-139, widened by the margin's cells on each side.
  m_cellSize = _tuning.mvp.territory.cellSize;
  const double margin = _tuning.wld.view.settlementMargin;
  const auto regionCells = static_cast<std::int32_t>(std::ceil(_layout.side / m_cellSize));
  m_padCells = static_cast<std::int32_t>(std::ceil(margin / m_cellSize));
  m_cellsPerAxis = regionCells + 2 * m_padCells;
  m_originX = _layout.originX - m_padCells * m_cellSize;
  m_originZ = _layout.originZ - m_padCells * m_cellSize;
  const std::size_t cells = static_cast<std::size_t>(m_cellsPerAxis) * static_cast<std::size_t>(m_cellsPerAxis);
  m_territory.assign(cells, 0);
  m_inside.assign(cells, 0);
  std::int32_t minX = m_cellsPerAxis;
  std::int32_t minZ = m_cellsPerAxis;
  std::int32_t maxX = -1;
  std::int32_t maxZ = -1;
  for (const ReplicatedCells& chunk : m_chunks)
  {
    for (std::int32_t z = 0; z < CELL_CHUNK_SIDE; ++z)
    {
      for (std::int32_t x = 0; x < CELL_CHUNK_SIDE; ++x)
      {
        const std::int32_t cellX = chunk.chunkX * CELL_CHUNK_SIDE + x;
        const std::int32_t cellZ = chunk.chunkZ * CELL_CHUNK_SIDE + z;
        if (((chunk.cells >> (z * CELL_CHUNK_SIDE + x)) & 1u) != 0 && cellX < regionCells && cellZ < regionCells)
        {
          m_territory[Index(cellX + m_padCells, cellZ + m_padCells)] = 1;
          minX = std::min(minX, cellX);
          minZ = std::min(minZ, cellZ);
          maxX = std::max(maxX, cellX + 2 * m_padCells);
          maxZ = std::max(maxZ, cellZ + 2 * m_padCells);
        }
      }
    }
  }
  if (maxX >= 0)
  {
    MarkNearTerritory(minX, minZ, maxX, maxZ, margin);
  }
  // WLD-244 (ADR-167): each battlefield's cells, whose centres lie within its radius of its centre.
  const auto cellAt = [this](double _coordinate, double _origin)
  {
    return static_cast<std::int32_t>(
      std::clamp(std::floor((_coordinate - _origin) / m_cellSize), 0.0, static_cast<double>(m_cellsPerAxis) - 1.0));
  };
  for (const Field& field : m_fields)
  {
    for (std::int32_t z = cellAt(field.z - field.radius, m_originZ); z <= cellAt(field.z + field.radius, m_originZ); ++z)
    {
      for (std::int32_t x = cellAt(field.x - field.radius, m_originX); x <= cellAt(field.x + field.radius, m_originX); ++x)
      {
        const double dx = m_originX + (x + 0.5) * m_cellSize - field.x;
        const double dz = m_originZ + (z + 0.5) * m_cellSize - field.z;
        m_inside[Index(x, z)] = dx * dx + dz * dz <= field.radius * field.radius ? 1 : m_inside[Index(x, z)];
      }
    }
  }
}

void StewardAreas::MarkNearTerritory(std::int32_t _minX, std::int32_t _minZ, std::int32_t _maxX, std::int32_t _maxZ, double _margin)
{
  // The exact distance of each cell's centre from the nearest territory cell's centre, along the rows and then along the
  // columns, in cells², over the territory's box widened by the margin, beyond which no cell is near enough (ADR-142).
  const auto boxX = static_cast<std::size_t>(_minX);
  const auto boxZ = static_cast<std::size_t>(_minZ);
  const std::size_t width = static_cast<std::size_t>(_maxX) - boxX + 1;
  const std::size_t depth = static_cast<std::size_t>(_maxZ) - boxZ + 1;
  const auto side = static_cast<std::size_t>(m_cellsPerAxis);
  std::vector<double> distance(width * depth);
  for (std::size_t z = 0; z < depth; ++z)
  {
    for (std::size_t x = 0; x < width; ++x)
    {
      distance[z * width + x] = m_territory[(boxZ + z) * side + boxX + x] != 0 ? 0.0 : FAR_SQUARED;
    }
  }
  std::vector<double> line(std::max(width, depth));
  std::vector<double> result(depth);
  std::vector<std::int32_t> roots;
  std::vector<double> bounds;
  for (std::size_t z = 0; z < depth; ++z)
  {
    const std::span<double> row(distance.data() + z * width, width);
    std::copy(row.begin(), row.end(), line.begin());
    DistanceLine(std::span<const double>(line.data(), width), row, roots, bounds);
  }
  for (std::size_t x = 0; x < width; ++x)
  {
    for (std::size_t z = 0; z < depth; ++z)
    {
      line[z] = distance[z * width + x];
    }
    DistanceLine(std::span<const double>(line.data(), depth), result, roots, bounds);
    for (std::size_t z = 0; z < depth; ++z)
    {
      m_inside[(boxZ + z) * side + boxX + x] = result[z] * m_cellSize * m_cellSize <= _margin * _margin ? 1 : 0;
    }
  }
}

std::int32_t StewardAreas::CellOf(double _coordinate, double _origin) const noexcept
{
  const double cell = std::floor((_coordinate - _origin) / m_cellSize);
  return cell < 0.0 || cell >= static_cast<double>(m_cellsPerAxis) ? -1 : static_cast<std::int32_t>(cell);
}

std::size_t StewardAreas::Index(std::int32_t _x, std::int32_t _z) const noexcept
{
  return static_cast<std::size_t>(_z) * static_cast<std::size_t>(m_cellsPerAxis) + static_cast<std::size_t>(_x);
}

bool StewardAreas::Contains(double _x, double _z) const noexcept
{
  if (!Known())
  {
    return true;
  }
  const std::int32_t x = CellOf(_x, m_originX);
  const std::int32_t z = CellOf(_z, m_originZ);
  return x >= 0 && z >= 0 && m_inside[Index(x, z)] != 0;
}

bool StewardAreas::InTerritory(double _x, double _z) const noexcept
{
  if (!Known())
  {
    return false;
  }
  const std::int32_t x = CellOf(_x, m_originX);
  const std::int32_t z = CellOf(_z, m_originZ);
  return x >= 0 && z >= 0 && m_territory[Index(x, z)] != 0;
}

std::optional<StewardAreas::Point> StewardAreas::Nearest(double _x, double _z) const
{
  if (!Known())
  {
    return std::nullopt;
  }
  if (Contains(_x, _z))
  {
    return Point{_x, _z};
  }
  // A point outside is rare (ADR-142), so every cell of the areas is tried.
  std::optional<Point> nearest;
  double best = std::numeric_limits<double>::infinity();
  for (std::int32_t z = 0; z < m_cellsPerAxis; ++z)
  {
    for (std::int32_t x = 0; x < m_cellsPerAxis; ++x)
    {
      if (m_inside[Index(x, z)] == 0)
      {
        continue;
      }
      const Point center{m_originX + (x + 0.5) * m_cellSize, m_originZ + (z + 0.5) * m_cellSize};
      const double squared = (center.x - _x) * (center.x - _x) + (center.z - _z) * (center.z - _z);
      if (squared < best)
      {
        best = squared;
        nearest = center;
      }
    }
  }
  return nearest;
}

std::shared_ptr<const Neuron::ViewMask> BuildViewMask(const StewardAreas& _areas, const TuningPresentation& _presentation)
{
  if (!_areas.Known())
  {
    return nullptr;
  }
  constexpr std::uint32_t LEVELS = 255;
  auto mask = std::make_shared<Neuron::ViewMask>();
  const StewardAreas::Point origin = _areas.Origin();
  mask->originX = origin.x;
  mask->originZ = origin.z;
  mask->cellSize = _areas.CellSize();
  const auto side = static_cast<std::size_t>(_areas.CellsPerAxis());
  mask->side = static_cast<std::uint32_t>(side);
  mask->fogStrength = static_cast<float>(_presentation.mvp.viewFog.strength);
  mask->cells.assign(side * side, 0);
  // The box of the areas' cells; nothing beyond it, widened by the blur, is clear.
  const std::vector<std::uint8_t>& inside = _areas.InsideCells();
  std::size_t minX = side;
  std::size_t minZ = side;
  std::size_t maxX = 0;
  std::size_t maxZ = 0;
  for (std::size_t z = 0; z < side; ++z)
  {
    for (std::size_t x = 0; x < side; ++x)
    {
      if (inside[z * side + x] != 0)
      {
        minX = std::min(minX, x);
        maxX = std::max(maxX, x);
        minZ = std::min(minZ, z);
        maxZ = std::max(maxZ, z);
      }
    }
  }
  if (minX > maxX)
  {
    return mask;
  }
  // A box blur of the edge width, the count of cells in the areas along each row's box and then along each column's,
  // so the fog's edge is a ramp centred on the areas' edge rather than a cell's step.
  const auto radius = static_cast<std::size_t>(std::max(0.0, std::round(_presentation.mvp.viewFog.edgeWidth / (2.0 * mask->cellSize))));
  const std::size_t width = 2 * radius + 1;
  const std::size_t firstX = minX > radius ? minX - radius : 0;
  const std::size_t lastX = std::min(maxX + radius, side - 1);
  const std::size_t firstZ = minZ > radius ? minZ - radius : 0;
  const std::size_t lastZ = std::min(maxZ + radius, side - 1);
  const auto count = [&](std::size_t _from, std::size_t _to, std::size_t _k, const auto& _at)
  {
    std::uint32_t sum = 0;
    for (std::size_t j = (_k > _from + radius ? _k - radius : _from); j <= std::min(_k + radius, _to); ++j)
    {
      sum += _at(j);
    }
    return sum;
  };
  std::vector<std::uint32_t> rows(side * side, 0);
  for (std::size_t z = minZ; z <= maxZ; ++z)
  {
    const auto at = [&](std::size_t _x) { return std::uint32_t{inside[z * side + _x]}; };
    std::uint32_t sum = count(firstX, lastX, firstX, at);
    for (std::size_t x = firstX; x <= lastX; ++x)
    {
      rows[z * side + x] = sum;
      if (x + radius + 1 <= lastX)
      {
        sum += at(x + radius + 1);
      }
      if (x >= firstX + radius)
      {
        sum -= at(x - radius);
      }
    }
  }
  const std::uint32_t area = static_cast<std::uint32_t>(width * width);
  for (std::size_t x = firstX; x <= lastX; ++x)
  {
    const auto at = [&](std::size_t _z) { return rows[_z * side + x]; };
    std::uint32_t sum = count(firstZ, lastZ, firstZ, at);
    for (std::size_t z = firstZ; z <= lastZ; ++z)
    {
      mask->cells[z * side + x] = static_cast<std::uint8_t>((sum * LEVELS + area / 2) / area);
      if (z + radius + 1 <= lastZ)
      {
        sum += at(z + radius + 1);
      }
      if (z >= firstZ + radius)
      {
        sum -= at(z - radius);
      }
    }
  }
  return mask;
}

} // namespace Species
