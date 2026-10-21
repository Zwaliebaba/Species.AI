#pragma once

#include "RegionTerrain.h"
#include "TuningMvp.h"
#include "TuningPresentation.h"
#include "Vec3.h"
#include "WorldScene.h"

#include <array>
#include <optional>
#include <vector>

namespace Species
{

/// The route preview of M1 (M1 task T17b, ADR-125; O78, R-CTRL-023, RND-353, RND-354): the route that the client's
/// selection would walk to the order target, built with the region's land route graph into a store of its own, never the
/// simulation's (R-SIM-358), and drawn in pass 12 as dotted spots with an arrow on each leg and the destination mark at
/// its end. With O82 (ADR-127), the preview of an order sent is kept as an ordered route with a fade record, drawn
/// fading until it is gone (RND-351, RND-352). Frame thread only.
class RoutePreview
{
public:
  // _presentation and _tuning outlive the preview.
  RoutePreview(const TuningPresentation& _presentation, const TuningMvp& _tuning) noexcept;

  // Once per frame: the route from _from to _to, straight while Ctrl is held (R-SIM-345); none of either drops it. The
  // route is built again when an end has moved more than rnd.route.preview_rebuild_distance since it was built, or the
  // straight flag changed, and kept otherwise.
  void Update(const std::optional<Neuron::Vec3>& _from, const std::optional<Neuron::Vec3>& _to, bool _straight,
              const RegionTerrain& _terrain);
  // The preview dropped; the ordered routes stay.
  void Clear() noexcept;
  // A new world: the preview and the ordered routes dropped.
  void Reset() noexcept;
  // RND-351 for an order sent (ADR-127): the preview, while it is shown and walkable, becomes an ordered route with the
  // fade value 1.
  void Commit();
  // RND-352: each ordered route's fade value falls by _realStepSeconds × rnd.route.fade_rate; at 0 the route is dropped.
  void Advance(double _realStepSeconds);

  // Whether a route of at least one leg is shown, and whether every leg of it is walkable land (RND-354).
  [[nodiscard]] bool Shown() const noexcept
  {
    return m_points.size() > 1;
  }
  [[nodiscard]] bool Walkable() const noexcept
  {
    return m_walkable;
  }
  [[nodiscard]] const std::vector<Neuron::Vec3>& Points() const noexcept
  {
    return m_points;
  }

  /// An ordered route (ADR-127): its points, the terrain's normal at its end, and its fade value.
  struct Ordered
  {
    std::vector<Neuron::Vec3> points;
    Neuron::Vec3 endNormal{0.0, 1.0, 0.0};
    double fade = 1.0;
  };
  [[nodiscard]] const std::vector<Ordered>& OrderedRoutes() const noexcept
  {
    return m_ordered;
  }

  // Pass 12 (RND-353, DEC-959 item 4): the ordered routes, then the preview. Each leg's spots of fx.particle_dot, each one
  // spot's size after the last, with an arrow of marker.route_arrow at the leg's middle on the screen, in _teamColor while
  // walkable and the blocked colour otherwise, with the grey offset, pulsing on the real clock _realSeconds; and
  // marker.order_trail on the ground at the route's end (ADR-127). An ordered route's alpha is × its fade value.
  void AppendBatches(const Neuron::Vec3& _camera, const Neuron::Vec3& _cameraFront, double _realSeconds,
                     const std::array<float, 4>& _teamColor, std::vector<Neuron::QuadBatch>& _batches) const;

private:
  struct Batches;
  void AppendRoute(const std::vector<Neuron::Vec3>& _points, const Neuron::Vec3& _endNormal, bool _walkable, double _fade,
                   const Neuron::Vec3& _camera, const Neuron::Vec3& _cameraFront, double _realSeconds,
                   const std::array<float, 4>& _teamColor, Batches& _batches) const;

  const TuningPresentation* m_presentation;
  const TuningMvp* m_tuning;
  std::vector<Neuron::Vec3> m_points;
  Neuron::Vec3 m_endNormal{0.0, 1.0, 0.0}; // the terrain's normal at the preview's end
  std::vector<Ordered> m_ordered;
  Neuron::Vec3 m_from;
  Neuron::Vec3 m_to;
  bool m_straight = false;
  bool m_walkable = false;
};

} // namespace Species
