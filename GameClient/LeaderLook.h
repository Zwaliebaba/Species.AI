#pragma once

#include "CrowdLook.h"
#include "MeshAsset.h"
#include "ReplicaTiming.h"
#include "TuningMvp.h"
#include "TuningPresentation.h"
#include "WorldScene.h"

#include <cstdint>
#include <map>
#include <span>
#include <vector>

namespace Species
{

/// What a frame of the Leaders' look draws (ADR-122): their figures and flickering bodies, the banners' poles and the
/// banners' cloth, each by pass 7b's entries.
struct LeaderDraws
{
  std::vector<Neuron::MeshBatch> meshes;
  std::vector<Neuron::QuadBatch> quads;
  std::vector<Neuron::TriangleBatch> triangles;
};

/// The Leaders' look (M1 task T16b, ADR-122; R-UNIT-190, R-UNIT-191, RND-324, VFX-126, VFX-127, VFX-139 to VFX-142):
/// each Leader's figure under its team's tint, its damage flicker, and its banner of cloth at the figure's flag socket.
/// It keeps each Leader's presentation state, the banner's cloth and the flicker's choice, keyed by its id, and drops a
/// Leader's when it is no longer drawn (RND-307). Frame thread only.
class LeaderLook
{
public:
  // Once per frame: _figures' Leaders, with _figure the cooked mesh.unit.leader_figure, whose flag socket holds the
  // banner. The cloth runs in fixed steps of vfx.emit.step_rate on the unscaled clock (VFX-018), _unscaledStep's worth.
  void Draw(std::span<const InterpolatedFigure> _figures, const Neuron::MeshAsset& _figure, const CrowdView& _view, double _unscaledStep,
            const TuningPresentation& _presentation, const TuningMvp& _tuning, LeaderDraws& _draws);

  [[nodiscard]] std::size_t KeptLeaders() const noexcept
  {
    return m_leaders.size();
  }

private:
  struct Leader
  {
    std::vector<Neuron::Vec3> cloth; // resolution × resolution vertices, column x outer and row z inner
    std::int64_t flickerStep = -1;   // the scaled clock's flicker step last evaluated
    bool flickering = false;
    std::uint8_t flickerAxis = 0; // 0 front, 1 up, 2 right
    bool drawn = false;           // this frame
  };

  std::map<StableId, Leader> m_leaders;
  double m_stepCarry = 0.0; // the unscaled time not yet stepped
};

} // namespace Species
