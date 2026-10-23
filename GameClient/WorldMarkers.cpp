#include "pch.h"

#include "CrowdLook.h"
#include "DevelopmentOverlay.h"
#include "FigureKind.h"
#include "Picking.h"
#include "ReplicatedState.h"
#include "ResourceSites.h"
#include "WorldMarkers.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <limits>
#include <string>
#include <utility>

namespace Species
{

namespace
{

constexpr Neuron::Vec3 UP{0.0, 1.0, 0.0};
constexpr std::array<float, 4> WHITE{1.0f, 1.0f, 1.0f, 1.0f};
// R-CTRL-366: the badge lies along Normalize(0.85, 1) from the pointer, down and to the right.
constexpr double BADGE_DIRECTION_X = 0.85;
constexpr double BADGE_DIRECTION_Y = 1.0;
// The shortest ray from the screen's centre that finds an edge point, px.
constexpr double EDGE_RAY_MIN = 1e-6;

Neuron::OverlayQuad QuadOf(SceneTexture _texture, Neuron::OverlayBlend _blend, double _x, double _y, double _side, double _rotation,
                           const std::array<float, 4>& _color) noexcept
{
  return {static_cast<std::uint16_t>(_texture), _blend, static_cast<float>(_x), static_cast<float>(_y), static_cast<float>(_side * 0.5),
          static_cast<float>(_rotation),        _color};
}

std::array<float, 4> Shadow(double _alpha) noexcept
{
  return {0.0f, 0.0f, 0.0f, static_cast<float>(_alpha)};
}

} // namespace

WorldMarkers::WorldMarkers(const TuningPresentation& _presentation) noexcept
  : m_presentation(&_presentation)
{
}

void WorldMarkers::Clear() noexcept
{
  m_markers.clear();
  m_quads.clear();
  m_icons.clear();
  m_underPointer.reset();
}

void WorldMarkers::Update(std::span<const InterpolatedFigure> _figures, std::optional<std::uint8_t> _team,
                          std::optional<StableId> _selected, const MarkerView& _view)
{
  m_quads.clear();
  m_icons.clear();
  m_underPointer.reset();
  if (!_team.has_value())
  {
    m_markers.clear();
    return;
  }
  const auto& marker = m_presentation->ref.ctrl.marker;
  const double dt = std::max(_view.realStepSeconds, 0.0);
  for (Marker& kept : m_markers)
  {
    kept.tracked = false;
  }
  // R-CTRL-381: a Leader marker for each living Leader of the local team, made when it is first seen; and, from M3
  // (ADR-167), of its expedition, whose Leaders leave the settlement's team while they are out (ADR-161).
  for (const InterpolatedFigure& figure : _figures)
  {
    const bool expedition = _view.expeditionTeam.has_value() && figure.state.team == *_view.expeditionTeam;
    if (figure.state.kind != FigureKind::Leader || (figure.state.team != *_team && !expedition) || figure.state.dead)
    {
      continue;
    }
    auto found = std::ranges::find(m_markers, figure.id, &Marker::leader);
    if (found == m_markers.end())
    {
      m_markers.push_back({figure.id, figure.state.team, {}, false, 0.0, 0.0, false});
      found = std::prev(m_markers.end());
    }
    // R-CTRL-382 step 2: the tracked position, smoothed, or taken at once by a new marker.
    const Neuron::Vec3 target = figure.position + UP * marker.lift;
    const double k = std::min(marker.smoothing * dt, 1.0);
    found->position = found->placed ? found->position * (1.0 - k) + target * k : target;
    found->placed = true;
    found->ageSeconds += dt;
    found->tracked = true;
    found->away =
      expedition && _view.areas != nullptr && _view.areas->Known() && !_view.areas->Contains(figure.position.x, figure.position.z);
  }
  std::erase_if(m_markers, [](const Marker& _marker) { return !_marker.tracked; });
  // R-CTRL-382 step 1: one bubble-sort pass, farthest first, so the nearest is drawn last; a marker younger than the new
  // age takes its age as its key, so the newest are drawn over the rest. Every marker is the local team's.
  const auto key = [&](const Marker& _marker)
  {
    return _marker.ageSeconds < marker.newAge ? _marker.ageSeconds
                                              : Neuron::LengthSquared(_marker.position - _view.camera) - marker.ownSortOffset;
  };
  for (std::size_t n = 0; n + 1 < m_markers.size(); ++n)
  {
    if (key(m_markers[n]) < key(m_markers[n + 1]))
    {
      std::swap(m_markers[n], m_markers[n + 1]);
    }
  }
  // ADR-167: the attacks' and claims' markers, under the Leaders'.
  for (const MarkerTarget& target : _view.targets)
  {
    LayTarget(target, _view);
  }
  for (Marker& kept : m_markers)
  {
    const std::optional<Neuron::ScreenPoint> screen = Neuron::ProjectToScreen(kept.position, _view.frame, _view.size);
    const bool onScreen = screen.has_value() && screen->x >= 0.0 && screen->y >= 0.0 &&
                          screen->x <= static_cast<double>(_view.size.width) && screen->y <= static_cast<double>(_view.size.height);
    if (onScreen)
    {
      kept.onScreenSeconds += dt; // R-CTRL-384 rule 1
      // ADR-167: an away Leader has no figure drawn, so its marker stands at it.
      if (kept.away)
      {
        Draw(kept, kept.leader == _selected, screen->x, screen->y, std::nullopt, _view);
      }
    }
    else
    {
      Lay(kept, kept.leader == _selected, _view);
    }
  }
}

void WorldMarkers::Lay(const Marker& _marker, bool _selected, const MarkerView& _view)
{
  // R-CTRL-384 rule 5: a Leader marker shows off screen only until it has been seen; an away one always (ADR-167).
  if (_marker.onScreenSeconds >= m_presentation->ref.ctrl.marker.alpha.seenAfterS && !_marker.away)
  {
    return;
  }
  if (const std::optional<std::array<double, 3>> edge = EdgePoint(_marker.position, _view); edge.has_value())
  {
    Draw(_marker, _selected, (*edge)[0], (*edge)[1], (*edge)[2], _view);
  }
}

std::optional<std::array<double, 3>> WorldMarkers::EdgePoint(const Neuron::Vec3& _position, const MarkerView& _view) const
{
  const auto& marker = m_presentation->ref.ctrl.marker;
  // R-CTRL-383: the probe point from ahead of the camera towards the marker, and the ray from the screen's centre
  // through its projection to the screen's edge; the icon lies inset from there towards the centre.
  const Neuron::Vec3 ahead = _view.camera + _view.cameraFront * marker.edgeProbeAhead;
  const Neuron::Vec3 probe = ahead + Neuron::NormalizedOr(_position - ahead, _view.cameraFront) * marker.edgeProbe;
  const std::optional<Neuron::ScreenPoint> projected = Neuron::ProjectToScreen(probe, _view.frame, _view.size);
  if (!projected.has_value())
  {
    return std::nullopt;
  }
  const double width = static_cast<double>(_view.size.width);
  const double height = static_cast<double>(_view.size.height);
  const double centerX = width * 0.5;
  const double centerY = height * 0.5;
  const double rayX = projected->x - centerX;
  const double rayY = projected->y - centerY;
  const double ray = std::hypot(rayX, rayY);
  if (ray < EDGE_RAY_MIN)
  {
    return std::nullopt;
  }
  double reach = std::numeric_limits<double>::infinity();
  if (rayX != 0.0)
  {
    reach = std::min(reach, ((rayX > 0.0 ? width : 0.0) - centerX) / rayX);
  }
  if (rayY != 0.0)
  {
    reach = std::min(reach, ((rayY > 0.0 ? height : 0.0) - centerY) / rayY);
  }
  const double inwardX = -rayX / ray;
  const double inwardY = -rayY / ray;
  // The compass's arrow points up in its art; it is turned to point out from the centre, towards the marker's place.
  return std::array<double, 3>{centerX + rayX * reach + inwardX * marker.edgeInset, centerY + rayY * reach + inwardY * marker.edgeInset,
                               std::atan2(-inwardX, inwardY)};
}

void WorldMarkers::LayTarget(const MarkerTarget& _target, const MarkerView& _view)
{
  const auto& marker = m_presentation->ref.ctrl.marker;
  const auto& size = marker.size;
  const double height = static_cast<double>(_view.size.height);
  const Neuron::Vec3 place = _target.position + UP * marker.lift;
  // R-CTRL-385: the sizes from the target's height, larger while the camera is near.
  double scale = 1.0;
  const double distance = Neuron::Length(place - _view.camera);
  if (distance < size.nearDistance)
  {
    scale *= 2.0 - distance / size.nearDistance;
  }
  const double icon = height / size.iconDivisor * scale;
  const double compass = icon + height / size.compassDivisor * scale;
  const std::array<float, 4> team = TeamColor(_target.team);
  const std::optional<Neuron::ScreenPoint> screen = Neuron::ProjectToScreen(place, _view.frame, _view.size);
  const bool onScreen = screen.has_value() && screen->x >= 0.0 && screen->y >= 0.0 && screen->x <= static_cast<double>(_view.size.width) &&
                        screen->y <= static_cast<double>(_view.size.height);
  double x = 0.0;
  double y = 0.0;
  std::optional<double> turn;
  if (onScreen)
  {
    x = screen->x;
    y = screen->y;
  }
  else if (const std::optional<std::array<double, 3>> edge = EdgePoint(place, _view); edge.has_value())
  {
    x = (*edge)[0];
    y = (*edge)[1];
    turn = (*edge)[2];
  }
  else
  {
    return;
  }
  // R-CTRL-386, RND-357: the shadows, then the compass, its empty ring on screen, then the disc.
  m_quads.push_back(
    QuadOf(SceneTexture::TeamDiscShadow, Neuron::OverlayBlend::Alpha, x, y, icon + size.shadowPad, 0.0, Shadow(marker.shadowAlpha)));
  m_quads.push_back(
    QuadOf(SceneTexture::EdgeCompassShadow, Neuron::OverlayBlend::Alpha, x, y, compass, turn.value_or(0.0), Shadow(marker.shadowAlpha)));
  m_quads.push_back(QuadOf(turn.has_value() ? SceneTexture::EdgeCompass : SceneTexture::EdgeCompassEmpty, Neuron::OverlayBlend::Additive, x,
                           y, compass, turn.value_or(0.0), team));
  m_quads.push_back(QuadOf(SceneTexture::TeamDisc, Neuron::OverlayBlend::Additive, x, y, icon, 0.0, team));
  m_icons.push_back({static_cast<float>(x), static_cast<float>(y), static_cast<float>(icon), _target.icon});
}

void WorldMarkers::Draw(const Marker& _marker, bool _selected, double _iconX, double _iconY, std::optional<double> _turn,
                        const MarkerView& _view)
{
  const auto& marker = m_presentation->ref.ctrl.marker;
  const double height = static_cast<double>(_view.size.height);
  // R-CTRL-385: the sizes from the target's height, larger while the camera is near, and while hovered or selected.
  const auto& size = marker.size;
  double scale = 1.0;
  const double distance = Neuron::Length(_marker.position - _view.camera);
  if (distance < size.nearDistance)
  {
    scale *= 2.0 - distance / size.nearDistance;
  }
  double icon = height / size.iconDivisor * scale;
  const bool hovered = std::abs(static_cast<double>(_view.pointer.x) - _iconX) <= icon * 0.5 &&
                       std::abs(static_cast<double>(_view.pointer.y) - _iconY) <= icon * 0.5;
  if (hovered || _selected)
  {
    scale *= size.hoverScale;
    icon = height / size.iconDivisor * scale;
  }
  if (hovered)
  {
    m_underPointer = _marker.leader; // the last drawn, on top, wins
  }
  const double shadow = icon + size.shadowPad;
  const double compass = icon + height / size.compassDivisor * scale;
  const std::array<float, 4> team = TeamColor(_marker.team);
  m_quads.push_back(
    QuadOf(SceneTexture::TeamDiscShadow, Neuron::OverlayBlend::Alpha, _iconX, _iconY, shadow, 0.0, Shadow(marker.shadowAlpha)));
  if (_turn.has_value())
  {
    m_quads.push_back(
      QuadOf(SceneTexture::EdgeCompassShadow, Neuron::OverlayBlend::Alpha, _iconX, _iconY, compass, *_turn, Shadow(marker.shadowAlpha)));
    m_quads.push_back(QuadOf(SceneTexture::EdgeCompass, Neuron::OverlayBlend::Additive, _iconX, _iconY, compass, *_turn, team));
  }
  m_quads.push_back(QuadOf(SceneTexture::TeamDisc, Neuron::OverlayBlend::Additive, _iconX, _iconY, icon, 0.0, team));
  m_quads.push_back(QuadOf(SceneTexture::IconLeader, Neuron::OverlayBlend::Additive, _iconX, _iconY, icon, 0.0, WHITE));
}

std::vector<MarkerTarget> ConflictTargetsOf(const ValleyStatus& _valley, std::uint8_t _site, const StateReplica::View& _latest,
                                            const RegionTerrain& _terrain, const TuningMvp& _tuning)
{
  std::vector<MarkerTarget> targets;
  const auto claimOf = [&](StableId _id) { return std::ranges::find(_valley.claims, _id, &ClaimLine::id); };
  // MVP-043: each battle the steward's settlement is a side of, at its objective, in the other side's colour.
  for (const BattleLine& battle : _valley.battles)
  {
    if (battle.side)
    {
      targets.push_back({PositionOf(battle.objective), battle.attacker == _site ? battle.defender : battle.attacker, TargetIcon::Attack});
    }
  }
  for (const ExpeditionLine& expedition : _valley.expeditions)
  {
    if (expedition.state == ExpeditionState::Fighting)
    {
      continue; // its battle's marker stands for it
    }
    if (expedition.target == _site)
    {
      // MVP-042: an incoming expedition, where the replica holds its figures, which the seen layer decides (ADR-158).
      const std::uint8_t team = ExpeditionTeamOf(expedition.attacker);
      Neuron::Vec3 sum{};
      std::size_t count = 0;
      for (const auto& [id, state] : _latest)
      {
        if (const auto* figure = std::get_if<ReplicatedFigure>(&state); figure != nullptr && figure->team == team && !figure->dead)
        {
          sum = sum + PositionOf(figure->position);
          ++count;
        }
      }
      if (count > 0)
      {
        targets.push_back({sum * (1.0 / static_cast<double>(count)), expedition.attacker, TargetIcon::Attack});
      }
    }
    else if (expedition.attacker == _site)
    {
      // Its own expedition's target: the claimed area, or the target's Wellspring, which is public (ADR-158).
      std::optional<Neuron::Vec3> place;
      if (const auto claim = claimOf(expedition.claim); expedition.claim != 0 && claim != _valley.claims.end())
      {
        place = ClaimAreaCenter(_terrain.layout, _terrain.field, _tuning, claim->area);
      }
      for (auto it = _latest.begin(); !place.has_value() && it != _latest.end(); ++it)
      {
        const auto* structure = std::get_if<ReplicatedStructure>(&it->second);
        if (structure != nullptr && structure->kind == StructureKind::Wellspring && structure->owner == expedition.target)
        {
          place = PositionOf(structure->position);
        }
      }
      if (place.has_value())
      {
        targets.push_back({*place, expedition.target, TargetIcon::Attack});
      }
    }
  }
  // MVP-053: its own claims, and the claims on the areas it holds, in the claimant's colour.
  for (const ClaimLine& claim : _valley.claims)
  {
    const bool onOurs = claim.area < VALLEY_AREAS && _valley.areas[claim.area].holder == std::optional<std::uint8_t>(_site);
    if (claim.claimant != _site && !onOurs)
    {
      continue;
    }
    if (const std::optional<Neuron::Vec3> place = ClaimAreaCenter(_terrain.layout, _terrain.field, _tuning, claim.area); place.has_value())
    {
      targets.push_back({*place, claim.claimant, TargetIcon::Claim});
    }
  }
  return targets;
}

std::vector<Neuron::OverlayQuad> CursorBadge(bool _leaderSelected, bool _workersSelected, std::uint8_t _team, Neuron::PointPx _pointer,
                                             Neuron::SizePx _size, const TuningPresentation& _presentation)
{
  std::vector<Neuron::OverlayQuad> quads;
  if (!_leaderSelected && !_workersSelected)
  {
    return quads;
  }
  // R-CTRL-366: the icon's centre lies the offset and the centre offset along n from the pointer; its size is the
  // target's height ÷ the divisor, drawn in the world markers' layers (R-CTRL-386).
  const auto& cursor = _presentation.ref.ctrl.cursor;
  const double length = std::hypot(BADGE_DIRECTION_X, BADGE_DIRECTION_Y);
  const double along = cursor.iconOffset + cursor.iconCenterOffset;
  const double x = static_cast<double>(_pointer.x) + BADGE_DIRECTION_X / length * along;
  const double y = static_cast<double>(_pointer.y) + BADGE_DIRECTION_Y / length * along;
  const double icon = static_cast<double>(_size.height) / cursor.iconDivisor;
  const auto& marker = _presentation.ref.ctrl.marker;
  quads.push_back(
    QuadOf(SceneTexture::TeamDiscShadow, Neuron::OverlayBlend::Alpha, x, y, icon + marker.size.shadowPad, 0.0, Shadow(marker.shadowAlpha)));
  quads.push_back(QuadOf(SceneTexture::TeamDisc, Neuron::OverlayBlend::Additive, x, y, icon, 0.0, TeamColor(_team)));
  // Row 2 before row 3: a selected Leader, then selected Workers.
  quads.push_back(
    QuadOf(_leaderSelected ? SceneTexture::IconLeader : SceneTexture::IconWorkers, Neuron::OverlayBlend::Additive, x, y, icon, 0.0, WHITE));
  return quads;
}

std::optional<BadgeLabel> BadgeCount(std::span<const Neuron::OverlayQuad> _badge, std::size_t _workers)
{
  if (_badge.empty() || _workers == 0)
  {
    return std::nullopt;
  }
  const Neuron::OverlayQuad& shadow = _badge.front();
  return BadgeLabel{std::to_string(_workers), shadow.centerX + shadow.halfSize, shadow.centerY};
}

} // namespace Species
