#include "pch.h"

#include "Geometry.h"
#include "SettlementCamera.h"
#include "StewardAreas.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <numbers>

namespace Species
{

namespace
{

constexpr double RADIANS_PER_DEGREE = std::numbers::pi / 180.0;
constexpr double VERTICAL_FRONT_LIMIT = 1.0 - 1e-9; // CAM-034: a front this close to world up keeps the previous yaw
// ADR-119: looking around, the front stays within 89° of the horizontal. The reference states no limit, but a front
// through the vertical reverses its own ground projection each frame it crosses.
constexpr double STEEPEST_FRONT_DEGREES = 89.0;
// ADR-180: the look-around's return ends below this share, and a zoom anchor is let go once the height is within this
// share of its goal.
constexpr double LOOK_BLEND_END = 1e-3;
constexpr double ZOOM_SETTLED_SHARE = 1e-3;

// ADR-180: the level front of a heading, the yaw about world up, 0 looking along −z (CameraPose's convention).
Neuron::Vec3 HeadingOf(double _yawRadians) noexcept
{
  return {-std::sin(_yawRadians), 0.0, -std::cos(_yawRadians)};
}

// An angle brought into [−π, π].
double Wrapped(double _radians) noexcept
{
  return std::remainder(_radians, 2.0 * std::numbers::pi);
}

// ADR-180: where a ray meets the level plane at _height ahead of its start; none when it runs along the plane or away.
std::optional<Neuron::Vec3> OnLevel(const Neuron::Ray& _ray, double _height) noexcept
{
  if (std::abs(_ray.direction.y) < 1e-9)
  {
    return std::nullopt;
  }
  const double along = (_height - _ray.origin.y) / _ray.direction.y;
  if (along <= 0.0)
  {
    return std::nullopt;
  }
  return _ray.origin + _ray.direction * along;
}

// ADR-142: a move from _from, which is inside the areas, to _point, which may not be, slides along whichever axis keeps it
// inside, or stays at _from; a point already outside goes to the nearest point of the areas.
void KeepInside(const StewardAreas& _areas, Neuron::Vec3& _point, const Neuron::Vec3& _from)
{
  if (_areas.Contains(_point.x, _point.z))
  {
    return;
  }
  if (_areas.Contains(_from.x, _from.z))
  {
    if (_areas.Contains(_point.x, _from.z))
    {
      _point.z = _from.z;
    }
    else if (_areas.Contains(_from.x, _point.z))
    {
      _point.x = _from.x;
    }
    else
    {
      _point.x = _from.x;
      _point.z = _from.z;
    }
    return;
  }
  if (const std::optional<StewardAreas::Point> nearest = _areas.Nearest(_point.x, _point.z))
  {
    _point.x = nearest->x;
    _point.z = nearest->z;
  }
}

// Rodrigues' rotation of _vector about the unit _axis by _angle radians.
Neuron::Vec3 Rotate(const Neuron::Vec3& _vector, const Neuron::Vec3& _axis, double _angle) noexcept
{
  const double cosine = std::cos(_angle);
  const double sine = std::sin(_angle);
  return _vector * cosine + Neuron::Cross(_axis, _vector) * sine + _axis * (Neuron::Dot(_axis, _vector) * (1.0 - cosine));
}

} // namespace

SettlementCamera::SettlementCamera(const TuningPresentation& _presentation, const Neuron::ProjectionPlanes& _planes) noexcept
  : m_presentation(_presentation),
    m_planes(_planes),
    m_fovDegrees(_presentation.ref.ctrl.camera.fovBase),
    m_targetFovDegrees(_presentation.ref.ctrl.camera.fovBase)
{
}

void SettlementCamera::EnterWorld(const Neuron::TerrainField& _terrain, const Neuron::Vec3& _siteCenter, const CameraBounds& _region)
{
  const TuningPresentation::Ref::Sim::MatchEntry& entry = m_presentation.ref.sim.matchEntry;
  m_region = _region;
  m_bounds = {_region.minX - entry.cameraBorder, _region.maxX + entry.cameraBorder, _region.minZ - entry.cameraBorder,
              _region.maxZ + entry.cameraBorder};
  // O76 as O108 amends it (ADR-180): held over the site's center at the fallback height, with the heading of the fallback
  // direction; that pose is the camera's home.
  const Neuron::Vec3 direction = Neuron::NormalizedOr(
    {entry.fallbackCameraDirection.x, entry.fallbackCameraDirection.y, entry.fallbackCameraDirection.z}, {0.0, -1.0, 0.0});
  m_homeYaw = std::abs(direction.y) < VERTICAL_FRONT_LIMIT ? std::atan2(-direction.x, -direction.z) : 0.0;
  m_homeFocus = {_siteCenter.x, Land(_siteCenter.x, _siteCenter.z, _terrain), _siteCenter.z};
  m_homeHeight = ClampHeight(entry.fallbackCameraHeight);
  m_focus = m_homeFocus;
  m_focusGoal = m_homeFocus;
  m_height = m_homeHeight;
  m_heightGoal = m_homeHeight;
  m_yawRadians = m_homeYaw;
  m_yawGoal = m_homeYaw;
  // R-CTRL-470: the base field of view, and the pointer to the screen center.
  m_fovDegrees = m_presentation.ref.ctrl.camera.fovBase;
  m_targetFovDegrees = m_presentation.ref.ctrl.camera.fovBase;
  m_lookBlend = 0.0;
  m_grab.reset();
  m_zoomAnchor.reset();
  m_lastPointer.reset();
  Place(_terrain);
  m_lookFront = m_front;
  m_lookYaw = m_yawRadians;
  m_ground.reset();
  m_lockRemainderX = 0.0;
  m_lockRemainderY = 0.0;
  m_inWorld = true;
  m_centerPointer = true;
}

void SettlementCamera::LeaveWorld() noexcept
{
  m_inWorld = false;
  m_centerPointer = false;
  m_grab.reset();
  m_zoomAnchor.reset();
  m_ground.reset();
}

void SettlementCamera::HoldEntryPose(const Neuron::TerrainField& _terrain, const Neuron::Vec3& _ground) noexcept
{
  const TuningPresentation::Ref::Sim::MatchEntry& entry = m_presentation.ref.sim.matchEntry;
  const Neuron::Vec3 direction = Neuron::NormalizedOr(
    {entry.fallbackCameraDirection.x, entry.fallbackCameraDirection.y, entry.fallbackCameraDirection.z}, {0.0, -1.0, 0.0});
  const Neuron::Vec3 ground{_ground.x, _terrain.Height(_ground.x, _ground.z), _ground.z};
  const double along = direction.y < -1e-6 ? (entry.fallbackCameraHeight - ground.y) / -direction.y : 0.0;
  m_position = ground - direction * along;
  m_front = direction;
}

SettlementCamera::PointerRequest SettlementCamera::Advance(double _dtSeconds, const CameraControls& _controls, Neuron::PointPx _pointer,
                                                           Neuron::SizePx _view, const Neuron::TerrainField& _terrain,
                                                           const StewardAreas* _areas)
{
  PointerRequest request;
  if (!m_inWorld)
  {
    return request;
  }
  const double dt = std::clamp(_dtSeconds, 0.0, m_presentation.ref.sim.frame.deltaClamp);
  if (m_centerPointer)
  {
    _pointer = {static_cast<std::int32_t>(_view.width / 2), static_cast<std::int32_t>(_view.height / 2)};
    request.warpTo = _pointer;
    m_centerPointer = false;
    UpdatePointerGround(_pointer, _view, _terrain);
  }
  // R-CTRL-460: the zoom of the field of view, skipped while looking around.
  if (!_controls.lookAround)
  {
    Zoom(dt, _controls);
  }
  if (_controls.lookAround)
  {
    // R-CTRL-472 as O108 amends it: the camera holds still over its ground point, and its front alone turns towards the
    // point under the pointer, which the cursor lock keeps under it (R-CTRL-464).
    if (m_lookBlend < 1.0)
    {
      m_lookFront = m_front;
      if (std::abs(m_front.y) < VERTICAL_FRONT_LIMIT)
      {
        m_lookYaw = std::atan2(-m_front.x, -m_front.z);
      }
      m_lookBlend = 1.0;
    }
    m_grab.reset();
    m_zoomAnchor.reset();
    const std::optional<Neuron::ScreenPoint> before =
      m_ground ? Neuron::ProjectToScreen(m_ground->position, View(_view), _view) : std::nullopt;
    LookAround(dt);
    m_front = m_lookFront;
    if (before && m_ground)
    {
      if (const std::optional<Neuron::ScreenPoint> after = Neuron::ProjectToScreen(m_ground->position, View(_view), _view))
      {
        m_lockRemainderX += after->x - before->x;
        m_lockRemainderY += after->y - before->y;
        const double stepX = std::floor(m_lockRemainderX);
        const double stepY = std::floor(m_lockRemainderY);
        m_lockRemainderX -= stepX;
        m_lockRemainderY -= stepY;
        if (stepX != 0.0 || stepY != 0.0)
        {
          _pointer.x =
            std::clamp(_pointer.x + static_cast<std::int32_t>(stepX), 0, std::max(0, static_cast<std::int32_t>(_view.width) - 1));
          _pointer.y =
            std::clamp(_pointer.y + static_cast<std::int32_t>(stepY), 0, std::max(0, static_cast<std::int32_t>(_view.height) - 1));
          request.warpTo = _pointer;
        }
      }
    }
  }
  else
  {
    // O108 (ADR-180): the goals from the controls, the state eased towards them, the camera placed, then the drag's and
    // the zoom's ground held under their pixels, and the ground point bounded (R-CTRL-471 step 10, WLD-244).
    const Neuron::Vec3 startFocus = m_focus;
    const Neuron::Vec3 startGoal = m_focusGoal;
    m_lookBlend *= 1.0 - std::min(1.0, m_presentation.cam.orbit.smoothing * dt);
    if (m_lookBlend < LOOK_BLEND_END)
    {
      m_lookBlend = 0.0;
    }
    Steer(dt, _controls, _pointer, _view, _terrain);
    Ease(dt, _terrain);
    Place(_terrain);
    if (m_grab)
    {
      HoldUnder(*m_grab, _pointer, _view, _terrain);
    }
    else if (m_zoomAnchor)
    {
      HoldUnder(m_zoomAnchor->ground, m_zoomAnchor->pixel, _view, _terrain);
      if (std::abs(m_height - m_heightGoal) <= ZOOM_SETTLED_SHARE * m_heightGoal)
      {
        m_zoomAnchor.reset();
      }
    }
    Bound(startFocus, startGoal, _areas);
    Place(_terrain);
  }
  m_lastPointer = _pointer;
  UpdatePointerGround(_pointer, _view, _terrain);
  return request;
}

double SettlementCamera::PitchRadiansAt(double _height) const noexcept
{
  const TuningPresentation::Cam::Orbit& orbit = m_presentation.cam.orbit;
  const double low = std::max(orbit.heightMin, 1e-3);
  const double high = std::max(m_presentation.ref.ctrl.camera.maxHeight, low);
  const double share = high > low ? std::clamp(std::log(std::max(_height, low) / low) / std::log(high / low), 0.0, 1.0) : 0.0;
  return (orbit.pitchLow + (orbit.pitchHigh - orbit.pitchLow) * share) * RADIANS_PER_DEGREE;
}

double SettlementCamera::Land(double _x, double _z, const Neuron::TerrainField& _terrain) const noexcept
{
  return std::max(_terrain.Height(_x, _z), m_presentation.ref.ctrl.camera.minHeight);
}

double SettlementCamera::Distance() const noexcept
{
  return m_height / std::max(std::sin(PitchRadiansAt(m_height)), 1e-6);
}

double SettlementCamera::ClampHeight(double _height) const noexcept
{
  const double low = std::max(m_presentation.cam.orbit.heightMin, 1e-3);
  return std::clamp(_height, low, std::max(m_presentation.ref.ctrl.camera.maxHeight, low));
}

void SettlementCamera::Zoom(double _dt, const CameraControls& _controls) noexcept
{
  // R-CTRL-460: the free movement's target eased at the normal rate, or a quarter of it at the zooming rate.
  const TuningPresentation::Ref::Ctrl::Camera& camera = m_presentation.ref.ctrl.camera;
  double target = m_targetFovDegrees;
  double rate = camera.zoomRates.normal;
  if (_controls.zoom)
  {
    target /= camera.zoomDivisor;
    rate = camera.zoomRates.zooming;
  }
  const double k = _dt * rate * camera.zoomRates.blendFactor;
  m_fovDegrees = m_fovDegrees * (1.0 - k) + target * k;
}

void SettlementCamera::Steer(double _dt, const CameraControls& _controls, Neuron::PointPx _pointer, Neuron::SizePx _view,
                             const Neuron::TerrainField& _terrain)
{
  const TuningPresentation::Cam::Orbit& orbit = m_presentation.cam.orbit;
  const double distance = Distance();
  // The middle button dragged with Alt held turns the heading with the pointer's movement at once, the view following
  // the pointer; dragged alone it holds the ground under the pointer.
  if (_controls.turnDrag)
  {
    if (m_lastPointer)
    {
      const double turn = static_cast<double>(m_lastPointer->x - _pointer.x) * orbit.dragTurn * RADIANS_PER_DEGREE;
      m_yawRadians += turn;
      m_yawGoal += turn;
    }
    m_grab.reset();
    m_zoomAnchor.reset();
  }
  else if (_controls.drag)
  {
    if (!m_grab)
    {
      UpdatePointerGround(_pointer, _view, _terrain);
      if (m_ground && m_ground->onTerrain)
      {
        m_grab = m_ground->position;
      }
    }
    m_zoomAnchor.reset();
  }
  else
  {
    m_grab.reset();
  }
  // Q and E turn the heading's goal; Home and Backspace send it back to the entry heading the short way.
  const double turnStep = orbit.turnRate * RADIANS_PER_DEGREE * _dt;
  if (_controls.turnLeft)
  {
    m_yawGoal += turnStep;
  }
  if (_controls.turnRight)
  {
    m_yawGoal -= turnStep;
  }
  if (_controls.resetHeading || _controls.home)
  {
    m_yawGoal = m_yawRadians + Wrapped(m_homeYaw - m_yawRadians);
  }
  // The pan keys and the horizontal wheel move the ground point's goal level, along the heading and across it, by shares
  // of the camera's distance from it, so that the picture moves alike at every height.
  const Neuron::Vec3 forward = HeadingOf(m_yawRadians);
  const Neuron::Vec3 left{forward.z, 0.0, -forward.x};
  const double step = orbit.panRate * distance * (_controls.fastPan ? orbit.fastPanFactor : 1.0) * _dt;
  Neuron::Vec3 pan;
  if (_controls.panLeft)
  {
    pan = pan + left * step;
  }
  if (_controls.panRight)
  {
    pan = pan - left * step;
  }
  if (_controls.panForward)
  {
    pan = pan + forward * step;
  }
  if (_controls.panBack)
  {
    pan = pan - forward * step;
  }
  pan = pan - left * (static_cast<double>(_controls.panWheelNotches) * orbit.wheelPanShare * distance);
  if (pan.x != 0.0 || pan.z != 0.0)
  {
    m_focusGoal.x += pan.x;
    m_focusGoal.z += pan.z;
    m_zoomAnchor.reset();
  }
  // R-CTRL-461 as O108 amends it: the wheel zooms towards the ground under the pointer, a notch forward multiplying the
  // height by cam.orbit.zoom_step; the zoom keys zoom about the screen's center.
  if (_controls.wheelNotches != 0.0f)
  {
    m_heightGoal = ClampHeight(m_heightGoal * std::pow(orbit.zoomStep, static_cast<double>(_controls.wheelNotches)));
    UpdatePointerGround(_pointer, _view, _terrain);
    if (m_ground && m_ground->onTerrain && !m_grab)
    {
      m_zoomAnchor = ZoomAnchor{_pointer, m_ground->position};
    }
    else
    {
      m_zoomAnchor.reset();
    }
  }
  const double keyNotches = ((_controls.lower ? 1.0 : 0.0) - (_controls.raise ? 1.0 : 0.0)) * orbit.zoomKeyNotches * _dt;
  if (keyNotches != 0.0)
  {
    m_heightGoal = ClampHeight(m_heightGoal * std::pow(orbit.zoomStep, keyNotches));
    m_zoomAnchor.reset();
  }
  if (_controls.home)
  {
    m_focusGoal = m_homeFocus;
    m_heightGoal = m_homeHeight;
    m_grab.reset();
    m_zoomAnchor.reset();
  }
  // The zoom holds its ground under its pixel only while the pointer stays on that pixel.
  if (m_zoomAnchor && (_pointer.x != m_zoomAnchor->pixel.x || _pointer.y != m_zoomAnchor->pixel.y))
  {
    m_zoomAnchor.reset();
  }
}

void SettlementCamera::Ease(double _dt, const Neuron::TerrainField& _terrain) noexcept
{
  // Each closes on its goal by min(1, cam.orbit.smoothing × dt) of the difference; the heading the short way.
  const double k = std::min(1.0, m_presentation.cam.orbit.smoothing * _dt);
  m_focus.x += (m_focusGoal.x - m_focus.x) * k;
  m_focus.z += (m_focusGoal.z - m_focus.z) * k;
  m_focus.y += (Land(m_focus.x, m_focus.z, _terrain) - m_focus.y) * k;
  m_height += (m_heightGoal - m_height) * k;
  m_yawRadians += Wrapped(m_yawGoal - m_yawRadians) * k;
  const double turns = m_yawRadians - Wrapped(m_yawRadians);
  m_yawRadians -= turns;
  m_yawGoal -= turns;
}

void SettlementCamera::Place(const Neuron::TerrainField& _terrain) noexcept
{
  // O108 (ADR-180): the front at the tilt the height gives, along the heading, onto the ground point; the position back
  // along it by the distance that gives the height.
  const double pitch = PitchRadiansAt(m_height);
  const Neuron::Vec3 orbitFront = HeadingOf(m_yawRadians) * std::cos(pitch) + Neuron::Vec3{0.0, -std::sin(pitch), 0.0};
  Neuron::Vec3 position = m_focus - orbitFront * Distance();
  // R-CTRL-471 step 6 as amended: above the land under the camera by the ground clearance, still facing the ground point.
  const double floor = Land(position.x, position.z, _terrain) + m_presentation.ref.ctrl.camera.groundClearance;
  m_orbitFront = orbitFront;
  if (position.y < floor)
  {
    position.y = floor;
    m_orbitFront = Neuron::NormalizedOr(m_focus - position, orbitFront);
  }
  m_position = position;
  m_front =
    m_lookBlend > 0.0 ? Neuron::NormalizedOr(m_orbitFront * (1.0 - m_lookBlend) + m_lookFront * m_lookBlend, m_orbitFront) : m_orbitFront;
}

void SettlementCamera::HoldUnder(const Neuron::Vec3& _ground, Neuron::PointPx _pixel, Neuron::SizePx _view,
                                 const Neuron::TerrainField& _terrain) noexcept
{
  // ADR-180: the ground point and its goal move level until _ground lies under _pixel again, as the camera now stands. A
  // level move of the camera moves the ray's meeting with a level plane by as much, so one step is exact on that plane.
  const Neuron::Ray ray =
    Neuron::PickRayAt(static_cast<double>(_pixel.x), static_cast<double>(_pixel.y), View(_view), _view, m_presentation.cam.pick.rowOffset);
  const std::optional<Neuron::Vec3> hit = OnLevel(ray, _ground.y);
  if (!hit)
  {
    return;
  }
  const double dx = _ground.x - hit->x;
  const double dz = _ground.z - hit->z;
  m_focus.x += dx;
  m_focus.z += dz;
  m_focusGoal = {m_focus.x, m_focusGoal.y, m_focus.z};
  Place(_terrain);
}

void SettlementCamera::Bound(const Neuron::Vec3& _startFocus, const Neuron::Vec3& _startGoal, const StewardAreas* _areas)
{
  // R-CTRL-471 step 10 as O108 amends it: the region's square ± the camera border bounds the ground point and its goal,
  // so that a pan held against the bound winds no goal beyond it (ADR-119).
  for (Neuron::Vec3* point : {&m_focus, &m_focusGoal})
  {
    point->x = std::clamp(point->x, m_bounds.minX, m_bounds.maxX);
    point->z = std::clamp(point->z, m_bounds.minZ, m_bounds.maxZ);
  }
  // WLD-244 (ADR-142): the ground point stays in the steward's areas as well, the goal first, each from its place at the
  // frame's start.
  if (_areas != nullptr && _areas->Known())
  {
    KeepInside(*_areas, m_focusGoal, _startGoal);
    KeepInside(*_areas, m_focus, _startFocus);
  }
}

void SettlementCamera::LookAround(double _dt) noexcept
{
  // R-CTRL-471 step 8, for looking around only: the front turns towards the ground point under the pointer by
  // |front × wanted| × min(1, √dt) radians.
  if (!m_ground)
  {
    return;
  }
  const Neuron::Vec3 wanted = Neuron::NormalizedOr(m_ground->position - m_position, m_lookFront);
  const Neuron::Vec3 axis = Neuron::Cross(m_lookFront, wanted);
  const double sine = Neuron::Length(axis);
  if (sine < 1e-12)
  {
    return;
  }
  const Neuron::Vec3 turned =
    Neuron::NormalizedOr(Rotate(m_lookFront, axis * (1.0 / sine), sine * std::min(1.0, std::sqrt(_dt))), m_lookFront);
  // A turn that ends steeper than the limit, or past the vertical, stops at the limit on the side it came from.
  const double limit = std::sin(STEEPEST_FRONT_DEGREES * RADIANS_PER_DEGREE);
  const Neuron::Vec3 heading = HeadingOf(m_lookYaw);
  const bool crossed = std::abs(turned.y) > 0.5 && Neuron::Dot(Neuron::Vec3{turned.x, 0.0, turned.z}, heading) < 0.0;
  if (std::abs(turned.y) > limit || crossed)
  {
    const double steepest = STEEPEST_FRONT_DEGREES * RADIANS_PER_DEGREE;
    m_lookFront = heading * std::cos(steepest) + Neuron::Vec3{0.0, turned.y < 0.0 ? -std::sin(steepest) : std::sin(steepest), 0.0};
    return;
  }
  m_lookFront = turned;
  if (std::abs(m_lookFront.y) < VERTICAL_FRONT_LIMIT)
  {
    m_lookYaw = std::atan2(-m_lookFront.x, -m_lookFront.z);
  }
}

void SettlementCamera::UpdatePointerGround(Neuron::PointPx _pointer, Neuron::SizePx _view, const Neuron::TerrainField& _terrain)
{
  const Neuron::Ray ray = Neuron::PickRayAt(static_cast<double>(_pointer.x), static_cast<double>(_pointer.y), View(_view), _view,
                                            m_presentation.cam.pick.rowOffset);
  // TER-017 may report a hit behind the start, which is no ground point.
  if (const std::optional<Neuron::Vec3> hit = _terrain.CastRay(ray.origin, ray.direction);
      hit && Neuron::Dot(*hit - ray.origin, ray.direction) >= 0.0)
  {
    m_ground = GroundPoint{*hit, true};
    return;
  }
  // R-CTRL-058 about the region (ADR-119): the ray, reversed from miss_ray_start radii out, meets the inside of the
  // sphere at the region's centre whose radius is miss_sphere_factor × the region's side; that is its far meeting.
  const TuningPresentation::Ref::Ctrl::Pointer& pointer = m_presentation.ref.ctrl.pointer;
  const Neuron::Vec3 center{(m_region.minX + m_region.maxX) * 0.5, 0.0, (m_region.minZ + m_region.maxZ) * 0.5};
  const double radius = pointer.missSphereFactor * (m_region.maxX - m_region.minX);
  const Neuron::Vec3 offset = ray.origin - center;
  const double b = Neuron::Dot(offset, ray.direction);
  const double discriminant = b * b - (Neuron::Dot(offset, offset) - radius * radius);
  if (discriminant < 0.0)
  {
    m_ground.reset();
    return;
  }
  m_ground = GroundPoint{ray.origin + ray.direction * (-b + std::sqrt(discriminant)), false};
}

Neuron::CameraPose SettlementCamera::Pose() const noexcept
{
  Neuron::CameraPose pose;
  pose.position = m_position;
  pose.yawRadians = std::abs(m_front.y) < VERTICAL_FRONT_LIMIT ? std::atan2(-m_front.x, -m_front.z) : m_yawRadians;
  pose.pitchRadians = std::asin(std::clamp(m_front.y, -1.0, 1.0));
  pose.verticalFovRadians = m_fovDegrees * RADIANS_PER_DEGREE;
  return pose;
}

Neuron::FrameView SettlementCamera::View(Neuron::SizePx _view) const noexcept
{
  const double aspect = static_cast<double>(std::max(_view.width, 1u)) / static_cast<double>(std::max(_view.height, 1u));
  return Neuron::MakeFrameView(Pose(), aspect, m_planes);
}

} // namespace Species
