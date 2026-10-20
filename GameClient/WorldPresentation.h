#pragma once

#include "ClientSession.h"
#include "LooseContent.h"
#include "PlatformClock.h"
#include "TuningMvp.h"
#include "TuningPresentation.h"
#include "WorldScene.h"

#include <cstdint>
#include <memory>
#include <optional>

namespace Species
{

/// The client's world frame (M1 task T13, ADR-115): the world's surfaces, built from the session's terrain with the
/// client's presentation record and the content's textures once the terrain arrives, and each frame's inputs from the
/// fixed development view.
class WorldPresentation
{
public:
  struct Desc
  {
    const TuningMvp* tuning = nullptr; // the client's records and content, outliving the presentation
    const TuningPresentation* presentation = nullptr;
    const Neuron::LooseContent* content = nullptr;
    const Neuron::PlatformClock* clock = nullptr;
  };

  explicit WorldPresentation(Desc _desc);

  // Once per frame, after the session's update: builds the world's surfaces when the session's terrain is new.
  void Update(const ClientSession& _session);
  // The inputs of the frame drawn now.
  [[nodiscard]] Neuron::FrameInputs Frame(const ClientSession& _session);

  [[nodiscard]] const std::shared_ptr<const Neuron::WorldScene>& World() const noexcept
  {
    return m_world;
  }

private:
  Desc m_desc;
  const RegionTerrain* m_terrain = nullptr; // the terrain the world was built from
  std::shared_ptr<const Neuron::WorldScene> m_world;
  Neuron::Nanoseconds m_start{};
  std::optional<double> m_firstDisplayTick;
};

// The world's surfaces from a region's terrain, the records and the textures of the content: none when a texture is
// missing or damaged, which is logged.
[[nodiscard]] std::shared_ptr<const Neuron::WorldScene> BuildWorldScene(const RegionTerrain& _terrain, const TuningMvp& _tuning,
                                                                        const TuningPresentation& _presentation,
                                                                        const Neuron::LooseContent& _content);

// The looks of the records (ADR-116): what the CPU builds and the passes read.
[[nodiscard]] Neuron::TerrainLook TerrainLookOf(const TuningMvp& _tuning, const TuningPresentation& _presentation) noexcept;
[[nodiscard]] Neuron::SeaLook SeaLookOf(const TuningMvp& _tuning, const TuningPresentation& _presentation) noexcept;
[[nodiscard]] Neuron::WorldLook WorldLookOf(const TuningMvp& _tuning, const TuningPresentation& _presentation,
                                            const Neuron::Vec3& _regionOrigin) noexcept;

// The stand-in for the player camera of M1 task T14 (ADR-115): over the sea beyond the region's +z coast, looking
// across the valley along the reference's initial pose, below the lowest cloud layer.
[[nodiscard]] Neuron::CameraPose DevelopmentView(const Neuron::Vec3& _regionOrigin, double _regionSide) noexcept;

} // namespace Species
