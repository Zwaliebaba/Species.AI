#pragma once

#include "FrameView.h"
#include "RegionTerrain.h"
#include "RelationMessages.h"
#include "ReplicaTiming.h"
#include "StableId.h"
#include "StateReplica.h"
#include "StewardAreas.h"
#include "TuningPresentation.h"
#include "Vec3.h"
#include "WindowGeometry.h"
#include "WorldScene.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace Species
{

// ADR-167: the icon of an attack's or a claim's world marker, which the HUD draws over its disc (icon.hud.attack,
// icon.hud.claim).
enum class TargetIcon : std::uint8_t
{
  Attack,
  Claim
};

/// ADR-167: an attack's or a claim's world marker: the place it marks, the team whose colour its disc takes, and its icon.
struct MarkerTarget
{
  Neuron::Vec3 position;
  std::uint8_t team = 0;
  TargetIcon icon = TargetIcon::Attack;

  friend bool operator==(const MarkerTarget&, const MarkerTarget&) = default;
};

/// ADR-167: where a target marker's icon lies on the overlay's target, its centre and side in physical pixels.
struct MarkerIconPlace
{
  float x = 0.0f;
  float y = 0.0f;
  float side = 0.0f;
  TargetIcon icon = TargetIcon::Attack;
};

// MVP-042, MVP-053 (ADR-167): the world markers of the valley status _valley for the steward's settlement at site _site:
// - each battle it is a side of, at the battle's objective, in the other side's colour;
// - each attack declared on it and not yet fought, at its expedition's figures that the replica's tick _latest holds;
// - its own expedition not yet fighting, at its target: the claimed area's centre, or the target's Wellspring;
// - each claim it made, or made on an area it holds, at the area's centre, in the claimant's colour.
[[nodiscard]] std::vector<MarkerTarget> ConflictTargetsOf(const ValleyStatus& _valley, std::uint8_t _site,
                                                          const StateReplica::View& _latest, const RegionTerrain& _terrain,
                                                          const TuningMvp& _tuning);

/// What the world markers read of a frame (ADR-125): the camera's view, the overlay's target in physical pixels, the
/// pointer on it, and the real frame step; from M3 task T12 (ADR-167), the team of the steward's expedition and the
/// steward's areas, outside which its Leaders are away.
struct MarkerView
{
  Neuron::FrameView frame;
  Neuron::Vec3 camera;
  Neuron::Vec3 cameraFront{0.0, 0.0, -1.0}; // a unit vector
  Neuron::SizePx size;
  Neuron::PointPx pointer;
  double realStepSeconds = 0.0;
  std::optional<std::uint8_t> expeditionTeam;
  const StewardAreas* areas = nullptr; // none: no Leader is away
  std::span<const MarkerTarget> targets;
};

/// The world markers of M1 (M1 task T17b, ADR-125; R-CTRL-380 to R-CTRL-388, RND-356, RND-357): the Leader markers, one
/// for each of the local team's living Leaders, made when the Leader is first seen and removed with it, the only type
/// whose source M1 has. Each tracks its Leader lifted by ref.ctrl.marker.lift, smoothed (R-CTRL-382), and counts its time
/// on screen. On screen its icon is the Leader's banner, which pass 7b draws (R-CTRL-388 as amended); off screen, until
/// it has been seen (R-CTRL-384), it is held at the screen's edge (R-CTRL-383) and drawn in pass 16 as the shadow, the
/// compass pointing towards its Leader, the team disc and icon.power.leader in place of the banner (R-CTRL-386). A
/// marker whose icon box holds the pointer is the marker under the pointer, which selects as the Leader does
/// (R-CTRL-387). From M3 task T12 (O98, ADR-167) the steward's expedition's Leaders have markers too: while one is away,
/// outside the steward's areas and so under the fog with no figure drawn, its marker is drawn at its Leader on screen,
/// with no compass, and held at the screen's edge off screen however long it has been seen. The attacks' and claims'
/// markers are the team disc at their place on screen and held at the screen's edge with the compass off screen, always,
/// under the Leaders'; the HUD draws their icons where IconPlaces says. Frame thread only.
class WorldMarkers
{
public:
  // _presentation outlives the markers.
  explicit WorldMarkers(const TuningPresentation& _presentation) noexcept;

  // Once per frame: _figures' Leaders of _team (none: no markers), _selected the team's selected Leader.
  void Update(std::span<const InterpolatedFigure> _figures, std::optional<std::uint8_t> _team, std::optional<StableId> _selected,
              const MarkerView& _view);
  // A new world: no markers (R-CTRL-380).
  void Clear() noexcept;

  // Pass 16's quads, nearest marker last (R-CTRL-382).
  [[nodiscard]] const std::vector<Neuron::OverlayQuad>& Quads() const noexcept
  {
    return m_quads;
  }
  // ADR-167: the attacks' and claims' icons of this frame, which the HUD draws over their discs.
  [[nodiscard]] const std::vector<MarkerIconPlace>& IconPlaces() const noexcept
  {
    return m_icons;
  }
  // R-CTRL-387: the Leader of the marker under the pointer at the end of the frame.
  [[nodiscard]] std::optional<StableId> UnderPointer() const noexcept
  {
    return m_underPointer;
  }
  [[nodiscard]] std::size_t Count() const noexcept
  {
    return m_markers.size();
  }

private:
  struct Marker
  {
    StableId leader = 0;
    std::uint8_t team = 0;
    Neuron::Vec3 position;
    bool placed = false; // its position has been set once, so it smooths from there
    double ageSeconds = 0.0;
    double onScreenSeconds = 0.0;
    bool tracked = false; // its Leader was among this frame's figures
    bool away = false;    // ADR-167: an expedition's Leader outside the steward's areas
  };

  void Lay(const Marker& _marker, bool _selected, const MarkerView& _view);
  // ADR-167: a target's marker at its place on screen or at the edge.
  void LayTarget(const MarkerTarget& _target, const MarkerView& _view);
  // R-CTRL-383: the point inset from the screen's edge on the ray from its centre towards _position, and the compass's
  // turn there; none when the probe does not project.
  [[nodiscard]] std::optional<std::array<double, 3>> EdgePoint(const Neuron::Vec3& _position, const MarkerView& _view) const;
  // The marker's quads with its icon at the screen point (_iconX, _iconY), and the compass turned by _turn out from the
  // centre when it has one.
  void Draw(const Marker& _marker, bool _selected, double _iconX, double _iconY, std::optional<double> _turn, const MarkerView& _view);

  const TuningPresentation* m_presentation;
  std::vector<Marker> m_markers; // in drawing order
  std::vector<Neuron::OverlayQuad> m_quads;
  std::vector<MarkerIconPlace> m_icons;
  std::optional<StableId> m_underPointer;
};

// R-CTRL-366 rows 2 and 3 (ADR-125): the badge near the pointer while a Leader or Workers are selected, icon.power.leader
// or icon.power.workers on the team's disc over its shadow, the icon's centre down and to the right of the pointer;
// none while nothing is selected.
[[nodiscard]] std::vector<Neuron::OverlayQuad> CursorBadge(bool _leaderSelected, bool _workersSelected, std::uint8_t _team,
                                                           Neuron::PointPx _pointer, Neuron::SizePx _size,
                                                           const TuningPresentation& _presentation);

/// R-CTRL-366's count (ADR-127, ADR-145): its text, and the point in physical pixels where its line's left edge and
/// vertical middle lie; the UI lays it out in the body role (ADR-147).
struct BadgeLabel
{
  std::string text;
  float left = 0.0f;
  float middle = 0.0f;
};

// The selected Workers' count, its line's left edge at the right edge of the badge's shadow, its first layer, and its
// middle level with the badge's centre; none for no badge or no Worker.
[[nodiscard]] std::optional<BadgeLabel> BadgeCount(std::span<const Neuron::OverlayQuad> _badge, std::size_t _workers);

} // namespace Species
