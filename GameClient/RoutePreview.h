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
/// simulation's (R-SIM-358), and drawn in pass 12 as dotted spots with an arrow on each leg. Frame thread only.
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
  void Clear() noexcept;

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

  // Pass 12 (RND-353, DEC-959 item 4): each leg's spots of fx.particle_dot, each one spot's size after the last, then an
  // arrow of marker.route_arrow at each leg's middle, in _teamColor while walkable and the blocked colour otherwise, with
  // the grey offset, pulsing on the real clock _realSeconds.
  void AppendBatches(const Neuron::Vec3& _camera, const Neuron::Vec3& _cameraFront, double _realSeconds,
                     const std::array<float, 4>& _teamColor, std::vector<Neuron::QuadBatch>& _batches) const;

private:
  const TuningPresentation* m_presentation;
  const TuningMvp* m_tuning;
  std::vector<Neuron::Vec3> m_points;
  Neuron::Vec3 m_from;
  Neuron::Vec3 m_to;
  bool m_straight = false;
  bool m_walkable = false;
};

} // namespace Species
