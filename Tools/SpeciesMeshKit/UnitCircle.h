#pragma once

#include <cstdint>
#include <optional>

namespace MeshKit
{

/// A direction in the horizontal plane: the cosine and sine of its angle from +x towards +z.
struct Direction
{
  double cosine = 1.0;
  double sine = 0.0;
};

// The direction _step ÷ _steps of a turn from +x towards +z, for _steps of 3, 4, 5, 6, 8, 10, 12, 16 or 20. Each value
// comes from closed forms of square roots, sums and halvings only, which IEEE 754 rounds the same on every host, so the
// kit writes the same bytes wherever it runs (ADR-128); none for any other count of steps.
[[nodiscard]] std::optional<Direction> TurnDirection(std::uint32_t _step, std::uint32_t _steps) noexcept;

} // namespace MeshKit
