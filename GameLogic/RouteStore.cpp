#include "pch.h"

#include "Check.h"
#include "RouteStore.h"

#include <bit>
#include <cmath>
#include <utility>

namespace Species
{

namespace
{

// A store larger than this, or a route longer than this, is no state.
constexpr std::uint64_t ROUTES_MAX = std::uint64_t{1} << 24;
constexpr std::uint64_t ROUTE_POINTS_MAX = std::uint64_t{1} << 16;

} // namespace

RouteHandle RouteStore::Add(std::vector<Neuron::Vec3> _points)
{
  ENGINE_CHECK(m_routes.size() < ROUTES_MAX && _points.size() <= ROUTE_POINTS_MAX);
  m_routes.push_back(std::move(_points));
  return static_cast<RouteHandle>(m_routes.size());
}

const std::vector<Neuron::Vec3>* RouteStore::Find(RouteHandle _handle) const noexcept
{
  return _handle == 0 || _handle > m_routes.size() ? nullptr : &m_routes[_handle - 1];
}

void RouteStore::WriteState(Neuron::BitWriter& _writer) const
{
  _writer.WriteBounded(m_routes.size(), ROUTES_MAX);
  for (const std::vector<Neuron::Vec3>& route : m_routes)
  {
    _writer.WriteBounded(route.size(), ROUTE_POINTS_MAX);
    for (const Neuron::Vec3& point : route)
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
  const std::uint64_t count = _reader.ReadBounded(ROUTES_MAX);
  for (std::uint64_t n = 0; n < count && !_reader.Failed(); ++n)
  {
    std::vector<Neuron::Vec3>& route = m_routes.emplace_back();
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
      route.push_back(point);
    }
  }
  return !_reader.Failed();
}

} // namespace Species
