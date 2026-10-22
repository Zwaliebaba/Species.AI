#pragma once

#include "ClientSession.h"
#include "MessageLocale.h"
#include "ReplicatedState.h"
#include "SettlementMessages.h"
#include "StringTable.h"
#include "TextSystem.h"
#include "TuningMvp.h"
#include "TuningPresentation.h"
#include "UiLayout.h"
#include "WindowGeometry.h"
#include "WorldScene.h"

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace Species
{

// The structures the steward places, in the construction bar's order (MVP-032, MVP-101).
inline constexpr std::array<StructureKind, 3> BUILD_KINDS{StructureKind::Dwelling, StructureKind::Storehouse, StructureKind::Turret};

/// A notification of MVP-102 as the HUD keeps it: the server's notice and the real time it arrived.
struct HudNotice
{
  SettlementNotice notice;
  double arrivedSeconds = 0.0;
};

/// What the HUD shows (MVP-102 at M2): the client's own settlement in the replica, whose stores and population are
/// fresh every tick; the server's latest status (MVP-098) for its growth, priorities, tasks, idle Workers, upkeep and
/// queue; the construction bar's costs (MVP-034); the notices; and the world's seed (MVP-005).
struct HudModel
{
  std::optional<ReplicatedSettlement> settlement; // none: no settlement of the client's, and no HUD (the sandbox)
  std::optional<SettlementStatus> status;
  std::optional<std::uint64_t> seed;
  std::array<std::int64_t, BUILD_KINDS.size()> buildCostsMilliRu{};
  std::vector<HudNotice> notices; // oldest first
  double nowSeconds = 0.0;        // real time
  double tickSeconds = 0.1;       // ref.sim.tick_period
};

// The HUD's screen zones (UI-461, R-CTRL-223): the pointer over one reaches no world verb (T17).
enum class HudZone : std::uint8_t
{
  Settlement, // the settlement panel
  Build,      // a construction bar entry; its index is BUILD_KINDS's
  Queue,      // the construction queue
  Notices,    // the notification cards
  World       // the world's information panel
};

struct HudZoneRect
{
  HudZone zone = HudZone::Settlement;
  std::uint8_t index = 0;
  Neuron::UiRect rect;
};

// MVP-102: _notices with the notices _arrived at real time _nowSeconds added, without those mvp.hud.notice_seconds old
// or older, and at most mvp.hud.notices_max of them, the newest kept; oldest first.
[[nodiscard]] std::vector<HudNotice> AgeNotices(std::vector<HudNotice> _notices, std::span<const SettlementNotice> _arrived,
                                                double _nowSeconds, const TuningPresentation& _presentation);

// The HUD's model from the session at real time _nowSeconds, with the notices of AgeNotices.
[[nodiscard]] HudModel HudModelOf(const ClientSession& _session, const TuningMvp& _tuning, const TuningPresentation& _presentation,
                                  std::vector<HudNotice> _notices, std::span<const SettlementNotice> _arrived, double _nowSeconds);

/// MVP-102's settlement HUD at M2 (ADR-152, recipe D of UI-460): panels over the world in the HUD space of RND-377,
/// with no focus of their own, drawn in the UI pass. The settlement panel holds the stores and their capacity, the
/// Workers and the population limit, growth and the cause when it stops, the upkeep and the inactive structures, the
/// priorities with their Workers, and the idle Workers by reason (MVP-037); the construction bar holds each structure
/// with its cost, the queue each site with its progress; the notification cards the structures completed; and the
/// world's panel its seed. Its looks and measures are `mvp.hud.*`, provisional (O95, DEC-1005). Main thread only.
class SettlementHud
{
public:
  SettlementHud(const TuningPresentation& _presentation, const TuningMvp& _tuning);

  // Once a frame in the UI's update (UI-460): asks the server for the settlement's status every mvp.hud.status_period of
  // real time (MVP-098), takes the session's notices, and lays the panels out for _metrics, producing their zones,
  // which the pointer's hit test reads from the next frame (UI-461).
  void Update(ClientSession& _session, double _realSeconds, const Neuron::UiMetrics& _metrics);
  // The frame's panels, from the model and layout of the last update.
  void Draw(Neuron::TextSystem& _text, const Neuron::StringTable& _strings, const Neuron::MessageLocale& _locale,
            std::vector<Neuron::UiBatch>& _out) const;

  [[nodiscard]] const HudModel& Model() const noexcept
  {
    return m_model;
  }
  [[nodiscard]] std::span<const HudZoneRect> Zones() const noexcept
  {
    return m_zones;
  }
  // The zone under a client position, or none: the world lies under it.
  [[nodiscard]] std::optional<HudZoneRect> ZoneAt(Neuron::PointPx _position) const noexcept;

  // For a test: the model and the layout as an update would make them.
  void SetModel(HudModel _model, const Neuron::UiMetrics& _metrics);

private:
  void Layout(const Neuron::UiMetrics& _metrics);
  [[nodiscard]] float Units(double _hudUnits) const noexcept;

  const TuningPresentation& m_presentation;
  const TuningMvp& m_tuning;
  HudModel m_model;
  Neuron::UiMetrics m_metrics;
  std::vector<HudZoneRect> m_zones;
  Neuron::UiRect m_panel;
  Neuron::UiRect m_world;
  std::array<Neuron::UiRect, BUILD_KINDS.size()> m_build{};
  Neuron::UiRect m_queue;
  std::size_t m_panelRows = 0;
  std::optional<double> m_lastRequest;
};

} // namespace Species
