#pragma once

#include "ClientSession.h"
#include "MessageLocale.h"
#include "RelationMessages.h"
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

/// What the HUD shows of the settlement verbs (M2 task T17, ADR-153).
struct HudVerbs
{
  std::optional<StructureKind> placing;              // the structure the steward is placing
  std::optional<PlacementRefusal> foreseen;          // the refusal the client foresees at the pointer while placing
  std::optional<PlacementRefusal> refusal;           // the server's last refusal of a placement,
  double refusalAgeSeconds = 0.0;                    // and how long ago it came, in real time
  std::optional<std::array<std::uint8_t, 3>> shares; // the priorities sent, shown until a status holds them
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
  HudVerbs verbs;                 // ADR-153
  // M3 task T11 (ADR-166): the server's latest valley status (MVP-098), the replica's latest tick for its countdowns, and
  // mvp.conflict.warning_minutes in seconds.
  std::optional<ValleyStatus> valley;
  std::uint64_t tick = 0;
  double warningSeconds = 0.0;
};

// MVP-102 (ADR-166): what a row of the relations panel shows.
enum class RelationRowKind : std::uint8_t
{
  Heading,       // the panel's title
  Warning,       // an attack declared on the steward's settlement, its countdown to the warning's end
  Expedition,    // the steward's own expedition and where it stands
  Respite,       // the steward's settlement under a respite, the minutes it has left
  Cladan,        // another Cladan's anger toward the steward's settlement, and whether it is under a respite
  Grievance,     // one of that Cladan's grievances against the steward's settlement: its cause, weight and fade
  ClaimsHeading, // the claims' title
  Claim,         // a claim: its claimant, area and state
  More           // the rows the panel's bound leaves out
};

/// A row of the relations panel: its kind; the settlement it names by its local site (the Cladan, the attacker, the
/// expedition's target, the claimant); a value (anger or weight in whole units, the seconds or minutes left, the rows
/// left out); and the kind's detail (a grievance's cause, a claim's area, an expedition's state) and state (a claim's
/// state, a grievance whose cause is removed, a Cladan under a respite).
struct RelationRow
{
  RelationRowKind kind = RelationRowKind::Heading;
  std::uint8_t site = 0;
  std::int64_t value = 0;
  std::uint8_t detail = 0;
  std::uint8_t state = 0;

  friend bool operator==(const RelationRow&, const RelationRow&) = default;
};

// MVP-050, MVP-102 (ADR-166): the relations panel's rows for the settlement at site _site after tick _tick of the
// valley status: the warning of each attack declared on it, its own expedition, its respite, each other Cladan's anger
// toward it with the grievances that make it, and every claim; at most _rowsMax rows, the last of a longer list saying
// how many it leaves out.
[[nodiscard]] std::vector<RelationRow> RelationRowsOf(const ValleyStatus& _valley, std::uint8_t _site, std::uint64_t _tick,
                                                      double _tickSeconds, double _warningSeconds, std::size_t _rowsMax);

// The HUD's screen zones (UI-461, R-CTRL-223): a press over one reaches no world verb (ADR-153).
enum class HudZone : std::uint8_t
{
  Settlement,    // the settlement panel
  Build,         // a construction bar entry; its index is BUILD_KINDS's
  Queue,         // the construction queue
  Notices,       // the notification cards
  World,         // the world's information panel
  PriorityRaise, // a priority's raising arrow; its index is the task's, from gathering (ADR-153)
  PriorityLower, // a priority's lowering arrow
  Relations      // the relations panel (ADR-166)
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

// The dwellings of team _team in a replica's tick, built or queued, as the server counts them for MVP-034's cost.
[[nodiscard]] std::int64_t TeamDwellings(const StateReplica::View& _latest, std::uint8_t _team);

// The HUD's model from the session at real time _nowSeconds, with the notices of AgeNotices and the verbs' _verbs.
[[nodiscard]] HudModel HudModelOf(const ClientSession& _session, const TuningMvp& _tuning, const TuningPresentation& _presentation,
                                  std::vector<HudNotice> _notices, std::span<const SettlementNotice> _arrived, double _nowSeconds,
                                  const HudVerbs& _verbs);

/// MVP-102's settlement HUD at M2 (ADR-152, recipe D of UI-460): panels over the world in the HUD space of RND-377,
/// with no focus of their own, drawn in the UI pass. The settlement panel holds the stores and their capacity, the
/// Workers and the population limit, growth and the cause when it stops, the upkeep and the inactive structures, the
/// priorities with their Workers, and the idle Workers by reason (MVP-037); the construction bar holds each structure
/// with its cost, the queue each site with its progress; the notification cards the structures completed; and the
/// world's panel its seed. From M3 task T11 (ADR-166) it asks for the valley's status with the settlement's, shows the
/// relations panel of RelationRowsOf, and draws a card for every kind of notice. Its looks and measures are `mvp.hud.*`,
/// provisional (O95, DEC-1005). Main thread only.
class SettlementHud
{
public:
  SettlementHud(const TuningPresentation& _presentation, const TuningMvp& _tuning);

  // Once a frame in the UI's update (UI-460): asks the server for the settlement's status every mvp.hud.status_period of
  // real time (MVP-098), takes the session's notices and the verbs' state, and lays the panels out for _metrics,
  // producing their zones, which the pointer's hit test reads from the next frame (UI-461).
  void Update(ClientSession& _session, double _realSeconds, const Neuron::UiMetrics& _metrics, const HudVerbs& _verbs);
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
  // The zone under a client position, the last laid out that holds it, so an arrow before its panel; or none: the
  // world lies under it.
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
  std::array<std::array<Neuron::UiRect, 2>, 3> m_arrows{}; // each priority's lowering and raising arrows (ADR-153)
  Neuron::UiRect m_queue;
  Neuron::UiRect m_relations;
  std::vector<RelationRow> m_relationRows;
  std::size_t m_panelRows = 0;
  std::optional<double> m_lastRequest;
};

} // namespace Species
