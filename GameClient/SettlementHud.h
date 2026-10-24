#pragma once

#include "ClientSession.h"
#include "MessageLocale.h"
#include "NineSlice.h"
#include "RelationMessages.h"
#include "ReplicatedState.h"
#include "SettlementMessages.h"
#include "StringTable.h"
#include "TextSystem.h"
#include "TuningMvp.h"
#include "TuningPresentation.h"
#include "UiLayout.h"
#include "UiLook.h"
#include "WindowGeometry.h"
#include "WorldMarkers.h"
#include "WorldScene.h"

#include <array>
#include <cstdint>
#include <deque>
#include <optional>
#include <span>
#include <string>
#include <utility>
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
  std::optional<BattleResultNotice> result; // ADR-168: another battle's public line, shown in place of the notice
};

// M3 task T12 (ADR-167): the kinds of the conflict commands whose refusals the HUD shows.
enum class ConflictKind : std::uint8_t
{
  Claim,  // a claim or its withdrawal (ClaimRefusal)
  Attack, // a declared attack or its withdrawal (AttackRefusal)
  Tribute // a tribute (TributeRefusal)
};

/// MVP-101 (ADR-167): an attack the steward is declaring, shown with its cost and whether it is unprovoked until a
/// second press on its button declares it: a raid on the settlement at site `target`, or the claim battle of a claim on
/// `area`, with the Leaders that would lead it.
struct PendingAttack
{
  std::uint8_t target = 0;
  std::optional<std::uint8_t> area; // a claim battle's area; none for a raid
  std::size_t leaders = 0;
  std::int64_t costMilliRu = 0;
  bool unprovoked = false;
  std::uint8_t button = 0; // the zone index of the button that declares it
};

/// What the HUD shows of the settlement verbs (M2 task T17, ADR-153).
struct HudVerbs
{
  std::optional<StructureKind> placing;              // the structure the steward is placing
  std::optional<PlacementRefusal> foreseen;          // the refusal the client foresees at the pointer while placing
  std::optional<PlacementRefusal> refusal;           // the server's last refusal of a placement,
  double refusalAgeSeconds = 0.0;                    // and how long ago it came, in real time
  std::optional<std::array<std::uint8_t, 3>> shares; // the priorities sent, shown until a status holds them
  std::optional<bool> growthHeld;                    // O106 (ADR-178): the hold sent, shown until a status holds it
  // ADR-167: the attack being declared, and the server's last refusal of a conflict command with its kind and reason.
  std::optional<PendingAttack> pending;
  std::optional<std::pair<ConflictKind, std::uint8_t>> conflictRefusal;
  double conflictRefusalAgeSeconds = 0.0;
  std::uint64_t resultsClosed = 0; // ADR-168: the presses on the result card, each closing the result it shows
};

/// What the HUD shows (MVP-102 at M2): the client's own settlement in the replica, whose stores and population are
/// fresh every tick; the server's latest status (MVP-098) for its growth, priorities, tasks, idle Workers, upkeep and
/// queue; the construction bar's costs (MVP-034); the notices; and the world's seed (MVP-005).
struct HudModel
{
  std::optional<ReplicatedSettlement> settlement; // none: no settlement of the client's, and no HUD (the sandbox)
  std::optional<SettlementStatus> status;
  std::optional<std::uint64_t> seed;
  WorldPace pace = WorldPace::Session; // MVP-071 (ADR-187)
  bool awayReport = false;             // MVP-074 (O109, ADR-188): an away report of an absence has come, which its button opens
  double idleMinutes = 0.0;            // mvp.presence.idle_minutes (MVP-072)
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
  // M3 task T12 (ADR-167): the icons of the attacks' and claims' world markers of the frame; T13 (ADR-168): the results
  // of the settlement's battles not yet closed, the first on the result card.
  std::vector<MarkerIconPlace> markers;
  std::vector<BattleResultNotice> results;
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
  Claimable,     // an area the steward's settlement may claim now (M3 task T12, ADR-167)
  More           // the rows the panel's bound leaves out
};

/// A row of the relations panel: its kind; the settlement it names by its local site (the Cladan, the attacker, the
/// expedition's target, the claimant); a value (anger or weight in whole units, the seconds or minutes left, the rows
/// left out); the kind's detail (a grievance's cause, a claim's or a claimable area, an expedition's state) and state (a
/// claim's state, a grievance whose cause is removed, a Cladan under a respite); and, for its actions (ADR-167), what
/// it names by id (a claim, a grievance's subject) and a grievance's weight in millionths.
struct RelationRow
{
  RelationRowKind kind = RelationRowKind::Heading;
  std::uint8_t site = 0;
  std::int64_t value = 0;
  std::uint8_t detail = 0;
  std::uint8_t state = 0;
  std::uint64_t subject = 0;
  std::int64_t weightMicro = 0;

