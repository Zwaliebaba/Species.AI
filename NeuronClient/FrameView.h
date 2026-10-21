#pragma once

#include "ShaderLayouts.h"
#include "Vec3.h"

#include <array>

namespace Neuron
{

/// Where a frame looks from (ADR-016): a position in world units, kept in binary64, and the yaw and pitch of the view.
/// A yaw of 0 looks along −Z, a positive yaw turns towards −X (right-handed, +Y up), and a negative pitch looks down.
/// The roll turns the view's up vector about its front, as the camera shake's offset does (VFX-232, ADR-123).
struct CameraPose
{
  Vec3 position;
  double yawRadians = 0.0;
  double pitchRadians = 0.0;
  double rollRadians = 0.0;
  double verticalFovRadians = 1.0; // the camera's (ADR-016: Hor+, the horizontal field grows with the aspect ratio)
};

struct ProjectionPlanes
{
  double nearPlane = 5.0;                      // rnd.view.near_plane, wu (RND-211)
  double farPlane = 15'000.0;                  // rnd.view.far_plane, wu
  double fovMinRadians = 0.017453292519943295; // rnd.view.fov_min: the field of view's clamp (RND-212)
  double fovMaxRadians = 3.141592653589793;    // rnd.view.fov_max
};

/// The matrices of one frame (ADR-016, WLD-246): reversed-Z perspective, the near plane at depth 1 and the far plane
/// at 0, and a view that holds the camera's rotation alone, because every position a shader sees is relative to the
/// camera. Row vectors and row-major storage, as the shaders read them.
struct FrameView
{
  Float4x4 view;
  Float4x4 projection;
  Float4x4 viewProjection;
  Vec3 camera; // the camera's world position, in binary64
};

[[nodiscard]] FrameView MakeFrameView(const CameraPose& _pose, double _aspect, const ProjectionPlanes& _planes) noexcept;

// WLD-246: the transform of a model whose origin is _origin in world units, relative to the camera: the difference is
// taken in binary64 and only then rounded to binary32. RND-262: the 3x4 matrix is the transpose of the row-vector
// transform, applied as mul(M, float4(p, 1)).
[[nodiscard]] Float3x4 CameraRelativeTransform(const Vec3& _origin, const Vec3& _camera) noexcept;

// The direction the pose looks along, a unit vector in world space.
[[nodiscard]] Vec3 ViewDirection(const CameraPose& _pose) noexcept;

// The six planes of the frame's view volume in camera-relative coordinates, each (a, b, c, d) with a point inside when
// a·x + b·y + c·z + d ≥ 0: left, right, bottom, top, and the two depth planes of the reversed range.
[[nodiscard]] std::array<Float4, 6> FrustumPlanes(const FrameView& _frame) noexcept;
// RND-138, RND-236: false only when the box lies wholly outside one plane, so a box that meets the volume is never
// culled. The box is camera-relative.
[[nodiscard]] bool BoxMayBeVisible(const std::array<Float4, 6>& _planes, const Vec3& _min, const Vec3& _max) noexcept;

} // namespace Neuron
