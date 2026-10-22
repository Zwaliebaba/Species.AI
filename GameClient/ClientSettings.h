#pragma once

#include "AppWindow.h"
#include "FrameRenderer.h"
#include "SettingsStore.h"
#include "WindowGeometry.h"

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace Species
{

// The settings the client stores at M2 (DAT table 3.1-A as ADR-150 narrows it), by key.
inline constexpr std::string_view SETTING_WINDOW_MODE = "render.window_mode";
inline constexpr std::string_view SETTING_RENDER_WIDTH = "render.render_width";
inline constexpr std::string_view SETTING_RENDER_HEIGHT = "render.render_height";
inline constexpr std::string_view SETTING_PRESENT_MODE = "render.present_mode";
inline constexpr std::string_view SETTING_FRAME_RATE_CAP = "render.frame_rate_cap";
inline constexpr std::string_view SETTING_LOW_LATENCY = "render.low_latency";
inline constexpr std::string_view SETTING_ANTI_ALIASING = "render.anti_aliasing";
inline constexpr std::string_view SETTING_BRIGHTNESS = "render.brightness";
inline constexpr std::string_view SETTING_SEA_DETAIL = "render.sea_detail";
inline constexpr std::string_view SETTING_ENTITY_DETAIL = "render.entity_detail";
inline constexpr std::string_view SETTING_WORKER_SCALE_FACTOR = "render.worker_scale_factor";
inline constexpr std::string_view SETTING_UI_SCALE = "interface.ui_scale";
inline constexpr std::string_view SETTING_TEXT_SCALE = "interface.text_scale";
inline constexpr std::string_view SETTING_REDUCED_FLASH = "interface.reduced_flash";
inline constexpr std::string_view SETTING_REDUCED_MOTION = "interface.reduced_motion";
inline constexpr std::string_view SETTING_MASTER_VOLUME = "audio.master_volume";
inline constexpr std::string_view SETTING_KEYBOARD_LAYOUT = "input.keyboard_layout"; // INP-164: absent, the system's layout
inline constexpr std::string_view SETTING_BINDING_FAMILY = "binding.";               // INP-070: binding.<action id>

// The enumerators of the enumerations, as the file spells them (DAT-197).
inline constexpr std::string_view WINDOW_MODE_WINDOWED = "windowed";
inline constexpr std::string_view WINDOW_MODE_BORDERLESS = "borderless_full_screen";
inline constexpr std::string_view PRESENT_MODE_VERTICAL_SYNC = "vertical_sync";
inline constexpr std::string_view PRESENT_MODE_UNCAPPED = "uncapped";
inline constexpr std::string_view ANTI_ALIASING_OFF = "off";
inline constexpr std::string_view ANTI_ALIASING_2X = "2x";
inline constexpr std::string_view ANTI_ALIASING_4X = "4x";

// The one defaults table (DAT-193, DEC-100, PLT-030): every setting the client stores, with its type, range and default,
// compiled in so that it exists before any content is mounted. The `rnd.option.*.default` keys stay named constants
// here (ADR-116, ADR-150); the UI and text scales are absent until set, and then the presentation record's
// `ui.scale.*_default` apply.
[[nodiscard]] std::vector<Neuron::SettingSpec> ClientSettingSpecs();

/// The display options the client's presentation reads, from the store (RND-442, R-META-511; ADR-150).
struct DisplayOptions
{
  std::int64_t seaDetail = 1;     // render.sea_detail: 0 to 3 (RND-144)
  std::int64_t entityDetail = 1;  // render.entity_detail: 1 to 3 (RND-446)
  double workerScaleFactor = 2.0; // render.worker_scale_factor (R-UNIT-142)
  double brightness = 1.0;        // render.brightness: the final pass's multiplier (RND-246)
  bool reducedFlash = false;      // interface.reduced_flash (VFX-235, DEC-964)
  bool reducedMotion = false;     // interface.reduced_motion (VFX-232, DEC-964)
  std::optional<double> uiScale;  // interface.ui_scale; none: ui.scale.ui_default (UI-168)
  std::optional<double> textScale;
};

[[nodiscard]] DisplayOptions DisplayOptionsOf(const Neuron::SettingsStore& _store);
// The renderer's settings of RND-442 in _config: the present mode, the frame-rate cap, low latency and anti-aliasing.
void ApplyRenderSettings(const Neuron::SettingsStore& _store, Neuron::RenderConfig& _config);
[[nodiscard]] Neuron::WindowMode WindowModeOf(const Neuron::SettingsStore& _store);
// RND-021: the stored render size, or {0, 0} while none is stored, which takes the desktop size (PLT-112).
[[nodiscard]] Neuron::SizePx RenderSizeOf(const Neuron::SettingsStore& _store);
// PLT-080, R-META-424: the renderer's start-up writes the display settings it started with back to the store.
void WriteBackDisplay(Neuron::SettingsStore& _store, Neuron::WindowMode _mode, Neuron::SizePx _renderSize);

} // namespace Species