  friend bool operator==(const RelationRow&, const RelationRow&) = default;
};

// MVP-101 (ADR-167): an action a row of the relations panel offers on a button at its end.
enum class RelationAction : std::uint8_t
{
  None,
  Raid,           // a Cladan's row: declare a raid on its settlement
  ClaimBattle,    // the steward's own open or contested claim: declare its claim battle
  WithdrawClaim,  // the steward's own claim
  WithdrawAttack, // the steward's own expedition
  Tribute,        // a grievance held against the steward's settlement: ease it whole
  Claim           // a claimable area
};

// The actions of a row for the steward's settlement at site _site, at most two, the first nearest the row's end.
[[nodiscard]] std::array<RelationAction, 2> RelationActionsOf(const RelationRow& _row, std::uint8_t _site) noexcept;

/// MVP-048 (ADR-168): a row of the result card of a battle the steward's settlement was a side of.
enum class ResultRowKind : std::uint8_t
{
  Title,     // value: 1 won, 0 lost
  End,       // value: the BattleEnd; second: the battle's length in whole seconds
  Lost,      // value: the attacker's Workers lost; second: the defender's
  Stores,    // value: the stores the attacker carried off, in whole resource units
  Essence,   // value: the Essence the defender took from the attackers who fell in its territory
  Area,      // the claimed area moved to the winner
  Grievance, // value: the grievance the attack gave the defender, in whole units of weight
  Respite,   // value: the defender's respite from the battle's end, in whole minutes
  Renown,    // value: the renown the battle gave its winner
  Close      // value: the results waiting after this one
};

struct ResultRow
{
  ResultRowKind kind = ResultRowKind::Title;
  std::int64_t value = 0;
  std::int64_t second = 0;

  friend bool operator==(const ResultRow&, const ResultRow&) = default;
};

// MVP-048 (ADR-168): the rows of _result's card for the steward's settlement at site _site, with ticks _tickSeconds long
// and _waiting results after it: the outcome, the end and the battle's length, each side's Workers lost, those of the
// stores and Essence moved, the area moved, the grievance, the respite and the renown that are not none, and the close.
[[nodiscard]] std::vector<ResultRow> ResultRowsOf(const BattleResultNotice& _result, std::uint8_t _site, double _tickSeconds,
                                                  std::size_t _waiting);

// MVP-013 (ADR-166): a Cladan's name by its colour, `ui.hud.cladan` with its site's selector.
[[nodiscard]] std::string CladanName(std::uint8_t _site, const Neuron::StringTable& _strings, const Neuron::MessageLocale& _locale);
// ADR-166: an area's name: the central site, or a half of a valley border area by the settlement whose half it is and the
// neighbour it faces.
[[nodiscard]] std::string AreaName(std::uint8_t _area, const Neuron::StringTable& _strings, const Neuron::MessageLocale& _locale);

// MVP-062 (ADR-169): the settlements' renown in the order of the standing, the most first and the lower site on a tie, as
// the server's Standing orders them.
[[nodiscard]] std::vector<RenownLine> StandingOf(const ValleyStatus& _valley);
// A renown line's total.
[[nodiscard]] std::int64_t RenownTotal(const RenownLine& _line) noexcept;

// MVP-060 (ADR-169): where the run stands at the replica's tick _tick, ticks _tickSeconds long: running or in its quiet
// minutes with the whole seconds left, or ended.
struct RunTime
{
  enum class State : std::uint8_t
  {
    Running,
    Quiet,
    Ended
  };

  State state = State::Running;
  std::int64_t secondsLeft = 0;

  friend bool operator==(const RunTime&, const RunTime&) = default;
};
[[nodiscard]] RunTime RunTimeOf(const ValleyStatus& _valley, std::uint64_t _tick, double _tickSeconds) noexcept;

// MVP-050, MVP-102 (ADR-166): the relations panel's rows for the settlement at site _site after tick _tick of the
// valley status: the warning of each attack declared on it, its own expedition, its respite, each other Cladan's anger
// toward it with the grievances that make it, and every claim; at most _rowsMax rows, the last of a longer list saying
// how many it leaves out.
[[nodiscard]] std::vector<RelationRow> RelationRowsOf(const ValleyStatus& _valley, std::uint8_t _site, std::uint64_t _tick,
                                                      double _tickSeconds, double _warningSeconds, std::size_t _rowsMax);

