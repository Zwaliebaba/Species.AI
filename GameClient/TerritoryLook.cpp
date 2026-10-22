#include "pch.h"

#include "CrowdLook.h"
#include "DevelopmentOverlay.h"
#include "Geometry.h"
#include "TerritoryLook.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <limits>
#include <string_view>
#include <utility>
#include <variant>

namespace Species
{

namespace
{

// The catalogue entry of the effects of pass 10a (ADR-123, ADR-143): additive, depth-tested and read through the mip
// chain, so a far line neither shows through a hill nor shimmers.
constexpr std::string_view EFFECT_ADDITIVE = "effect_additive";
constexpr std::uint8_t LAST_ID_BYTE = std::numeric_limits<std::uint8_t>::max();

// Marching squares over the cell centres: the edge midpoints, in cells from the square's lower corner (x, z), and for
// each case of the corners held, bit 0 at (x, z), bit 1 at (x + 1, z), bit 2 at (x + 1, z + 1) and bit 3 at (x, z + 1),
// up to two pieces between them, NONE for none. Two diagonal corners are taken as apart.
constexpr std::int8_t SOUTH = 0;
constexpr std::int8_t EAST = 1;
constexpr std::int8_t NORTH = 2;
constexpr std::int8_t WEST = 3;
constexpr std::int8_t NONE = -1;
constexpr std::array<std::array<double, 2>, 4> EDGE_POINTS{{{1.0, 0.5}, {1.5, 1.0}, {1.0, 1.5}, {0.5, 1.0}}};
constexpr std::array<std::array<std::int8_t, 4>, 16> CASES{{{NONE, NONE, NONE, NONE},
                                                            {WEST, SOUTH, NONE, NONE},
                                                            {SOUTH, EAST, NONE, NONE},
                                                            {WEST, EAST, NONE, NONE},
                                                            {EAST, NORTH, NONE, NONE},
                                                            {WEST, SOUTH, EAST, NORTH},
                                                            {SOUTH, NORTH, NONE, NONE},
                                                            {WEST, NORTH, NONE, NONE},
                                                            {NORTH, WEST, NONE, NONE},
                                                            {SOUTH, NORTH, NONE, NONE},
                                                            {SOUTH, EAST, NORTH, WEST},
                                                            {EAST, NORTH, NONE, NONE},
                                                            {WEST, EAST, NONE, NONE},
                                                            {SOUTH, EAST, NONE, NONE},
                                                            {WEST, SOUTH, NONE, NONE},
                                                            {NONE, NONE, NONE, NONE}}};

// One settlement's territory cells over its region's square, from its chunks (ADR-139), and the box that holds them.
struct TerritoryCells
{
  std::int32_t side = 0;
  std::vector<std::uint8_t> held;
  std::int32_t minX = std::numeric_limits<std::int32_t>::max();
  std::int32_t minZ = std::numeric_limits<std::int32_t>::max();
  std::int32_t maxX = -1;
  std::int32_t maxZ = -1;

