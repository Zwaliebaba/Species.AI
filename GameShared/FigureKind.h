#pragma once

#include <cstdint>

namespace Species
{

// The kinds of figure the MVP builds (MVP-040): the Worker and the Leader of the reference crowd rules. The client
// reads the kind of a replicated figure to draw it, so it is a shared type.
enum class FigureKind : std::uint8_t
{
  Worker,
  Leader
};

inline constexpr std::uint8_t FIGURE_KIND_MAX = static_cast<std::uint8_t>(FigureKind::Leader);

} // namespace Species
