#include "pch.h"

#include "UnitCircle.h"

#include <array>
#include <cmath>

namespace MeshKit
{

namespace
{

// A turn is 720 half-degrees, so that 22.5° is a whole number of units.
constexpr std::uint32_t HALF_DEGREES_PER_TURN = 720;
constexpr std::uint32_t HALF_DEGREES_PER_QUADRANT = 180;
constexpr std::array<std::uint32_t, 9> STEP_COUNTS{3, 4, 5, 6, 8, 10, 12, 16, 20};

// The first quadrant's directions that those step counts reach, by their angle in half-degrees.
std::optional<Direction> FirstQuadrant(std::uint32_t _halfDegrees) noexcept
{
  const double root2 = std::sqrt(2.0);
  const double root3 = std::sqrt(3.0);
  const double root5 = std::sqrt(5.0);
  const double cos18 = std::sqrt(10.0 + 2.0 * root5) / 4.0;
  const double sin18 = (root5 - 1.0) / 4.0;
  const double cos36 = (root5 + 1.0) / 4.0;
  const double sin36 = std::sqrt(10.0 - 2.0 * root5) / 4.0;
  const double cos225 = std::sqrt(2.0 + root2) / 2.0;
  const double sin225 = std::sqrt(2.0 - root2) / 2.0;
  switch (_halfDegrees)
  {
  case 0:
    return Direction{1.0, 0.0};
  case 36:
    return Direction{cos18, sin18};
  case 45:
    return Direction{cos225, sin225};
  case 60:
    return Direction{root3 / 2.0, 0.5};
  case 72:
    return Direction{cos36, sin36};
  case 90:
    return Direction{root2 / 2.0, root2 / 2.0};
  case 108:
    return Direction{sin36, cos36};
  case 120:
    return Direction{0.5, root3 / 2.0};
  case 135:
    return Direction{sin225, cos225};
  case 144:
    return Direction{sin18, cos18};
  default:
    return std::nullopt;
  }
}

} // namespace

std::optional<Direction> TurnDirection(std::uint32_t _step, std::uint32_t _steps) noexcept
{
  bool known = false;
  for (const std::uint32_t steps : STEP_COUNTS)
  {
    known = known || steps == _steps;
  }
  if (!known)
  {
    return std::nullopt;
  }
  const std::uint32_t halfDegrees = (_step % _steps) * (HALF_DEGREES_PER_TURN / _steps);
  const std::optional<Direction> first = FirstQuadrant(halfDegrees % HALF_DEGREES_PER_QUADRANT);
  if (!first)
  {
    return std::nullopt;
  }
  const Direction d = first.value_or(Direction{});
  switch (halfDegrees / HALF_DEGREES_PER_QUADRANT)
  {
  case 0:
    return d;
  case 1:
    return Direction{-d.sine, d.cosine};
  case 2:
    return Direction{-d.cosine, -d.sine};
  default:
    return Direction{d.sine, -d.cosine};
  }
}

} // namespace MeshKit
