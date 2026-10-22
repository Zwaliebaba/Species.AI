#pragma once

#include "ClientSession.h"
#include "DevelopmentOverlay.h"
#include "EffectsLook.h"
#include "FrameProfile.h"
#include "GameUi.h"
#include "InputRouter.h"
#include "LeaderLook.h"
#include "LooseContent.h"
#include "MessageLocale.h"
#include "PlatformClock.h"
#include "PresentationClock.h"
#include "SettlementView.h"
#include "StringTable.h"
#include "StructureLook.h"
#include "TerritoryLook.h"
#include "TextSystem.h"
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
/// settlement view's camera and cursor (M1 task T14, ADR-119), the crowds (M1 task T16, ADR-121, ADR-122), the weapons'
/// effects, particles and screen effects (ADR-123), the structures (M2 task T12, ADR-140) and the UI's batches (M2 task
/// T14, ADR-145). In a Debug build,
/// while the overlay is shown, each frame also carries a dot for each living figure (ADR-118) and the frame-rate line and
/// frame page from the frame loop's profile (M1 task T15b, ADR-120).
class WorldPresentation
{
public:
  struct Desc
  {
    const TuningMvp* tuning = nullptr; // the client's records and content, outliving the presentation
    const TuningPresentation* presentation = nullptr;
    const Neuron::LooseContent* content = nullptr;
    const Neuron::PlatformClock* clock = nullptr;
    bool uiTest = false; // a Debug build's `--ui-test`: the UI's test screen (ADR-145, ADR-149)
  };

  explicit WorldPresentation(Desc _desc);

  // Once per frame, after the session's update: builds the world's surfaces when the session's terrain is new.
  void Update(const ClientSession& _session);
  // ADR-149: once per frame, before the camera and the verbs read _router: the UI's update, which withholds from the
  // world the actions that a focused window takes (UI-006).
  void UpdateUi(Neuron::InputRouter& _router, Neuron::SizePx _client, float _dpiScale, bool _windowFocused)
  {
    m_ui->Update(_router, _client, _dpiScale, _windowFocused);
  }
  [[nodiscard]] const GameUi& Ui() const noexcept
  {
    return *m_ui;
  }
  // The inputs of the frame drawn now, seen through _view; _framesPerSecond is the renderer's counter (R-SIM-576), which a
  // Debug build's development overlay shows (DBG-095, ADR-118), and _dpiScale the window's, which text sizes take
  // (UI-170, ADR-147). The frame takes the session's presentation events, which its effects play (ADR-123).
  [[nodiscard]] Neuron::FrameInputs Frame(ClientSession& _session, const SettlementView& _view, double _framesPerSecond, float _dpiScale);

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
  std::shared_ptr<const Neuron::UiImages> m_uiImages;           // ADR-145: the UI's images, each empty when missing
  std::shared_ptr<const Neuron::StringTable> m_strings;         // ADR-146: the en-US strings (O91); empty when missing
  std::unique_ptr<Neuron::MessageLocale> m_locale;              // ADR-146: en-US's number forms and plural rules
  std::shared_ptr<const Neuron::CursorImages> m_cursorImages;   // ADR-119; none when the content lacks one
  std::shared_ptr<const Neuron::SceneTextures> m_sceneTextures; // ADR-121; none when the content lacks one
  std::shared_ptr<const Neuron::SceneMeshes> m_sceneMeshes;     // ADR-122; none when the content lacks one
  LeaderLook m_leaders;
  EffectsLook m_effects;
  StructureLook m_structures;
  TerritoryLook m_territories;
  std::shared_ptr<const Neuron::ViewMask> m_viewMask; // ADR-144: the fog's mask of the steward's areas
  std::optional<std::uint64_t> m_viewMaskRevision;    // the areas' revision it was built from
  Neuron::FrameProfile m_profile;
  Neuron::PresentationClock m_presentationClock;
  std::unique_ptr<Neuron::TextSystem> m_text; // ADR-147: DirectWrite's layouts and the glyph atlas
  std::unique_ptr<GameUi> m_ui;               // ADR-149: the UI core with the game's pages
  std::uint64_t m_frames = 0;
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
