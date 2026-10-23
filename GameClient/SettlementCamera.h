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

/// The camera's controls in one frame (R-CTRL-460, R-CTRL-461, R-CTRL-471, R-CTRL-472): the actions of table 2.1-A rows
/// A1 to A3, A5 and A6 as the input router answered them.
struct CameraControls
{
  bool panLeft = false;
  bool panRight = false;
  bool panForward = false;
  bool panBack = false;
  bool fastPan = false;
  bool raise = false;
  bool lower = false;
  bool zoom = false;
  bool lookAround = false;
  bool look = false;         // O107 (ADR-179): action.camera.look held, the camera turns towards the pointer
  float wheelNotches = 0.0f; // action.camera.height_wheel, forward positive
};

/// The horizontal square that bounds the camera (R-CTRL-471 step 10), in world units.
struct CameraBounds
{
  double minX = -1'000'000.0; // ref.ctrl.camera.initial_pose.bounds until a world is entered (R-CTRL-452)
  double maxX = 1'000'000.0;
  double minZ = -1'000'000.0;
  double maxZ = 1'000'000.0;
};

/// The player camera of the settlement view (`MatchCamera`, R-CTRL-450 to R-CTRL-497; ADR-119), in the modes the MVP
/// reaches: static until a world is entered, then free movement. It holds a front vector and rebuilds its up vector
/// from world up each frame, since nothing rolls it at M1. The forward and back keys fly it along its front (O90,
/// ADR-142), and in a settlement world the steward's areas bound it (WLD-244). It turns towards the pointer only while the
/// middle button is held (O107, ADR-179), so the pointer is free for the HUD otherwise. It is local to the client and never sent
/// (DEC-008), and it reads the terrain only through the client's own field. Frame thread only.
class SettlementCamera
{
public:
  SettlementCamera(const TuningPresentation& _presentation, const Neuron::ProjectionPlanes& _planes) noexcept;

  // CAM-060 as amended (O55, O76): the bounds become the region's square _region ± ref.sim.match_entry.camera_border;
  // the camera cuts to fallback_camera_height along fallback_camera_direction, placed so that the screen's centre falls
  // on _siteCenter; it enters free movement, and the pointer is asked to the screen centre (R-CTRL-470).
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

  /// What an advance asks of the pointer: where the cursor lock or the entry moved it (R-CTRL-464, R-CTRL-470).
  struct PointerRequest
  {
    std::optional<Neuron::PointPx> warpTo;
  };

  // R-CTRL-456: one rendered frame's advance, with the real frame step _dtSeconds (clamped to ref.sim.frame.delta_clamp,
  // CAM-030), the frame's controls, the pointer in client pixels of a view of _view, and the region's terrain. Nothing
  // moves before a world is entered. The pointer ground point is recomputed last (step 7). With _areas known, the
  // camera's ground point stays inside them as well as inside the bounds (WLD-244, ADR-142).
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
  [[nodiscard]] double Height() const noexcept
  {
    return m_height;
  }
  [[nodiscard]] const Neuron::Vec3& Target() const noexcept
  {
    return m_target;
  }

  // R-CTRL-058 again, for the pointer and view of this frame: the ground point as the camera now stands.
  void UpdatePointerGround(Neuron::PointPx _pointer, Neuron::SizePx _view, const Neuron::TerrainField& _terrain);

private:
  [[nodiscard]] double Land(double _x, double _z, const Neuron::TerrainField& _terrain) const noexcept;
  void Zoom(double _dt, const CameraControls& _controls) noexcept;
  void AdjustHeight(double _dt, const CameraControls& _controls, const Neuron::TerrainField& _terrain) noexcept;
  void FreeMovement(double _dt, const CameraControls& _controls, const Neuron::TerrainField& _terrain) noexcept;
  void MouseLook(double _dt) noexcept;

  const TuningPresentation& m_presentation;
  Neuron::ProjectionPlanes m_planes;
  CameraBounds m_bounds;
  CameraBounds m_region;                            // the region's own square, which the far sphere of R-CTRL-058 is centred on
  Neuron::Vec3 m_position{1'000.0, 500.0, 1'000.0}; // ref.ctrl.camera.initial_pose (R-CTRL-452)
  Neuron::Vec3 m_front{0.0, -0.4472135954999579, -0.8944271909999159};
  Neuron::Vec3 m_target{1'000.0, 500.0, 1'000.0};
  Neuron::Vec3 m_velocity;
  double m_yawRadians = 0.0; // the last yaw of a front that was not vertical (CAM-034)
  double m_height = 50.0;
  double m_fovDegrees = 60.0;
  double m_targetFovDegrees = 60.0;
  bool m_inWorld = false;
  bool m_centerPointer = false;
  std::optional<GroundPoint> m_ground;
  double m_lockRemainderX = 0.0; // R-CTRL-464's sub-pixel remainders
  double m_lockRemainderY = 0.0;
};

} // namespace Species
