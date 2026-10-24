#pragma once

#include "FrameView.h"
#include "Picking.h"
#include "TerrainField.h"
#include "TuningPresentation.h"
#include "Vec3.h"
#include "WindowGeometry.h"

#include <optional>

namespace Species
{

class StewardAreas;

/// The camera's controls in one frame (R-CTRL-460, R-CTRL-461, R-CTRL-471, R-CTRL-472 as O108 amends them): the actions
/// of table 3.6-A rows 1 to 4, 6 to 9, 12, 13 and 67 to 73 as the input router answered them.
struct CameraControls
{
  bool panLeft = false;
  bool panRight = false;
  bool panForward = false;
  bool panBack = false;
  bool fastPan = false;
  bool raise = false; // O108 (ADR-180): action.camera.raise, Page Up held, zooms out
  bool lower = false; // action.camera.lower, Page Down held, zooms in
  bool zoom = false;
  bool lookAround = false;
  bool turnLeft = false;        // action.camera.turn_left, Q held
  bool turnRight = false;       // action.camera.turn_right, E held
  bool drag = false;            // action.camera.drag, the middle button held: the ground under the pointer is held
  bool turnDrag = false;        // action.camera.turn_drag, Alt and the middle button held: the pointer's movement turns
  bool home = false;            // action.camera.home, Home pressed this frame
  bool resetHeading = false;    // action.camera.reset_heading, Backspace pressed this frame
  float wheelNotches = 0.0f;    // action.camera.height_wheel, forward positive: closer
  float panWheelNotches = 0.0f; // action.camera.pan_wheel, the horizontal wheel, right positive
};

/// The horizontal square that bounds the camera's ground point (R-CTRL-471 step 10 as O108 amends it), in world units.
struct CameraBounds
{
  double minX = -1'000'000.0; // ref.ctrl.camera.initial_pose.bounds until a world is entered (R-CTRL-452)
  double maxX = 1'000'000.0;
  double minZ = -1'000'000.0;
  double maxZ = 1'000'000.0;
};

/// The player camera of the settlement view (`MatchCamera`, R-CTRL-450 to R-CTRL-497; ADR-119), held over a point on
/// the ground since O108 (ADR-180): static until a world is entered, then the ground point it looks at, its height above
/// that point and its heading, each closing on its goal. Its tilt follows its height, and its position is the ground
/// point less its front × the distance that gives the height. In a settlement world the steward's areas bound the ground
/// point (WLD-244). The pointer is free; only looking around (R-CTRL-472) turns the front towards it, with the cursor lock.
/// It is local to the client and never sent (DEC-008), and it reads the terrain only through the client's own field.
/// Frame thread only.
class SettlementCamera
{
public:
  SettlementCamera(const TuningPresentation& _presentation, const Neuron::ProjectionPlanes& _planes) noexcept;

  // CAM-060 as amended (O55, O76, O108): the bounds become the region's square _region ± ref.sim.match_entry.camera_border;
  // the camera is held over _siteCenter at ref.sim.match_entry.fallback_camera_height, with the heading of
  // fallback_camera_direction, and that pose is its home; the pointer is asked to the screen center (R-CTRL-470).
  void EnterWorld(const Neuron::TerrainField& _terrain, const Neuron::Vec3& _siteCenter, const CameraBounds& _region);
  [[nodiscard]] const CameraBounds& Bounds() const noexcept
  {
    return m_bounds;
  }
  // CAM-063: the world is left; the camera holds its pose in the static mode.
  void LeaveWorld() noexcept;
  [[nodiscard]] bool InWorld() const noexcept
  {
    return m_inWorld;
  }
  // ADR-126 as ADR-180 keeps it: the frame bench's view, O76's pose at ref.sim.match_entry.fallback_camera_height along
  // fallback_camera_direction onto _ground, held until the next advance; the bench never advances its camera.
  void HoldEntryPose(const Neuron::TerrainField& _terrain, const Neuron::Vec3& _ground) noexcept;

  /// What an advance asks of the pointer: where the cursor lock or the entry moved it (R-CTRL-464, R-CTRL-470).
  struct PointerRequest
  {
    std::optional<Neuron::PointPx> warpTo;
  };

  // R-CTRL-456: one rendered frame's advance, with the real frame step _dtSeconds (clamped to ref.sim.frame.delta_clamp,
  // CAM-030), the frame's controls, the pointer in client pixels of a view of _view, and the region's terrain. Nothing
  // moves before a world is entered. The pointer ground point is recomputed last. With _areas known, the ground point
  // stays inside them as well as inside the bounds (WLD-244, ADR-142).
  PointerRequest Advance(double _dtSeconds, const CameraControls& _controls, Neuron::PointPx _pointer, Neuron::SizePx _view,
                         const Neuron::TerrainField& _terrain, const StewardAreas* _areas = nullptr);

