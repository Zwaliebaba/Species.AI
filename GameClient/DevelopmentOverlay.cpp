#include "pch.h"

#include "AssetError.h"
#include "DevelopmentOverlay.h"
#include "Log.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <utility>

namespace Species
{

namespace
{

// R-SIM-415: the base colours of the eight player colours, by colour id, in RGB from 0 to 255: ref.sim.team_color.green.base,
// .red.base, .yellow.base, .blue.base, .orange.base, .purple.base, .cyan.base and .pink.base. They are constants that cite
// their keys, because a record field named `base` would generate a type the R2 checker refuses (ADR-116, ADR-118).
constexpr std::array<std::array<std::uint8_t, 3>, 8> TEAM_BASE_COLORS{{
  {100, 255, 100},
  {255, 50, 50},
  {255, 255, 50},
  {120, 180, 255},
  {255, 150, 0},
  {150, 0, 255},
  {70, 200, 200},
  {255, 150, 255},
}};
constexpr float COLOR_LEVELS = 255.0f;

} // namespace

std::shared_ptr<const Neuron::FontAsset> LoadDevelopmentFace(const Neuron::LooseContent& _content)
{
  const auto bytes = _content.Read(DEVELOPMENT_FACE_KEY);
  if (!bytes)
  {
    Neuron::Log(Neuron::LogCategory::Content, Neuron::LogLevel::Error, "the development overlay's face: {}",
                Neuron::FormatAssetError(bytes.error()));
    return nullptr;
  }
  auto face = Neuron::ReadFontAsset(*bytes);
  if (!face)
  {
    Neuron::Log(Neuron::LogCategory::Content, Neuron::LogLevel::Error, "the development overlay's face {} is damaged: {}",
                DEVELOPMENT_FACE_KEY, face.error());
    return nullptr;
  }
  return std::make_shared<const Neuron::FontAsset>(std::move(*face));
}

std::string FrameRateText(double _framesPerSecond, std::optional<std::size_t> _objects)
{
  // The strings ui.dev.overlay.frame_rate_entities and ui.dev.overlay.frame_rate, until the string table is data; English
  // only, as development text is (DBG-004).
  const long long framesPerSecond = std::llround(std::max(_framesPerSecond, 0.0));
  return _objects.has_value() ? std::format("{} fps · {} objects", framesPerSecond, *_objects) : std::format("{} fps", framesPerSecond);
}

Neuron::OverlayLine FrameRateLine(const TuningPresentation& _presentation, double _framesPerSecond, std::optional<std::size_t> _objects)
{
  Neuron::OverlayLine line;
  line.text = FrameRateText(_framesPerSecond, _objects);
  line.fromRight = static_cast<float>(_presentation.dbg.overlay.frameRateRight);
  line.fromBottom = static_cast<float>(_presentation.dbg.overlay.frameRateBottom);
  return line;
}

std::array<float, 4> TeamColor(std::uint8_t _team) noexcept
{
  if (_team >= TEAM_BASE_COLORS.size())
  {
    return {1.0f, 1.0f, 1.0f, 1.0f};
  }
  const std::array<std::uint8_t, 3>& levels = TEAM_BASE_COLORS[_team];
  return {static_cast<float>(levels[0]) / COLOR_LEVELS, static_cast<float>(levels[1]) / COLOR_LEVELS,
          static_cast<float>(levels[2]) / COLOR_LEVELS, 1.0f};
}

std::vector<Neuron::DevelopmentDot> FigureDots(std::span<const InterpolatedFigure> _figures)
{
  std::vector<Neuron::DevelopmentDot> dots;
  dots.reserve(_figures.size());
  for (const InterpolatedFigure& figure : _figures)
  {
    if (figure.state.dead)
    {
      continue;
    }
    Neuron::DevelopmentDot dot;
    dot.position = figure.position + Neuron::Vec3{0.0, DOT_LIFT_WU, 0.0};
    dot.color = TeamColor(figure.state.team);
    dot.sidePixels = figure.state.kind == FigureKind::Leader ? LEADER_DOT_PIXELS : WORKER_DOT_PIXELS;
    dots.push_back(dot);
  }
  return dots;
}

} // namespace Species
