#pragma once

#include "BitReader.h"
#include "BitWriter.h"
#include "Vec3.h"

#include <cstdint>
#include <span>
#include <vector>

namespace Species
{

// A route's handle (R-SIM-359): routes are numbered from 1 in the order they are built and never reused; 0 is none.
using RouteHandle = std::uint32_t;

/// The routes a region's simulation has built and still holds (R-SIM-352, DEC-651 resolved by O71): the region frees a
/// route once nothing holds it, and handles keep counting, so a freed handle never resolves again. Routes are state:
/// the figures follow them.
class RouteStore
{
public:
  [[nodiscard]] RouteHandle Add(std::vector<Neuron::Vec3> _points);
  // The route's points, or null for a handle the store does not hold.
  [[nodiscard]] const std::vector<Neuron::Vec3>* Find(RouteHandle _handle) const noexcept;
  // The routes held.
  [[nodiscard]] std::size_t Count() const noexcept
  {
    return m_routes.size();
  }
  // The last handle given, 0 before the first.
  [[nodiscard]] RouteHandle LastHandle() const noexcept
  {
    return m_lastHandle;
  }

  // DEC-651: frees every route whose handle is not in _held, which is sorted and holds each handle once.
  void KeepOnly(std::span<const RouteHandle> _held);

  // The last handle, then every route in handle order, each its handle, its point count and its points as binary64
  // (ADR-081 item 7).
  void WriteState(Neuron::BitWriter& _writer) const;
  // Into an empty store; false when the values do not read whole, are not finite, or the handles are not ascending
  // from 1 to the last handle.
  [[nodiscard]] bool ReadState(Neuron::BitReader& _reader);

private:
  struct Route
  {
    RouteHandle handle = 0;
    std::vector<Neuron::Vec3> points;
  };

  std::vector<Route> m_routes; // in handle order
  RouteHandle m_lastHandle = 0;
};

} // namespace Species