  [[nodiscard]] bool Held(std::int32_t _x, std::int32_t _z) const noexcept
  {
    return _x >= 0 && _z >= 0 && _x < side && _z < side &&
           held[static_cast<std::size_t>(_z) * static_cast<std::size_t>(side) + static_cast<std::size_t>(_x)] != 0;
  }
};

TerritoryCells CellsOf(const std::vector<ReplicatedCells>& _chunks, std::int32_t _cellsPerAxis)
{
  TerritoryCells cells;
  cells.side = _cellsPerAxis;
  cells.held.assign(static_cast<std::size_t>(_cellsPerAxis) * static_cast<std::size_t>(_cellsPerAxis), 0);
  for (const ReplicatedCells& chunk : _chunks)
  {
    for (std::int32_t z = 0; z < CELL_CHUNK_SIDE; ++z)
    {
      for (std::int32_t x = 0; x < CELL_CHUNK_SIDE; ++x)
      {
        const std::int32_t cellX = chunk.chunkX * CELL_CHUNK_SIDE + x;
        const std::int32_t cellZ = chunk.chunkZ * CELL_CHUNK_SIDE + z;
        if (((chunk.cells >> (z * CELL_CHUNK_SIDE + x)) & 1u) != 0 && cellX < cells.side && cellZ < cells.side)
        {
          cells.held[static_cast<std::size_t>(cellZ) * static_cast<std::size_t>(cells.side) + static_cast<std::size_t>(cellX)] = 1;
          cells.minX = std::min(cells.minX, cellX);
          cells.minZ = std::min(cells.minZ, cellZ);
          cells.maxX = std::max(cells.maxX, cellX);
          cells.maxZ = std::max(cells.maxZ, cellZ);
        }
      }
    }
  }
  return cells;
}

std::array<float, 4> WithAlpha(std::array<float, 4> _color, double _alpha) noexcept
{
  _color[3] = static_cast<float>(_alpha);
  return _color;
}

} // namespace

TerritoryLook::TerritoryLook(const TuningPresentation& _presentation, const TuningMvp& _tuning) noexcept
  : m_presentation(&_presentation),
    m_tuning(&_tuning)
{
}

void TerritoryLook::Clear() noexcept
{
  m_terrain = nullptr;
  m_lines.clear();
  m_chunks.clear();
  m_spots.clear();
}

void TerritoryLook::Update(const StateReplica::View& _latest, const RegionTerrain& _terrain)
{
  if (&_terrain != m_terrain)
  {
    Clear();
    m_terrain = &_terrain;
    PlaceBorderSpots(_terrain);
  }
  // Every settlement of the replica, the public layer (WLD-242), in site order; a line is built again only when its
  // chunks changed.
  std::vector<Line> lines;
  std::vector<std::vector<ReplicatedCells>> chunksOf;
  const auto lastSettlement = _latest.upper_bound(SettlementObjectId(LAST_ID_BYTE));
  for (auto it = _latest.lower_bound(SettlementObjectId(0)); it != lastSettlement; ++it)
  {
    const auto* settlement = std::get_if<ReplicatedSettlement>(&it->second);
    if (settlement == nullptr)
    {
      continue;
    }
    std::vector<ReplicatedCells> chunks;
    const auto lastChunk = _latest.upper_bound(CellChunkObjectId(CellLayer::Territory, settlement->site, LAST_ID_BYTE, LAST_ID_BYTE));
    for (auto chunk = _latest.lower_bound(CellChunkObjectId(CellLayer::Territory, settlement->site, 0, 0)); chunk != lastChunk; ++chunk)
    {
      if (const auto* cells = std::get_if<ReplicatedCells>(&chunk->second); cells != nullptr)
      {
        chunks.push_back(*cells);
      }
    }
    std::size_t cached = 0;
    while (cached < m_lines.size() && m_lines[cached].site != settlement->site)
    {
      ++cached;
    }
    Line line =
      cached < m_lines.size() && m_chunks[cached] == chunks ? std::move(m_lines[cached]) : BuildLine(settlement->site, chunks, _terrain);
    line.color = settlement->color;
    lines.push_back(std::move(line));
    chunksOf.push_back(std::move(chunks));
  }
  m_lines = std::move(lines);
  m_chunks = std::move(chunksOf);
}

TerritoryLook::Line TerritoryLook::BuildLine(std::uint8_t _site, const std::vector<ReplicatedCells>& _chunks,
                                             const RegionTerrain& _terrain) const
{
  Line line;
  line.site = _site;
  const RegionLayout& layout = _terrain.layout;
  const double cell = m_tuning->mvp.territory.cellSize;
  const TerritoryCells cells = CellsOf(_chunks, static_cast<std::int32_t>(std::ceil(layout.side / cell)));
  if (cells.maxX < 0)
  {
    return line;
  }
  // The line runs halfway between held and free cell centres, at the line's lift over the terrain.
  const double lift = m_presentation->mvp.territoryLine.lift;
  const auto at = [&](std::int32_t _x, std::int32_t _z, std::int8_t _edge)
  {
    const std::array<double, 2>& point = EDGE_POINTS[static_cast<std::uint8_t>(_edge)];
    const double x = layout.originX + (_x + point[0]) * cell;
    const double z = layout.originZ + (_z + point[1]) * cell;
    return Neuron::Vec3{x, _terrain.field.Height(x, z) + lift, z};
  };
  for (std::int32_t z = cells.minZ - 1; z <= cells.maxZ; ++z)
  {
    for (std::int32_t x = cells.minX - 1; x <= cells.maxX; ++x)
    {
      const std::size_t index = (cells.Held(x, z) ? 1u : 0u) | (cells.Held(x + 1, z) ? 2u : 0u) | (cells.Held(x + 1, z + 1) ? 4u : 0u) |
                                (cells.Held(x, z + 1) ? 8u : 0u);
      const std::array<std::int8_t, 4>& pieces = CASES[index];
      for (std::size_t piece = 0; piece < pieces.size() && pieces[piece] >= 0; piece += 2)
      {
        line.pieces.push_back({at(x, z, pieces[piece]), at(x, z, pieces[piece + 1])});
      }
    }
  }
  return line;
}

void TerritoryLook::PlaceBorderSpots(const RegionTerrain& _terrain)
{
  // WLD-062: the lens where the discs of wld.territory.max_radius about two valley neighbours overlap, inside the region
  // and outside the coast bands, split at the halfway line. Its spots lie every mvp.border_mark.spacing along the
  // halfway line, from one meeting of the two circles to the other, and along each circle's arc inside the other disc.
  const RegionLayout& layout = _terrain.layout;
  const double band = m_tuning->wld.region.coastBand;
  const double radius = m_tuning->wld.territory.maxRadius;
  const TuningPresentation::Mvp::BorderMark& mark = m_presentation->mvp.borderMark;
  const double spacing = std::max(mark.spacing, 1.0);
  const auto place = [&](const Neuron::Vec3& _point)
  {
    if (_point.x >= layout.originX + band && _point.z >= layout.originZ + band && _point.x <= layout.originX + layout.side - band &&
        _point.z <= layout.originZ + layout.side - band)
    {
      m_spots.push_back({_point.x, _terrain.field.Height(_point.x, _point.z) + mark.spotSize, _point.z});
    }
  };
  for (const auto& pair : VALLEY_PAIRS)
  {
    const Neuron::Vec3& first = layout.sites[pair[0]];
    const Neuron::Vec3& second = layout.sites[pair[1]];
    const Neuron::Vec3 across{second.x - first.x, 0.0, second.z - first.z};
    const double distance = Neuron::Length(across);
    if (distance <= 0.0 || distance >= 2.0 * radius)
    {
      continue;
    }
    const Neuron::Vec3 toward = across * (1.0 / distance);
    const Neuron::Vec3 along{-toward.z, 0.0, toward.x};
    const Neuron::Vec3 middle = (first + second) * 0.5;
    const double reach = std::sqrt(radius * radius - distance * distance / 4.0);
    const auto lineSpots = static_cast<std::int64_t>(std::max(1.0, std::ceil(2.0 * reach / spacing)));
    for (std::int64_t k = 0; k <= lineSpots; ++k)
    {
      place(middle + along * (-reach + 2.0 * reach * static_cast<double>(k) / static_cast<double>(lineSpots)));
    }
    // Each arc runs between the two meetings, which the halfway line already marks.
    const double half = std::acos(std::clamp(distance / (2.0 * radius), -1.0, 1.0));
    const auto arcSpots = static_cast<std::int64_t>(std::max(1.0, std::ceil(2.0 * half * radius / spacing)));
    for (const auto& [center, other] : {std::pair{first, second}, std::pair{second, first}})
    {
      const double facing = std::atan2(other.z - center.z, other.x - center.x);
      for (std::int64_t k = 1; k < arcSpots; ++k)
      {
        const double angle = facing - half + 2.0 * half * static_cast<double>(k) / static_cast<double>(arcSpots);
        place({center.x + radius * std::cos(angle), 0.0, center.z + radius * std::sin(angle)});
      }
    }
  }
}

void TerritoryLook::AppendBatches(std::vector<Neuron::QuadBatch>& _batches) const
{
  const TuningPresentation::Mvp::TerritoryLine& look = m_presentation->mvp.territoryLine;
  Neuron::QuadBatch lines{Neuron::ScenePass::Effects,
                          EFFECT_ADDITIVE,
                          static_cast<std::uint16_t>(SceneTexture::BeamLong),
                          static_cast<float>(m_tuning->mvp.territory.cellSize + look.halfWidth),
                          0.0f,
                          {}};
  for (const Line& line : m_lines)
  {
    // MVP-084: in the Cladan's colour (MVP-013), u along the piece and the beam's glow across it.
    const std::array<float, 4> color = WithAlpha(TeamColor(line.color), look.alpha);
    for (const auto& piece : line.pieces)
    {
      Neuron::SceneQuad quad;
      quad.position = piece[0];
      quad.endPoint = piece[1];
      quad.halfWidth = static_cast<float>(look.halfWidth);
      quad.halfHeight = quad.halfWidth;
      quad.color = color;
      quad.endColor = color;
      quad.orientation = Neuron::QuadOrientation::Segment;
      lines.quads.push_back(quad);
    }
  }
  if (!lines.quads.empty())
  {
    _batches.push_back(std::move(lines));
  }
  // The border areas are marked on the territories (MVP-084): a world with no settlement, the sandbox, draws none, so
  // M1's scenes stay as they were measured.
  if (m_lines.empty())
  {
    return;
  }
  const TuningPresentation::Mvp::BorderMark& mark = m_presentation->mvp.borderMark;
  Neuron::QuadBatch spots{Neuron::ScenePass::Effects,
                          EFFECT_ADDITIVE,
                          static_cast<std::uint16_t>(SceneTexture::ParticleDot),
                          static_cast<float>(mark.spotSize),
                          0.0f,
                          {}};
  const std::array<float, 4> color{static_cast<float>(mark.color.r), static_cast<float>(mark.color.g), static_cast<float>(mark.color.b),
                                   static_cast<float>(mark.alpha)};
  for (const Neuron::Vec3& spot : m_spots)
  {
    Neuron::SceneQuad quad;
    quad.position = spot;
    quad.halfWidth = static_cast<float>(mark.spotSize);
    quad.halfHeight = quad.halfWidth;
    quad.color = color;
    quad.endColor = color;
    spots.quads.push_back(quad);
  }
  if (!spots.quads.empty())
  {
    _batches.push_back(std::move(spots));
  }
}

} // namespace Species