// The HUD's screen zones (UI-461, R-CTRL-223): a press over one reaches no world verb (ADR-153).
enum class HudZone : std::uint8_t
{
  Settlement,     // the settlement panel
  Build,          // a construction bar entry; its index is BUILD_KINDS's
  Queue,          // the construction queue
  Notices,        // the notification cards
  World,          // the world's information panel
  PriorityRaise,  // a priority's raising arrow; its index is the task's, from gathering (ADR-153)
  PriorityLower,  // a priority's lowering arrow
  Relations,      // the relations panel (ADR-166)
  RelationAction, // a button of a relations row (ADR-167); its index is the row's place × 2 + the action's
  Result,         // the result card (ADR-168)
  GrowthHold,     // the growth row's button that holds the growth or lets it run (O106, ADR-178)
  AwayReport      // the always-on row's button that opens the latest away report (O109, ADR-188)
};

struct HudZoneRect
{
  HudZone zone = HudZone::Settlement;
  std::uint8_t index = 0;
  Neuron::UiRect rect;
};

/// The plates the HUD's panels and buttons are drawn on (ADR-175): the frames of uitex.panel_accent and
/// uitex.panel_neutral (PlateOf). Without them each image is drawn whole over its box, with no frame.
struct HudPlates
{
  Neuron::NineSlice accent;
  Neuron::NineSlice neutral;
};

/// A row of a HUD panel or card (ADR-175), as the layout measures it and Draw draws it: its text and color, where its
/// icon and its text start in the panel's content, a meter under its text, a text at its end and the room it keeps there.
struct HudRow
{
  std::string text;
  std::array<float, 4> color{};
  float indent = 0.0f; // pixels from the content's left edge to the text
  std::optional<UiImage> icon;
  std::array<float, 4> iconColor{};
  float iconIndent = 0.0f;     // pixels from the content's left edge to the icon
  std::optional<double> meter; // a meter of this fraction under the text, to the row's end less its trailing room
  std::string end;             // a text right-aligned at the row's end, before its trailing room
  std::array<float, 4> endColor{};
  float trailing = 0.0f;           // pixels the row keeps at its end, for its buttons or arrows
  std::optional<std::size_t> task; // a priority's row: the task whose arrows it holds (ADR-153)
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
/// Workers and the population limit, growth and the cause when it stops with the button that holds it (O106, ADR-178),
/// the upkeep and the inactive structures, the
/// priorities with their Workers, and the idle Workers by reason (MVP-037); the construction bar holds each structure
/// with its cost, the queue each site with its progress; the notification cards the structures completed; and the
/// world's panel its seed. From M3 task T11 (ADR-166) it asks for the valley's status with the settlement's, shows the
/// relations panel of RelationRowsOf, and draws a card for every kind of notice. Since ADR-175 each panel and button is a
/// framed plate of the owner's panel art, and each panel and card is as wide as its widest row needs. Its looks and
/// measures are `mvp.hud.*`, provisional (O95, DEC-1005). Main thread only.
class SettlementHud
{
public:
  SettlementHud(const TuningPresentation& _presentation, const TuningMvp& _tuning);

  // Once a frame in the UI's update (UI-460): asks the server for the settlement's status every mvp.hud.status_period of
  // real time (MVP-098), takes the session's notices and the verbs' state, and lays the panels out for _metrics,
  // producing their zones, which the pointer's hit test reads from the next frame (UI-461).
  void Update(ClientSession& _session, double _realSeconds, const Neuron::UiMetrics& _metrics, const HudVerbs& _verbs,
              std::span<const MarkerIconPlace> _markers = {});
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
  // ADR-167: the relations panel's rows of the last layout, which its buttons' zones index.
  [[nodiscard]] std::span<const RelationRow> RelationRows() const noexcept
  {
    return m_relationRows;
  }
  // ADR-173: the notices that arrived at the last update, in their order, for the sounds they play.
  [[nodiscard]] std::span<const SettlementNotice> Arrived() const noexcept
  {
    return m_arrived;
  }

