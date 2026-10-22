#pragma once

#include "FontAsset.h"
#include "LooseContent.h"
#include "MessageLocale.h"
#include "StringTable.h"
#include "TuningPresentation.h"
#include "WindowGeometry.h"
#include "WorldMarkers.h"
#include "WorldScene.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace Species
{

// The UI's images (ADR-145), by their index in Neuron::UiImages::files.
enum class UiImage : std::uint16_t
{
  PanelAccent,  // uitex.panel_accent: window and HUD panel bodies (UI-054)
  PanelNeutral, // uitex.panel_neutral: HUD panel bodies
  Count
};

// The UI's images from the content; one that is missing is logged and left empty, so that the UI draws without it.
[[nodiscard]] std::shared_ptr<const Neuron::UiImages> LoadUiImages(const Neuron::LooseContent& _content);

// UI-171 and UI-198 as O63 amends them: the whole-number scale at which text of layout size _sizePixels is drawn in
// _face for a client area _clientHeight pixels tall, at a text scale and a UI scale of 1 until the settings store holds
// them (M2 task T15).
[[nodiscard]] std::uint32_t UiTextScale(const TuningPresentation& _presentation, const Neuron::FontAsset& _face, double _sizePixels,
                                        std::uint32_t _clientHeight) noexcept;

// R-CTRL-366's count (ADR-127, ADR-145): _label in white GameFont at the scale of `ui.text.default_size`, additive
// (UI-190), its line's left edge at the label's left and its cell's middle at the label's middle, on whole pixels.
void AppendBadgeCount(const BadgeLabel& _label, const Neuron::FontAsset& _face, const TuningPresentation& _presentation,
                      std::uint32_t _clientHeight, std::vector<Neuron::UiBatch>& _out);

// A Debug build's UI test page (`--ui-test`, ADR-145), at the top-left of a client area of _size: the accent panel strip
// as a window body (UI-054) with the neutral strip beside it; GameFont at the scales 1 to 4; a line each filled additive,
// outlined (UI-192), inked and laid over; the missing glyph; a string _strings holds, one it does not and a plural
// formatted in _locale's forms (ADR-146); and a solid and an image swatch in each blend that the UI draws them in, so
// that every pipeline of the UI pass draws.
void AppendUiTestPage(const Neuron::FontAsset& _face, const TuningPresentation& _presentation, const Neuron::StringTable& _strings,
                      const Neuron::MessageLocale& _locale, Neuron::SizePx _size, std::vector<Neuron::UiBatch>& _out);

} // namespace Species
