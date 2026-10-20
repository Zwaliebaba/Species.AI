#include "pch.h"

#include "Check.h"
#include "RouteStore.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <utility>

namespace Species
{

namespace
{

// A store holding more than this, or a route longer than this, is no state.
constexpr std::uint64_t ROUTES_MAX = std::uint64_t{1} << 24;
constexpr std::uint64_t ROUTE_POINTS_MAX = std::uint64_t{1} << 16;

} // namespace

RouteHandle RouteStore::Add(std::vector<Neuron::Vec3> _points)
{
  ENGINE_CHECK(m_lastHandle < std::numeric_limits<RouteHandle>::max() && m_routes.size() < ROUTES_MAX &&
               _points.size() <= ROUTE_POINTS_MAX);
  ++m_lastHandle;
  m_routes.push_back({m_lastHandle, std::move(_points)});
  return m_lastHandle;
}

const std::vector<Neuron::Vec3>* RouteStore::Find(RouteHandle _handle) const noexcept
{
  const auto found = std::ranges::lower_bound(m_routes, _handle, {}, &Route::handle);
  return found == m_routes.end() || found->handle != _handle ? nullptr : &found->points;
}

void RouteStore::KeepOnly(std::span<const RouteHandle> _held)
{
  ENGINE_ASSERT(std::ranges::is_sorted(_held) && std::ranges::adjacent_find(_held) == _held.end());
  std::erase_if(m_routes, [&](const Route& _route) { return !std::ranges::binary_search(_held, _route.handle); });
}

void RouteStore::WriteState(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(m_lastHandle, 32);
  _writer.WriteBounded(m_routes.size(), ROUTES_MAX);
  for (const Route& route : m_routes)
  {
    _writer.WriteBits(route.handle, 32);
    _writer.WriteBounded(route.points.size(), ROUTE_POINTS_MAX);
    for (const Neuron::Vec3& point : route.points)
    {
      _writer.WriteBits(std::bit_cast<std::uint64_t>(point.x), 64);
      _writer.WriteBits(std::bit_cast<std::uint64_t>(point.y), 64);
      _writer.WriteBits(std::bit_cast<std::uint64_t>(point.z), 64);
    }
  }
}

bool RouteStore::ReadState(Neuron::BitReader& _reader)
{
  m_routes.clear();
  m_lastHandle = static_cast<RouteHandle>(_reader.ReadBits(32));
  const std::uint64_t count = _reader.ReadBounded(ROUTES_MAX);
  RouteHandle previous = 0;
  for (std::uint64_t n = 0; n < count && !_reader.Failed(); ++n)
  {
    Route& route = m_routes.emplace_back();
    route.handle = static_cast<RouteHandle>(_reader.ReadBits(32));
    if (route.handle <= previous || route.handle > m_lastHandle)
    {
      return false;
    }
    previous = route.handle;
    const std::uint64_t points = _reader.ReadBounded(ROUTE_POINTS_MAX);
    for (std::uint64_t p = 0; p < points && !_reader.Failed(); ++p)
    {
      Neuron::Vec3 point;
      point.x = std::bit_cast<double>(_reader.ReadBits(64));
      point.y = std::bit_cast<double>(_reader.ReadBits(64));
      point.z = std::bit_cast<double>(_reader.ReadBits(64));
      if (!std::isfinite(point.x) || !std::isfinite(point.y) || !std::isfinite(point.z))
      {
        return false;
      }
      route.points.push_back(point);
    }
  }
  return !_reader.Failed();
}

} // namespace Species