  // For a test: the model and the layout as an update would make them.
  void SetModel(HudModel _model, const Neuron::UiMetrics& _metrics);
  // ADR-175: the plates of the content's panel images.
  void SetPlates(const HudPlates& _plates) noexcept
  {
    m_plates = _plates;
  }
  // ADR-175: what the layout measures each panel's rows with, as Draw lays them out, so each panel is as wide as its
  // widest row needs; until it is given, each panel keeps its least width.
  void SetText(Neuron::TextSystem& _text, const Neuron::StringTable& _strings, const Neuron::MessageLocale& _locale) noexcept;

private:
  void Layout(const Neuron::UiMetrics& _metrics);
  [[nodiscard]] float Units(double _hudUnits) const noexcept;
  // The final size of the HUD's text in pixels (UI-171).
  [[nodiscard]] float TextPixels() const noexcept;
  // ADR-175: each panel's rows and each card's, in the order they are drawn.
  [[nodiscard]] std::vector<HudRow> SettlementRows(const Neuron::StringTable& _strings, const Neuron::MessageLocale& _locale) const;
  [[nodiscard]] std::vector<HudRow> WorldRows(const Neuron::StringTable& _strings, const Neuron::MessageLocale& _locale) const;
  [[nodiscard]] std::vector<HudRow> RelationPanelRows(const Neuron::StringTable& _strings, const Neuron::MessageLocale& _locale) const;
  [[nodiscard]] std::vector<HudRow> ResultPanelRows(const Neuron::StringTable& _strings, const Neuron::MessageLocale& _locale) const;
  [[nodiscard]] HudRow NoticeRow(const HudNotice& _notice, double _alpha, const Neuron::StringTable& _strings,
                                 const Neuron::MessageLocale& _locale) const;
  // The name and the cost of the construction bar's entry _index (MVP-032, MVP-034).
  [[nodiscard]] std::array<HudRow, 2> BuildRows(std::size_t _index, const Neuron::StringTable& _strings,
                                                const Neuron::MessageLocale& _locale) const;
  // ADR-175: the widest of _rows in whole pixels, its text measured at the HUD's size; 0 with no text system given.
  [[nodiscard]] float Widest(std::span<const HudRow> _rows) const;
  // ADR-175: the pixels a texel of a plate's frame takes on a panel, or on a button.
  [[nodiscard]] float PlatePixels(bool _button) const noexcept;
  // ADR-175: the size of a panel's plate around content of _width × _height pixels.
  [[nodiscard]] std::array<float, 2> PlateSize(const Neuron::NineSlice& _plate, float _width, float _height) const noexcept;
  // ADR-175: the part of a plate inside its frame, a panel's or a button's.
  [[nodiscard]] Neuron::UiRect Inside(const Neuron::NineSlice& _plate, const Neuron::UiRect& _rect, bool _button) const noexcept;
  // ADR-167: the button of action _slot of the relations row at _place, an icon's square at the row's end.
  [[nodiscard]] Neuron::UiRect RelationButton(std::size_t _place, std::size_t _slot) const noexcept;

  const TuningPresentation& m_presentation;
  const TuningMvp& m_tuning;
  HudModel m_model;
  Neuron::UiMetrics m_metrics;
  std::vector<HudZoneRect> m_zones;
  Neuron::UiRect m_panel;
  Neuron::UiRect m_world;
  std::array<Neuron::UiRect, BUILD_KINDS.size()> m_build{};
  std::array<std::array<Neuron::UiRect, 2>, 3> m_arrows{}; // each priority's lowering and raising arrows (ADR-153)
  Neuron::UiRect m_holdButton;                             // the growth row's hold button (O106, ADR-178)
  Neuron::UiRect m_awayButton;                             // the always-on row's away report button (O109, ADR-188)
  Neuron::UiRect m_queue;
  Neuron::UiRect m_relations;
  std::vector<RelationRow> m_relationRows;
  std::vector<SettlementNotice> m_arrived;
  std::deque<BattleResultNotice> m_results;     // ADR-168: the results of its battles not yet closed
  std::optional<std::uint64_t> m_resultsClosed; // the verbs' closing presses already applied; none before the first update
  Neuron::UiRect m_result;
  std::vector<ResultRow> m_resultRows;
  std::size_t m_panelRows = 0;
  std::optional<double> m_lastRequest;
  // ADR-175: the plates, what the layout measures with, and each panel's content inside its plate's frame.
  HudPlates m_plates;
  Neuron::TextSystem* m_textSystem = nullptr;
  const Neuron::StringTable* m_strings = nullptr;
  const Neuron::MessageLocale* m_locale = nullptr;
  Neuron::UiRect m_panelInner;
  Neuron::UiRect m_worldInner;
  Neuron::UiRect m_relationsInner;
  Neuron::UiRect m_resultInner;
  float m_cardWidth = 0.0f;
  float m_buttonWidth = 0.0f;
};

} // namespace Species
