#pragma once

#include <array>
#include <string_view>

namespace Species
{

// UI-195 and UI-234 as O92 amends them (ADR-147): the system font families that Species' text draws from, each chain
// in order, its first family the face and the rest its fallback. The player's text, in the title, body and caption
// roles, is in Bahnschrift, with Segoe UI for what Bahnschrift lacks; the development role is in Cascadia Mono, with
// Consolas where Cascadia Mono is not installed. The client lays text out in them and the content build checks the
// string tables' coverage against them.
inline constexpr std::array<std::string_view, 2> GAME_FONT_FAMILIES{"Bahnschrift", "Segoe UI"};
inline constexpr std::array<std::string_view, 2> DEVELOPMENT_FONT_FAMILIES{"Cascadia Mono", "Consolas"};

} // namespace Species
