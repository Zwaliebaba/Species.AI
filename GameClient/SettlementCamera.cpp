#include "pch.h"

#include "Geometry.h"
#include "SettlementCamera.h"
#include "StewardAreas.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace Species
{

namespace
{

constexpr double RADIANS_PER_DEGREE = std::numbers::pi / 180.0;
constexpr Neuron::Vec3 WORLD_UP{0.0, 1.0, 0.0};
constexpr double VERTICAL_FRONT_LIMIT = 1.0 - 1e-9; // CAM-034: a front this close to world up keeps the previous yaw
// ADR-119: the front stays within 89° of the horizontal. The reference states no limit, but a front through the vertical
// reverses its own ground projection, and so the pan, each frame it crosses.
constexpr double STEEPEST_FRONT_DEGREES = 89.0;

Neuron::Vec3 Horizontal(const Neuron::Vec3& _vector) noexcept
{
  return Neuron::NormalizedOr({_vector.x, 0.0, _vector.z}, {0.0, 0.0, -1.0});
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
  // O76: at the fallback height along the fallback direction, with the site's centre on the screen's centre.
  const Neuron::Vec3 direction = Neuron::NormalizedOr(
    {entry.fallbackCameraDirection.x, entry.fallbackCameraDirection.y, entry.fallbackCameraDirection.z}, {0.0, -1.0, 0.0});
  const Neuron::Vec3 ground{_siteCenter.x, _terrain.Height(_siteCenter.x, _siteCenter.z), _siteCenter.z};
  const double along = direction.y < -1e-6 ? (entry.fallbackCameraHeight - ground.y) / -direction.y : 0.0;
  m_position = ground - direction * along;
  m_front = direction;
  m_target = m_position;
  m_velocity = {};
  m_height = m_position.y;
  // R-CTRL-470: free movement's entry, the pointer to the screen centre with it.
  m_fovDegrees = m_presentation.ref.ctrl.camera.fovBase;
  m_targetFovDegrees = m_presentation.ref.ctrl.camera.fovBase;
  if (std::abs(m_front.y) < VERTICAL_FRONT_LIMIT)
  {
    m_yawRadians = std::atan2(-m_front.x, -m_front.z);
  }
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
  m_ground.reset();
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
  const Neuron::Vec3 startPosition = m_position;
  const Neuron::Vec3 startTarget = m_target;
  const double dt = std::clamp(_dtSeconds, 0.0, m_presentation.ref.sim.frame.deltaClamp);
  if (m_centerPointer)
  {
    _pointer = {static_cast<std::int32_t>(_view.width / 2), static_cast<std::int32_t>(_view.height / 2)};
    request.warpTo = _pointer;
    m_centerPointer = false;
    UpdatePointerGround(_pointer, _view, _terrain);
  }
  // R-CTRL-456 steps 2 and 3, then free movement's advancer (step 4, R-CTRL-471).
  if (!_controls.lookAround)
  {
    Zoom(dt, _controls);
  }
  AdjustHeight(dt, _controls, _terrain);
  const std::optional<Neuron::ScreenPoint> before =
    m_ground ? Neuron::ProjectToScreen(m_ground->position, View(_view), _view) : std::nullopt; // step 2
  FreeMovement(dt, _controls, _terrain);
  MouseLook(dt);
  // Step 9, R-CTRL-464: the pointer stays on its ground point while the camera moves.
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
        _pointer.x = std::clamp(_pointer.x + static_cast<std::int32_t>(stepX), 0, std::max(0, static_cast<std::int32_t>(_view.width) - 1));
        _pointer.y = std::clamp(_pointer.y + static_cast<std::int32_t>(stepY), 0, std::max(0, static_cast<std::int32_t>(_view.height) - 1));
        request.warpTo = _pointer;
      }
    }
  }
  // Step 10: the bounds, on the target too, so that a pan held against them winds no target beyond them (ADR-119).
  m_position.x = std::clamp(m_position.x, m_bounds.minX, m_bounds.maxX);
  m_position.z = std::clamp(m_position.z, m_bounds.minZ, m_bounds.maxZ);
  m_target.x = std::clamp(m_target.x, m_bounds.minX, m_bounds.maxX);
  m_target.z = std::clamp(m_target.z, m_bounds.minZ, m_bounds.maxZ);
  // WLD-244 (ADR-142): the camera's ground point, the point under its position and under its target, stays in the
  // steward's areas as well.
  if (_areas != nullptr && _areas->Known())
  {
    KeepInside(*_areas, m_target, startTarget);
    KeepInside(*_areas, m_position, startPosition);
  }
  // R-CTRL-456 steps 6 and 7 (CAM-034): the front is kept unit and the up vector is rebuilt from world up by the pose.
  m_front = Neuron::NormalizedOr(m_front, {0.0, 0.0, -1.0});
  if (std::abs(m_front.y) < VERTICAL_FRONT_LIMIT)
  {
    m_yawRadians = std::atan2(-m_front.x, -m_front.z);
  }
  UpdatePointerGround(_pointer, _view, _terrain);
  return request;
}

