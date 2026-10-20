#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace ContentBuild
{

struct PixelSize
{
  std::uint32_t width = 0;
  std::uint32_t height = 0;
};

// The manifest's columns are prose for people and the build alike (AST-012); these read the numbers in them.

// The first "<width>x<height> px" of a row's text at or after a phrase, or the first in the text when the phrase is
// empty; none when the text holds none there.
[[nodiscard]] std::optional<PixelSize> PixelSizeAfter(std::string_view _text, std::string_view _phrase = {});
// The whole number that follows a phrase, as in "baseline 13" or "<= 100 triangles"; none when it is not there.
[[nodiscard]] std::optional<std::uint64_t> NumberAfter(std::string_view _text, std::string_view _phrase);
// The whole number before a phrase, as in "1 node" or "100 triangles budget"; none when it is not there.
[[nodiscard]] std::optional<std::uint64_t> NumberBefore(std::string_view _text, std::string_view _phrase);

} // namespace ContentBuild
