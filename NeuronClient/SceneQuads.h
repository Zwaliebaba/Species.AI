#pragma once

#include "ShaderLayouts.h"
#include "Vec3.h"
#include "WorldScene.h"

#include <array>

namespace Neuron
{

// WLD-246, RND-262: a scene quad as its vertex shader reads it, its points relative to _camera, the differences taken in
// binary64 and only then rounded.
[[nodiscard]] QuadInstance MakeQuadInstance(const SceneQuad& _quad, const Vec3& _camera) noexcept;

// RND-324 (ADR-122): a mesh instance as its vertex shader reads it, its origin relative to _camera.
[[nodiscard]] MeshInstance MakeMeshInstance(const SceneMeshInstance& _instance, const Vec3& _camera) noexcept;
// ADR-122: a triangle vertex relative to _camera.
[[nodiscard]] TriangleVertex MakeTriangleVertex(const SceneVertex& _vertex, const Vec3& _camera) noexcept;

// ADR-009's M1 form (ADR-121): the CPU view test, conservative as RND-236 asks, of a sphere of _radius about the quad's
// position, or about the middle of a segment with its half-length added, against the frame's camera-relative planes.
[[nodiscard]] bool QuadMayBeVisible(const std::array<Float4, 6>& _planes, const SceneQuad& _quad, const Vec3& _camera,
                                    float _radius) noexcept;

} // namespace Neuron