double SettlementCamera::Land(double _x, double _z, const Neuron::TerrainField& _terrain) const noexcept
{
  return std::max(_terrain.Height(_x, _z), m_presentation.ref.ctrl.camera.minHeight);
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

void SettlementCamera::AdjustHeight(double _dt, const CameraControls& _controls, const Neuron::TerrainField& _terrain) noexcept
{
  // R-CTRL-461: the wheel's notches and the raise and lower actions, faster with the square root of the altitude.
  const TuningPresentation::Ref::Ctrl::Camera& camera = m_presentation.ref.ctrl.camera;
  double notches = _controls.wheelNotches;
  if (_controls.raise)
  {
    notches += camera.heightKeyRate * _dt;
  }
  if (_controls.lower)
  {
    notches -= camera.heightKeyRate * _dt;
  }
  const double land = Land(m_position.x, m_position.z, _terrain);
  m_height += notches * camera.heightScale * std::sqrt(std::abs(m_height - land));
  m_height = std::min(std::max(m_height, land + camera.groundClearance), camera.maxHeight);
}

void SettlementCamera::FreeMovement(double _dt, const CameraControls& _controls, const Neuron::TerrainField& _terrain) noexcept
{
  const TuningPresentation::Ref::Ctrl::Camera& camera = m_presentation.ref.ctrl.camera;
  // Step 3: the pan along the horizontal left vector, up × front (R-CTRL-451). The forward and back keys fly along the
  // whole front (O90, ADR-142): its level part moves the target here, and its vertical part the height after step 5.
  const double rate = camera.panSpeed * (_controls.fastPan ? camera.fastPanFactor : 1.0);
  const Neuron::Vec3 left = Horizontal(Neuron::Cross(WORLD_UP, m_front));
  if (_controls.panLeft)
  {
    m_target = m_target + left * (_dt * rate);
  }
  if (_controls.panRight)
  {
    m_target = m_target - left * (_dt * rate);
  }
  const double along = ((_controls.panForward ? 1.0 : 0.0) - (_controls.panBack ? 1.0 : 0.0)) * _dt * rate;
  m_target.x += m_front.x * along;
  m_target.z += m_front.z * along;
  // Step 5: cliff avoidance. A blockage ahead shortens the step and lifts the camera by what it cut.
  const double distance = std::hypot(m_target.x - m_position.x, m_target.z - m_position.z);
  const std::int64_t samples = std::max<std::int64_t>(camera.cliffProbe.samples, 1);
  if (distance > 1e-9)
  {
    const Neuron::Vec3 direction{(m_target.x - m_position.x) / distance, 0.0, (m_target.z - m_position.z) / distance};
    const double spacing = camera.cliffProbe.length / static_cast<double>(samples);
    double blocked = camera.cliffProbe.length;
    for (std::int64_t k = 1; k <= samples; ++k)
    {
      const Neuron::Vec3 probe = m_position + direction * (static_cast<double>(k) * spacing);
      if (Land(probe.x, probe.z, _terrain) + camera.groundClearance > m_height)
      {
        blocked = static_cast<double>(k) * spacing;
        break;
      }
    }
    if (blocked < camera.cliffProbe.length)
    {
      const double s =
        std::max(0.0, (blocked - camera.cliffProbe.slowStart) / std::max(camera.cliffProbe.length - camera.cliffProbe.slowStart, 1e-9));
      m_target = m_position + (m_target - m_position) * s;
      m_height += (1.0 - s) * distance;
    }
  }
  // The fly's vertical part, after the cliff test, so that a descent is no cliff ahead. It stops at the highest height,
  // and at the ground clearance in step 6.
  if (along != 0.0)
  {
    m_height = std::min(m_height + m_front.y * along, camera.maxHeight);
  }
  // Step 6: the target's ground clearance from the target and the four points about it.
  const double around = camera.groundSamples;
  const double land = std::max({Land(m_target.x, m_target.z, _terrain), Land(m_target.x + around, m_target.z, _terrain),
                                Land(m_target.x - around, m_target.z, _terrain), Land(m_target.x, m_target.z + around, _terrain),
                                Land(m_target.x, m_target.z - around, _terrain)});
  if (land + camera.groundClearance > m_height)
  {
    m_height = land + camera.groundClearance;
  }
  m_target.y = m_height;
  // Step 7: the position eases towards the target, unclamped (R-CTRL-454, DEC-864 item 2), unless looking around.
  if (!_controls.lookAround)
  {
    const Neuron::Vec3 before = m_position;
    const double k = camera.positionSmoothing * _dt;
    m_position = m_position * (1.0 - k) + m_target * k;
    m_velocity = _dt > 0.0 ? (m_position - before) * (1.0 / _dt) : Neuron::Vec3{};
  }
}

void SettlementCamera::MouseLook(double _dt) noexcept
{
  // Step 8: the front turns towards the ground point under the pointer by |front × wanted| × min(1, √dt) radians.
  if (!m_ground)
  {
    return;
  }
  const Neuron::Vec3 wanted = Neuron::NormalizedOr(m_ground->position - m_position, m_front);
  const Neuron::Vec3 axis = Neuron::Cross(m_front, wanted);
  const double sine = Neuron::Length(axis);
  if (sine < 1e-12)
  {
    return;
  }
  const Neuron::Vec3 turned = Neuron::NormalizedOr(Rotate(m_front, axis * (1.0 / sine), sine * std::min(1.0, std::sqrt(_dt))), m_front);
  // A turn that ends steeper than the limit, or past the vertical, stops at the limit on the side it came from.
  const double limit = std::sin(STEEPEST_FRONT_DEGREES * RADIANS_PER_DEGREE);
  const Neuron::Vec3 heading{-std::sin(m_yawRadians), 0.0, -std::cos(m_yawRadians)};
  const bool crossed = std::abs(turned.y) > 0.5 && Neuron::Dot(Neuron::Vec3{turned.x, 0.0, turned.z}, heading) < 0.0;
  if (std::abs(turned.y) > limit || crossed)
  {
    const double steepest = STEEPEST_FRONT_DEGREES * RADIANS_PER_DEGREE;
    m_front = heading * std::cos(steepest) + Neuron::Vec3{0.0, turned.y < 0.0 ? -std::sin(steepest) : std::sin(steepest), 0.0};
    return;
  }
  m_front = turned;
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
