#pragma once

#include "BitReader.h"
#include "BitWriter.h"
#include "Vec3.h"

#include <cstdint>
#include <vector>

namespace Species
{

// A route's handle (R-SIM-359): routes are numbered from 1 in the order they are built and never reused; 0 is none.
using RouteHandle = std::uint32_t;

/// The routes a region's simulation has built (R-SIM-352, R-SIM-357): kept until the world ends, as DEC-651's stated
/// behaviour says, so a handle that a figure holds always resolves. Routes are state: the figures follow them.
class RouteStore
{
public:
  [[nodiscard]] RouteHandle Add(std::vector<Neuron::Vec3> _points);
  // The route's points, or null for a handle the store never gave.
  [[nodiscard]] const std::vector<Neuron::Vec3>* Find(RouteHandle _handle) const noexcept;
  [[nodiscard]] std::size_t Count() const noexcept
  {
    return m_routes.size();
  }

  // Every route in handle order, each its point count and its points as binary64 (ADR-081 item 7).
  void WriteState(Neuron::BitWriter& _writer) const;
  // Into an empty store; false when the values do not read whole or are not finite.
  [[nodiscard]] bool ReadState(Neuron::BitReader& _reader);

private:
  std::vector<std::vector<Neuron::Vec3>> m_routes; // handle − 1
};

} // namespace Species