  // R-CTRL-058: the click ray through the pointer met with the terrain; when it misses, the inside of the far sphere
  // about the region (DEC-864 item 1). It also says whether the terrain was hit.
  struct GroundPoint
  {
    Neuron::Vec3 position;
    bool onTerrain = false;
  };
  [[nodiscard]] const std::optional<GroundPoint>& PointerGround() const noexcept
  {
    return m_ground;
  }

  [[nodiscard]] Neuron::CameraPose Pose() const noexcept;
  [[nodiscard]] Neuron::FrameView View(Neuron::SizePx _view) const noexcept;
  [[nodiscard]] const Neuron::Vec3& Position() const noexcept
  {
    return m_position;
  }
  [[nodiscard]] const Neuron::Vec3& Front() const noexcept
  {
    return m_front;
  }
  [[nodiscard]] double FieldOfViewDegrees() const noexcept
  {
    return m_fovDegrees;
  }
  // O108 (ADR-180): the point on the ground the camera is held over, its height above that point, and its heading, the
  // yaw of its front about world up.
  [[nodiscard]] const Neuron::Vec3& Focus() const noexcept
  {
    return m_focus;
  }
  [[nodiscard]] double Height() const noexcept
  {
    return m_height;
  }
  [[nodiscard]] double HeadingRadians() const noexcept
  {
    return m_yawRadians;
  }
  // The tilt below the horizon that a height gives: cam.orbit.pitch_low at cam.orbit.height_min to cam.orbit.pitch_high
  // at ref.ctrl.camera.max_height, by the share of the height's logarithm between them.
  [[nodiscard]] double PitchRadiansAt(double _height) const noexcept;

  // R-CTRL-058 again, for the pointer and view of this frame: the ground point as the camera now stands.
  void UpdatePointerGround(Neuron::PointPx _pointer, Neuron::SizePx _view, const Neuron::TerrainField& _terrain);

private:
  struct ZoomAnchor
  {
    Neuron::PointPx pixel;
    Neuron::Vec3 ground;
  };

  [[nodiscard]] double Land(double _x, double _z, const Neuron::TerrainField& _terrain) const noexcept;
  [[nodiscard]] double Distance() const noexcept;
  [[nodiscard]] double ClampHeight(double _height) const noexcept;
  void Zoom(double _dt, const CameraControls& _controls) noexcept;
  void Steer(double _dt, const CameraControls& _controls, Neuron::PointPx _pointer, Neuron::SizePx _view,
             const Neuron::TerrainField& _terrain);
  void Ease(double _dt, const Neuron::TerrainField& _terrain) noexcept;
  void Place(const Neuron::TerrainField& _terrain) noexcept;
  void HoldUnder(const Neuron::Vec3& _ground, Neuron::PointPx _pixel, Neuron::SizePx _view, const Neuron::TerrainField& _terrain) noexcept;
  void Bound(const Neuron::Vec3& _startFocus, const Neuron::Vec3& _startGoal, const StewardAreas* _areas);
  void LookAround(double _dt) noexcept;

  const TuningPresentation& m_presentation;
  Neuron::ProjectionPlanes m_planes;
  CameraBounds m_bounds;
  CameraBounds m_region;                            // the region's own square, which the far sphere of R-CTRL-058 is centred on
  Neuron::Vec3 m_position{1'000.0, 500.0, 1'000.0}; // ref.ctrl.camera.initial_pose (R-CTRL-452)
  Neuron::Vec3 m_front{0.0, -0.4472135954999579, -0.8944271909999159};
  Neuron::Vec3 m_orbitFront{0.0, -0.4472135954999579, -0.8944271909999159}; // the front onto the ground point
  Neuron::Vec3 m_focus;                                                     // y: the ground's height under it, eased
  Neuron::Vec3 m_focusGoal;
  double m_height = 50.0;
  double m_heightGoal = 50.0;
  double m_yawRadians = 0.0;
  double m_yawGoal = 0.0;
  Neuron::Vec3 m_homeFocus; // the entry pose, which Home and the heading's reset return to
  double m_homeHeight = 50.0;
  double m_homeYaw = 0.0;
  Neuron::Vec3 m_lookFront{0.0, -0.4472135954999579, -0.8944271909999159}; // looking around's front (R-CTRL-472)
  double m_lookBlend = 0.0;                                                // 1 while looking around, closing on 0 after it
  double m_lookYaw = 0.0;                                                  // the last yaw of a look front that was not vertical (CAM-034)
  std::optional<Neuron::Vec3> m_grab;                                      // the ground held under the pointer by action.camera.drag
  std::optional<ZoomAnchor> m_zoomAnchor;                                  // the ground the wheel zooms towards, held under its pixel
  std::optional<Neuron::PointPx> m_lastPointer;
  double m_fovDegrees = 60.0;
  double m_targetFovDegrees = 60.0;
  bool m_inWorld = false;
  bool m_centerPointer = false;
  std::optional<GroundPoint> m_ground;
  double m_lockRemainderX = 0.0; // R-CTRL-464's sub-pixel remainders
  double m_lockRemainderY = 0.0;
};

} // namespace Species
