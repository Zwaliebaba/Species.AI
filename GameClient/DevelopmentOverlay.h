#pragma once

#include "FontAsset.h"
#include "LooseContent.h"
#include "ReplicaTiming.h"
#include "TuningPresentation.h"
#include "WorldScene.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Species
{

// The content key of the development overlay's face, EditorFont (O62, O64; AST-014).
inline constexpr std::string_view DEVELOPMENT_FACE_KEY = "fonts/editorfont";

// ADR-118: a development dot's side in unscaled pixels, a Leader's larger, and how far above a figure's feet it sits.
inline constexpr float WORKER_DOT_PIXELS = 3.0f;
inline constexpr float LEADER_DOT_PIXELS = 6.0f;
inline constexpr double DOT_LIFT_WU = 3.0;

// The development overlay's face from the content: none when it is missing or damaged, which is logged.
[[nodiscard]] std::shared_ptr<const Neuron::FontAsset> LoadDevelopmentFace(const Neuron::LooseContent& _content);

// DBG-095: the frame-rate line's text, the frame rate and, while a world exists, the number of live simulated objects
// (ui.dev.overlay.frame_rate, ui.dev.overlay.frame_rate_entities).
[[nodiscard]] std::string FrameRateText(double _framesPerSecond, std::optional<std::size_t> _objects);
// The frame-rate line at its anchor (dbg.overlay.frame_rate_right, dbg.overlay.frame_rate_bottom).
[[nodiscard]] Neuron::OverlayLine FrameRateLine(const TuningPresentation& _presentation, double _framesPerSecond,
                                                std::optional<std::size_t> _objects);

// R-SIM-415, R-SIM-416: a team's base colour, its colour id its team id outside co-op and Siege, from 0 to 1; white
// for a slot beyond the eight player colours.
[[nodiscard]] std::array<float, 4> TeamColor(std::uint8_t _team) noexcept;

// M1 task T15a, ADR-118: a dot for each living figure at its interpolated position, in its team's colour, until its
// crowd is drawn (T16).
[[nodiscard]] std::vector<Neuron::DevelopmentDot> FigureDots(std::span<const InterpolatedFigure> _figures);

} // namespace Species
