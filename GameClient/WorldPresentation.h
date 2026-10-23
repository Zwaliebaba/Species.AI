#pragma once

#include "ClientSettings.h"
#include "SettlementHud.h"
#include "ClientSession.h"
#include "DevelopmentOverlay.h"
#include "EffectsLook.h"
#include "FrontEnd.h"
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
#include "WorldSounds.h"

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
    bool uiTest = false;                  // a Debug build's `--ui-test`: the UI's test screen (ADR-145, ADR-149)
    DisplayOptions display;               // the display options of the settings store (ADR-150)
    Neuron::AudioEngine* audio = nullptr; // ADR-173: the world's sounds play on it, outliving the presentation; none: none
  };

  explicit WorldPresentation(Desc _desc);

  // Once per frame, after the session's update: builds the world's surfaces when the session's terrain is new.
  void Update(const ClientSession& _session);
  // ADR-170: the world has ended for the client, which leaves to the front end: its surfaces, looks and HUD are dropped,
  // so that the next world builds its own even when its terrain lands at the same address.
  void LeaveWorld();
  // ADR-152: once per frame, after the session's update: the settlement HUD's model, with the settlement verbs of _view
  // (ADR-153), its status requests and its layout for a client area of _client at _dpiScale.
  void UpdateHud(ClientSession& _session, const SettlementView& _view, Neuron::SizePx _client, float _dpiScale);
  [[nodiscard]] const SettlementHud& Hud() const noexcept
  {
    return *m_hud;
  }
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
  // ADR-155: the front end's screen over no world, its pages made by _frontEnd; and the screen closed as a world starts.
  void OpenFrontEnd(FrontEnd& _frontEnd, FrontEndPage _page)
  {
    m_ui->OpenFrontEnd(_frontEnd,
                       static_cast<GamePage>(static_cast<Neuron::UiPageId>(GamePage::FrontEndMain) + static_cast<Neuron::UiPageId>(_page)));
  }
  void CloseFrontEnd()
  {
    m_ui->CloseFrontEnd();
  }
  // ADR-170: the world's menu while a world runs, none as it ends; its screen opened over the world on _page, and closed.
  void AttachWorldMenu(WorldMenu* _menu)
  {
    m_ui->AttachWorldMenu(_menu);
  }
  void OpenWorldMenu(GamePage _page)
  {
    m_ui->OpenWorldMenu(_page);
  }
  void CloseWorldMenu()
  {
    m_ui->CloseWorldMenu();
  }
  // ADR-146: the en-US strings and their locale, which the front end formats its texts with.
  [[nodiscard]] const Neuron::StringTable& Strings() const noexcept
  {
    return *m_strings;
  }
  [[nodiscard]] const Neuron::MessageLocale& Locale() const noexcept
  {
    return *m_locale;
  }
  // ADR-150: the display options from the store, taken at once; the sea detail takes effect when the next world's
  // surfaces are built.
  void SetDisplay(const DisplayOptions& _display);
  [[nodiscard]] const DisplayOptions& Display() const noexcept
  {
    return m_desc.display;
  }
  // The inputs of the frame drawn now, seen through _view; _framesPerSecond is the renderer's counter (R-SIM-576), which a
  // Debug build's development overlay shows (DBG-095, ADR-118), and _dpiScale the window's, which text sizes take
  // (UI-170, ADR-147). The frame takes the session's presentation events, which its effects play (ADR-123).
  [[nodiscard]] Neuron::FrameInputs Frame(ClientSession& _session, const SettlementView& _view, double _framesPerSecond, float _dpiScale);
  // ADR-155: the inputs of a front-end frame, with no world: the background, the UI's pages, and the standard pointer at
  // _pointer (UI-465); in a Debug build with the overlay shown, the frame-rate line.
  [[nodiscard]] Neuron::FrameInputs FrontEndFrame(Neuron::PointPx _pointer, Neuron::SizePx _client, double _framesPerSecond,
                                                  float _dpiScale);

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
  // The world's surfaces and looks dropped, for no terrain.
  void ClearWorld();

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
  SoundSchedule m_sounds; // ADR-173: the effect events' sounds, on the display clock
  TerritoryLook m_territories;
  std::shared_ptr<const Neuron::ViewMask> m_viewMask; // ADR-144: the fog's mask of the steward's areas
  std::optional<std::uint64_t> m_viewMaskRevision;    // the areas' revision it was built from
  Neuron::FrameProfile m_profile;
  Neuron::PresentationClock m_presentationClock;
  std::unique_ptr<Neuron::TextSystem> m_text; // ADR-147: DirectWrite's layouts and the glyph atlas
  std::unique_ptr<GameUi> m_ui;               // ADR-149: the UI core with the game's pages
  std::unique_ptr<SettlementHud> m_hud;       // ADR-152: the settlement's HUD
  std::uint64_t m_frames = 0;
  bool m_overlayShown = false;
};

// The world's surfaces from a region's terrain, the records and the textures of the content: none when a texture is
// missing or damaged, which is logged.
// _seaDetail is render.sea_detail, 0 to 3 (RND-144).
[[nodiscard]] std::shared_ptr<const Neuron::WorldScene> BuildWorldScene(const RegionTerrain& _terrain, const TuningMvp& _tuning,
                                                                        const TuningPresentation& _presentation,
                                                                        const Neuron::LooseContent& _content, std::int64_t _seaDetail);

// The looks of the records (ADR-116): what the CPU builds and the passes read.
[[nodiscard]] Neuron::TerrainLook TerrainLookOf(const TuningMvp& _tuning, const TuningPresentation& _presentation) noexcept;
[[nodiscard]] Neuron::SeaLook SeaLookOf(const TuningMvp& _tuning, const TuningPresentation& _presentation,
                                        std::int64_t _seaDetail) noexcept;
[[nodiscard]] Neuron::WorldLook WorldLookOf(const TuningMvp& _tuning, const TuningPresentation& _presentation,
                                            const Neuron::Vec3& _regionOrigin) noexcept;

} // namespace Species
