#pragma once

#include "LooseContent.h"
#include "MessageLocale.h"
#include "NineSlice.h"
#include "StringTable.h"
#include "TextSystem.h"
#include "TuningPresentation.h"
#include "UiCore.h"
#include "UiLayout.h"
#include "WindowGeometry.h"
#include "WorldMarkers.h"
#include "WorldScene.h"

#include <cstdint>
#include <optional>
#include <memory>
#include <vector>

namespace Species
{

// The UI's images (ADR-145), by their index in Neuron::UiImages::files.
enum class UiImage : std::uint16_t
{
  PanelAccent,   // uitex.panel_accent: window and HUD panel bodies (UI-054)
  PanelNeutral,  // uitex.panel_neutral: HUD panel bodies
  MenuHighlight, // uitex.menu_highlight: the highlight beam behind a page title (UI-114)
  FillBlur,      // uitex.fill_blur: a fill box's body (UI-115)
  GlowCloudy,    // fx.glow_cloudy: a fill box's drifting glow (UI-115)
  IconCheck,     // uitex.icon_check: a check box's tick (UI-081)
  // ADR-152: the settlement HUD's images (MVP-102; the MVP assets' HUD rows).
  HudMeter,              // icon.hud.meter: the track and fill of every HUD meter
  NotificationCard,      // icon.hud.notification_card: the frame behind a notification
  IconResource,          // icon.hud.resource: the stores, their capacity and the costs
  IconGrowth,            // icon.hud.growth: growth and its cause when it stops
  IconPriorityGather,    // icon.hud.priority, in member order: gathering,
  IconPriorityConstruct, // construction
  IconPriorityGuard,     // and guard duty
  IconDwelling,          // icon.hud.dwelling
  IconStorehouse,        // icon.hud.storehouse
  IconWorkers,           // icon.power.workers: the Workers and the population limit
  IconTurret,            // icon.power.gun_turret: the turret on the construction bar
  ArrowPointer,          // cursor.pointer: the arrows of HUD lists (UI-465), pointing down; the priorities' arrows (ADR-153)
  // ADR-166: M3's HUD icons (the MVP assets' HUD rows).
  IconContact,  // icon.hud.contact: contact, and the territory-pressure cause
  IconClaim,    // icon.hud.claim: claims and the claim cause
  IconAttack,   // icon.hud.attack: attacks, the warning and the attack cause
  IconBattle,   // icon.hud.battle: a battle's start and end
  IconTribute,  // icon.hud.tribute: tribute
  IconWithdraw, // icon.hud.withdraw: a withdrawn claim or expedition
  IconRespite,  // icon.hud.respite: a settlement under a respite
  IconRenown,   // icon.hud.renown: the renown standing
  IconRunClock, // icon.hud.run_clock: the run's time left and its quiet minutes
  ResultCard,   // icon.hud.result_card: the frame of a battle's result
  Count
};

// The UI's images from the content; one that is missing is logged and left empty, so that the UI draws without it.
[[nodiscard]] std::shared_ptr<const Neuron::UiImages> LoadUiImages(const Neuron::LooseContent& _content);

// The text system of the client (ADR-147): the font roles' chains of GameFonts.h, the glyph atlas's pages from
// `ui.glyph.*`, and the en-US shaping rules (O91).
[[nodiscard]] Neuron::TextSystem::Desc TextSystemDesc(const TuningPresentation& _presentation);

// ADR-149: the page family's look from the presentation record's `ui.palette.*`, `ui.page_button.*`, `ui.page_title.*`,
// `ui.fill_box.*` and `ui.check.*` keys, with the images of UiImage.
[[nodiscard]] Neuron::UiStyle UiStyleOf(const TuningPresentation& _presentation);
// UI-163: the page grid's keys, `ref.meta.layout.*`.
[[nodiscard]] Neuron::PageGridKeys PageGridKeysOf(const TuningPresentation& _presentation) noexcept;
// ADR-033, UI-168 to UI-171: the metrics of a client area of _client at the DPI scale _dpiScale, at the UI and text scales
// of the settings store, or `ui.scale.*_default` where none is set (ADR-150), with UI-171's minimum and RND-377's HUD
// space.
[[nodiscard]] Neuron::UiMetrics UiMetricsOf(const TuningPresentation& _presentation, Neuron::SizePx _client, float _dpiScale,
                                            std::optional<double> _uiScale, std::optional<double> _textScale) noexcept;

// UI-171 and UI-170 as O92 amends them: the final size of text of layout size _sizePixels for a client area
// _clientHeight pixels tall at the DPI scale _dpiScale and the text scale _textScale.
[[nodiscard]] float UiTextPixels(const TuningPresentation& _presentation, double _sizePixels, std::uint32_t _clientHeight, float _dpiScale,
                                 double _textScale) noexcept;

// R-CTRL-366's count (ADR-127, ADR-145): _label in white in the body role at `ui.text.default_size`, additive (UI-190),
// its line's left edge at the label's left and its line's middle at the label's middle, on whole pixels.
void AppendBadgeCount(const BadgeLabel& _label, Neuron::TextSystem& _text, const TuningPresentation& _presentation,
                      std::uint32_t _clientHeight, float _dpiScale, double _textScale, std::vector<Neuron::UiBatch>& _out);

// ADR-175: the panel image _image as a plate: its size read from its DDS header in _images, with the slices of
// `ui.chrome.accent_slice.*` for uitex.panel_accent and of `ui.chrome.neutral_slice.*` for uitex.panel_neutral; any
// other image, or one missing, has no size and is drawn whole.
[[nodiscard]] Neuron::NineSlice PlateOf(const TuningPresentation& _presentation, const Neuron::UiImages& _images, UiImage _image);

// A Debug build's UI test samples (`--ui-test`, ADR-145, ADR-147, ADR-149), in _area, the test page's right box: the
// accent panel as a window body's plate (UI-054, ADR-175) with the neutral plate beside it, drawn as _images' panels
// at `ui.chrome.plate_texel` × _dpiScale pixels a texel; the body role at four sizes and the
// development role at one; a line each filled additive, outlined (UI-192), inked and laid over; characters of the
// fallback family and one no family draws; a string _strings holds, one it does not and a plural formatted in _locale's
// forms (ADR-146); and a solid and an image swatch in each blend that the UI draws them in, so that every pipeline of
// the UI pass draws.
void AppendUiTestPage(Neuron::TextSystem& _text, const TuningPresentation& _presentation, const Neuron::UiImages& _images,
                      const Neuron::StringTable& _strings, const Neuron::MessageLocale& _locale, const Neuron::UiRect& _area,
                      float _dpiScale, std::vector<Neuron::UiBatch>& _out);

} // namespace Species
