#pragma once

#include "MeshModel.h"

#include <cstdint>

namespace MeshKit
{

/// The axis a frustum stands along: up, or forward as a barrel points.
enum class Axis : std::uint8_t
{
  Up,
  Forward
};

/// A regular frustum: a prism when its two radii are equal, a pyramid when its top radius is 0. Its base centre, its
/// axis and length, its side count (one of the counts of TurnDirection, or half of one when halfStep turns it by half
/// a side so that a flat side faces +x), and which caps it closes.
struct Frustum
{
  Point base{};
  Axis axis = Axis::Up;
  std::uint32_t sides = 8;
  bool halfStep = false;
  double baseRadius = 1.0;
  double topRadius = 1.0;
  double length = 1.0;
  bool baseCap = false;
  bool topCap = true;
  Color color{};
  Color capColor{};
  Surface surface = Surface::Facet;
};

// Each shape adds its triangles to the node, each wound counter-clockwise seen from outside the shape (the CMO
// convention of ADR-019), and throws std::logic_error for a step count TurnDirection does not know or a triangle with no
// area.
void AddFrustum(ModelNode& _node, const Frustum& _frustum);
void AddBox(ModelNode& _node, const Point& _min, const Point& _max, Color _color, Surface _surface, bool _bottom = false);

// A gabled roof over the box from _min to _max: its ridge runs along x at the height _max[1], above the middle of z.
void AddGable(ModelNode& _node, const Point& _min, const Point& _max, Color _roof, Color _ends, Surface _surface);

// The point at _radius from _center in the direction _step ÷ _steps of a turn, at the height _y.
[[nodiscard]] Point Around(const Point& _center, double _radius, std::uint32_t _step, std::uint32_t _steps, double _y);

} // namespace MeshKit
