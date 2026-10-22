#pragma once

#include "GlyphAtlas.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace Neuron
{

// UI-195 as O92 amends it: the font roles. Each role draws from a chain of system font families, its face the first
// family of the chain that the system holds, and a character that face lacks from the next family that has it (UI-234).
enum class FontRole : std::uint8_t
{
  Title,
  Body,
  Caption,
  Development, // the development overlay and windows, Debug builds only (DBG-004)
  Count
};

inline constexpr std::size_t FONT_ROLE_COUNT = static_cast<std::size_t>(FontRole::Count);

/// One glyph of a laid-out text (ADR-147): its atlas page, its rectangle in whole pixels from the text's top-left
/// corner (left, top, right, bottom), and its texture coordinates on the page in the same order.
struct PlacedGlyph
{
  std::uint16_t page = 0;
  std::array<float, 4> rectangle{};
  std::array<float, 4> uv{};
};

/// A text laid out at its final size (UI-171) by DirectWrite, its glyphs rasterized into the atlas (ADR-147). Only the
/// glyphs that draw something are held.
struct TextLine
{
  std::vector<PlacedGlyph> glyphs;
  float widthPixels = 0.0f;    // the measured advance width, trailing spaces included (UI-200)
  float heightPixels = 0.0f;   // the height of its lines
  float baselinePixels = 0.0f; // the first baseline's distance from the top, a whole number of pixels
};

/// The text system (UI-198 as ADR-034 specifies it, O92, ADR-147): DirectWrite lays a text out and shapes it in its
/// role's chain of system fonts, and each glyph not yet cached is rasterized in grayscale at its final size and
/// sub-pixel offset into the glyph atlas, which the UI renderer uploads. Every call is made from the frame thread.
/// When DirectWrite cannot be opened, which is logged, every text is laid out empty.
class TextSystem
{
public:
  struct Desc
  {
    std::array<std::vector<std::string>, FONT_ROLE_COUNT> families; // each role's chain of family names, its face first
    GlyphAtlas::Desc atlas;
    std::uint32_t subpixelSteps = 4;   // ui.glyph.subpixel_steps
    std::string languageTag = "en-US"; // the language whose shaping rules apply (BCP 47, ADR-063)
  };

  explicit TextSystem(const Desc& _desc);
  ~TextSystem();

  TextSystem(const TextSystem&) = delete;
  TextSystem& operator=(const TextSystem&) = delete;

  [[nodiscard]] bool Available() const noexcept;
  // The frame whose use the atlas's and the layouts' least-recently-used choices count.
  void BeginFrame(std::uint64_t _frame);
  // _utf8 in _role at a final size of _sizePixels, its top-left corner at the origin, broken into lines only at its
  // line breaks. A glyph the atlas cannot hold, larger than a page, is left out and logged once.
  [[nodiscard]] TextLine Layout(std::string_view _utf8, FontRole _role, float _sizePixels);
  // The height and baseline of a line in _role at _sizePixels, with no glyphs.
  [[nodiscard]] TextLine Metrics(FontRole _role, float _sizePixels);

  [[nodiscard]] std::shared_ptr<const GlyphAtlas> Atlas() const noexcept
  {
    return m_atlas;
  }
  // The glyphs rasterized since the system opened (UI-241: none in steady state).
  [[nodiscard]] std::uint64_t GlyphsRasterized() const noexcept;
  // The family of the system's that _role draws its face from: the first of its chain that the system holds, or the
  // first of the chain when it holds none.
  [[nodiscard]] std::string FaceFamily(FontRole _role) const;

private:
  struct Implementation; // DirectWrite's objects and the layouts kept

  std::shared_ptr<GlyphAtlas> m_atlas;
  std::unique_ptr<Implementation> m_implementation;
};

} // namespace Neuron
