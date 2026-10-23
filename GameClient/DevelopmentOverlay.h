#pragma once

#include "FrameProfile.h"
#include "ReplicaTiming.h"
#include "ServerStats.h"
#include "StateReplica.h"
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

namespace Neuron
{
class TextSystem;
}

namespace Species
{

struct TuningMvp;
struct TuningPresentation;

// ADR-118: a development dot's side in unscaled pixels, a Leader's larger, and how far above a figure's feet it sits.
inline constexpr float WORKER_DOT_PIXELS = 3.0f;
inline constexpr float LEADER_DOT_PIXELS = 6.0f;
inline constexpr double DOT_LIFT_WU = 3.0;

// DBG-095: the frame-rate line's text, the frame rate and, while a world exists, the number of live simulated objects
// (ui.dev.overlay.frame_rate, ui.dev.overlay.frame_rate_entities).
[[nodiscard]] std::string FrameRateText(double _framesPerSecond, std::optional<std::size_t> _objects);
// The frame-rate line at its anchor (dbg.overlay.frame_rate_right, dbg.overlay.frame_rate_bottom).
[[nodiscard]] Neuron::OverlayLine FrameRateLine(const TuningPresentation& _presentation, double _framesPerSecond,
                                                std::optional<std::size_t> _objects);

// R-SIM-415, R-SIM-416: a team's base colour, its colour id its team id outside co-op and Siege, from 0 to 1; white
// for a slot beyond the eight player colours. From M3 (ADR-167) an expedition's team, 4 + its site (ADR-161), takes its
// settlement's colour.
[[nodiscard]] std::array<float, 4> TeamColor(std::uint8_t _team) noexcept;

// M1 task T15a, ADR-118: a dot for each living figure at its interpolated position, in its team's colour, until its
// crowd is drawn (T16).
[[nodiscard]] std::vector<Neuron::DevelopmentDot> FigureDots(std::span<const InterpolatedFigure> _figures);

// ADR-120: the root steps of the client's frame loop that the frame page shares out (DBG-096), by their index in the
// frame profile; the wait is the renderer's, for the latency object, the cap, the fence and the present.
enum class ClientStep : std::uint8_t
{
  Input,
  Session,
  Presentation,
  Render,
  Wait,
  Count
};
inline constexpr std::array<std::string_view, 5> CLIENT_STEP_NAMES{"input", "session", "presentation", "render", "wait"};
// ADR-120: the statistics line goes to the log every tenth roll-up period.
inline constexpr std::uint64_t STATISTICS_LOG_PERIODS = 10;

/// DBG-096 as M1 has it: the living figures of a team by actor kind; M1 has no structures.
struct SideCount
{
  std::uint32_t workers = 0;
  std::uint32_t leaders = 0;
};

/// The live objects of the replica's latest view (DBG-096): each team's figures, from slot 0 to the highest seen, and the
/// Essence orbs and effects. M1 has no laser bolts and no particles.
struct ReplicaCounts
{
  std::vector<SideCount> sides;
  std::size_t orbs = 0;
  std::size_t effects = 0;
};

[[nodiscard]] ReplicaCounts CountReplica(const StateReplica& _replica);

/// What the frame page shows (DBG-096, DBG-097; ADR-120).
struct FramePageInputs
{
  const Neuron::FrameProfile* profile = nullptr; // the frame loop's; none shows no figures of it
  double framesPerSecond = 0.0;
  std::optional<std::size_t> objects; // the frame-rate line's count, while a world exists
  ReplicaCounts counts;
  std::optional<ServerStats> server; // the latest of a Debug server's statistics
};

/// The frame page's lines, and its backdrop and the frame-time graph's rectangles, laid out from the target's left edge.
struct FramePage
{
  std::vector<Neuron::OverlayLine> lines;
  std::vector<Neuron::OverlayRect> rects;
};

// DBG-096, DBG-097, DBG-089: the frame page: the frame-rate line (DBG-095, ADR-176); the percentiles of the frame time,
// and of the frame's work against its budget (rnd.budget.target_frame_rate); each step's share; the counts; the server's
// tick against half the tick period (ADR-080 item 6); and the frame-time graph, all on a dark backdrop. A row over its
// budget is drawn in dbg.overlay.warning_rgb. Its text is measured in _text's development role at dbg.overlay.text_size;
// every figure is in pixels at a DPI scale of 1. ADR-176: the page stands under _belowPixels, the bottom of the HUD's
// settlement panel, when the HUD shows one, else at the target's top-left corner.
[[nodiscard]] FramePage BuildFramePage(const FramePageInputs& _inputs, const TuningPresentation& _presentation, const TuningMvp& _tuning,
                                       Neuron::TextSystem& _text, std::optional<float> _belowPixels);
// ADR-120: the frame page's figures as one line of the log, written every tenth roll-up period so that the overlay can
// be checked against it.
[[nodiscard]] std::string StatisticsLine(const FramePageInputs& _inputs, const TuningPresentation& _presentation);

} // namespace Species
