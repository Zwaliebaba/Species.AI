#pragma once

#include "ClientSession.h"
#include "ReplicaTiming.h"
#include "TerrainField.h"
#include "WorldScene.h"

#include <cstdint>
#include <span>
#include <vector>

namespace Species
{

struct TuningPresentation;

/// What the structures' look reads of the frame (ADR-140).
struct StructureView
{
  double displayTick = 0.0; // the replica's display time in ticks: one tick behind the latest, plus the fraction
  double tickSeconds = 0.1; // ref.sim.tick_period
  double realSeconds = 0.0; // the presentation clock's real reading, which turns the Wellspring's glow
};

/// What a frame of the structures draws (ADR-140): the solid parts of pass 6 and the port pads of pass 8 as mesh batches,
/// and the stars, lights and glows of pass 8 as quad batches.
struct StructureDraws
{
  std::vector<Neuron::MeshBatch> meshes;
  std::vector<Neuron::QuadBatch> quads;
};

/// The structures drawn from the replica (M2 task T12, ADR-140; MVP-083, RND-163 to RND-170, R-BLDG-017, R-BLDG-045,
/// R-BLDG-046, R-BLDG-428). Each structure's mesh stands on its place, facing its front, tinted in its settlement's colour
/// (ADR-020); a construction site is sunk by what remains of its work and rises with it, and an inactive structure is
/// dark. A turret draws its head at its aim and its gun barrels at the head's barrel mount, raised to the barrels' tilt,
/// and its head is tinted by the team that mans it. In pass 8 each port draws its pad at the terrain and a star, green
/// while its Worker mans it and red otherwise; a dwelling's and a storehouse's construction ports only while it is a
/// site (O89), and the Wellspring's, which serve no task at M2, never. A built, active structure of a team shows its
/// lights in the command frames of its team, and the Wellspring its cloudy glow. A completed structure flashes once,
/// from its event. Nothing here reaches the simulation. Frame thread only.
class StructureLook
{
public:
  // _presentation outlives the look.
  explicit StructureLook(const TuningPresentation& _presentation) noexcept;

  // Each StructureCompleted of _events, taken from the session, a flash from its tick.
  void Update(std::span<const ReceivedEvent> _events);
  // The frame's draws of _structures, the replica's at the frame's fraction, with _meshes, the scene's meshes, which hold
  // every SceneMesh; _field gives the ports' terrain heights.
  void Draw(std::span<const InterpolatedStructure> _structures, const Neuron::SceneMeshes& _meshes, const Neuron::TerrainField& _field,
            const StructureView& _view, StructureDraws& _draws);
  // A new world: every flash is dropped.
  void Clear() noexcept;

private:
  struct Flash
  {
    Neuron::Vec3 position;
    std::uint8_t team = 0;
    double startTick = 0.0; // the display tick it starts at: its event's tick less one (ADR-123)
  };

  const TuningPresentation& m_presentation;
  std::vector<Flash> m_flashes;
};

} // namespace Species
