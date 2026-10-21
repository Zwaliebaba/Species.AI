#pragma once

#include "ClientSession.h"
#include "DevelopmentOverlay.h"
#include "FrameProfile.h"
#include "LooseContent.h"
#include "PlatformClock.h"
#include "SettlementView.h"
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
/// settlement view's camera and cursor (M1 task T14, ADR-119). In a Debug build each frame also carries a dot for each
/// living figure (M1 task T15a, ADR-118) and, while the overlay is shown, its frame-rate line and frame page from the frame
/// loop's profile (M1 task T15b, ADR-120).
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
  // The inputs of the frame drawn now, seen through _view; _framesPerSecond is the renderer's counter (R-SIM-576), which a
  // Debug build's development overlay shows (DBG-095, ADR-118).
  [[nodiscard]] Neuron::FrameInputs Frame(const ClientSession& _session, const SettlementView& _view, double _framesPerSecond);

  [[nodiscard]] const std::shared_ptr<const Neuron::WorldScene>& World() const noexcept
  {
    return m_world;
  }

  [[nodiscard]] const TuningPresentation& Presentation() const noexcept
  {
    return *m_desc.presentation;
  }
  // ADR-120: the frame loop's profile, whose steps are ClientStep's (dbg.profile.frame_history, dbg.profile.rollup_period_ms).
  [[nodiscard]] Neuron::FrameProfile& Profile() noexcept
  {
    return m_profile;
  }
  // DBG-095, DBG-098: action.debug.overlay shows and hides the overlay, which starts hidden; Release builds draw none.
  void ToggleOverlay() noexcept
  {
    m_overlayShown = !m_overlayShown;
  }
  [[nodiscard]] bool OverlayShown() const noexcept
  {
    return m_overlayShown;
  }
  // ADR-120: the frame page's figures, for the log line of every tenth roll-up period.
  [[nodiscard]] FramePageInputs FramePageOf(const ClientSession& _session, double _framesPerSecond) const;

private:
  Desc m_desc;
  const RegionTerrain* m_terrain = nullptr; // the terrain the world was built from
  std::shared_ptr<const Neuron::WorldScene> m_world;
  Neuron::Nanoseconds m_start{};
  std::optional<double> m_firstDisplayTick;
  std::shared_ptr<const Neuron::FontAsset> m_developmentFace; // a Debug build's overlay face; none in Release
  std::shared_ptr<const Neuron::CursorImages> m_cursorImages; // ADR-119; none when the content lacks one
  Neuron::FrameProfile m_profile;
  bool m_overlayShown = false;
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

} // namespace Species
