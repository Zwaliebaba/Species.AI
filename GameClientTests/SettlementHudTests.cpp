#include "pch.h"

#include "MessageLocale.h"
#include "SettlementHud.h"
#include "StringTable.h"
#include "StructureCost.h"
#include "TestSupport.h"
#include "UiLook.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <span>
#include <string_view>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameClientTests
{

namespace
{

Neuron::UiMetrics MetricsAt(Neuron::SizePx _client)
{
  return Species::UiMetricsOf(CommittedPresentation(), _client, 1.0f, std::nullopt, std::nullopt);
}

// A valley status for the settlement at site 0 after tick 1,000: grievances of two Cladans against it and one it holds,
// an attack declared on it and its own expedition, its respite and another's, and two claims (ADR-166).
Species::ValleyStatus ValleyOfSiteZero()
{
  Species::ValleyStatus valley;
  valley.tick = 1'000;
  for (std::uint8_t site = 0; site < 4; ++site)
  {
    valley.renown.push_back({site});
  }
  valley.grievances = {{1, 0, Species::GrievanceCause::Pressure, 0, false, false, 12'400'000},
                       {1, 0, Species::GrievanceCause::Claim, 77, false, true, 30'000'000},
                       {2, 0, Species::GrievanceCause::Attack, 78, true, false, 40'000'000},
                       {0, 1, Species::GrievanceCause::Pressure, 0, false, false, 5'000'000}};
  valley.expeditions = {
    {Species::MakeStableId(Species::REGION_OWNER_ID_FIRST, 78), 2, 0, 0, 900, Species::ExpeditionState::Held, true, 40},
    {Species::MakeStableId(Species::REGION_OWNER_ID_FIRST, 79), 0, 3, 0, 500, Species::ExpeditionState::Released, false, 30}};
  valley.respites = {{0, 2'200}, {3, 1'600}};
  valley.claims = {
    {Species::MakeStableId(Species::REGION_OWNER_ID_FIRST, 77), 1, 0, 400, Species::ClaimState::Open},
    {Species::MakeStableId(Species::REGION_OWNER_ID_FIRST, 80), 3, Species::CLAIM_AREA_CENTRAL, 600, Species::ClaimState::Waiting}};
  return valley;
}

Species::HudModel SettlementModel()
{
  Species::HudModel model;
  Species::ReplicatedSettlement own;
  own.team = 0;
  own.storesMilliRu = 120'000;
  own.capacityMilliRu = 400'000;
  own.population = 24;
  own.limit = 30;
  model.settlement = own;
  Species::SettlementStatus status;
  status.growth = Species::GrowthState::AtLimit;
  status.priorities = {50, 30, 20};
  status.tasks = {12, 7, 5};
  status.idle = {2, 0, 1, 0};
  status.queue = {{11, Species::StructureKind::Dwelling, 250}, {12, Species::StructureKind::Storehouse, 0}};
  model.status = status;
  model.seed = 4'294'967'296;
  model.buildCostsMilliRu = {100'000, 150'000, 300'000};
  Species::SettlementNotice notice;
  notice.structure = Species::StructureKind::Dwelling;
  model.notices = {{notice, 1.0}, {notice, 2.0}};
  model.nowSeconds = 3.0;
  return model;
}

} // namespace

TEST_CLASS(SettlementHudTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp102TheHudsPanelsAreItsZones)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-102")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-037")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-461")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp102TheHudsPanelsAreItsZones)
  {
    Species::SettlementHud hud(CommittedPresentation(), CommittedTuning());
    const Neuron::UiMetrics metrics = MetricsAt({1280, 720});
    hud.SetModel(SettlementModel(), metrics);
    // The settlement panel, three construction bar entries, the queue, the notices and the world's panel; and each
    // priority's two arrows (ADR-153).
    ExpectEqual(7 + 6, static_cast<std::int64_t>(hud.Zones().size()), L"every panel and arrow is a zone");
    const auto& keys = CommittedPresentation().mvp.hud;
    const double unit = 720.0 / 600.0;
    const Neuron::UiRect panel = hud.Zones()[0].rect;
    // MVP-037: the seven fixed rows, the idle heading and a row for each reason with Workers (two of four).
    const double rows = 7 + 1 + 2;
    Assert::AreEqual(std::round(keys.padding * unit) * 2.0 + std::round(keys.rowHeight * unit) * rows, static_cast<double>(panel.Height()),
                     1e-3, L"the panel holds its rows");
    const auto found = std::ranges::find_if(hud.Zones(), [](const Species::HudZoneRect& _zone)
                                            { return _zone.zone == Species::HudZone::Build && _zone.index == 1; });
    Assert::IsTrue(found != hud.Zones().end(), L"the bar's second entry, the storehouse");
    const Species::HudZoneRect storehouse = *found;
    const auto hit =
      hud.ZoneAt({static_cast<std::int32_t>(storehouse.rect.left + 2.0f), static_cast<std::int32_t>(storehouse.rect.top + 2.0f)});
    Assert::IsTrue(hit.has_value() && hit->zone == Species::HudZone::Build && hit->index == 1, L"the pointer over it finds it");
    Assert::IsFalse(hud.ZoneAt({640, 360}).has_value(), L"the middle of the view is the world's");
    // ADR-153: an arrow over its panel is found before the panel.
    const auto raise = std::ranges::find_if(hud.Zones(), [](const Species::HudZoneRect& _zone)
                                            { return _zone.zone == Species::HudZone::PriorityRaise && _zone.index == 2; });
    Assert::IsTrue(raise != hud.Zones().end() && panel.Contains(raise->rect.left + 1.0f, raise->rect.top + 1.0f), L"guard's raise arrow");
    const auto arrow = hud.ZoneAt({static_cast<std::int32_t>(raise->rect.left + 1.0f), static_cast<std::int32_t>(raise->rect.top + 1.0f)});
    Assert::IsTrue(arrow.has_value() && arrow->zone == Species::HudZone::PriorityRaise && arrow->index == 2, L"the arrow, not the panel");

    Species::HudModel sandbox;
    hud.SetModel(sandbox, metrics);
    Assert::IsTrue(hud.Zones().empty(), L"no settlement of the client's, no HUD");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp102TheHudDrawsItsMetersCardsAndText)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-102")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-460")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp102TheHudDrawsItsMetersCardsAndText)
  {
    Species::SettlementHud hud(CommittedPresentation(), CommittedTuning());
    hud.SetModel(SettlementModel(), MetricsAt({1920, 1080}));
    Neuron::TextSystem text(Species::TextSystemDesc(CommittedPresentation()));
    const Neuron::StringTable strings;
    const Neuron::MessageLocale locale("en-US");
    std::vector<Neuron::UiBatch> batches;
    hud.Draw(text, strings, locale, batches);
    const auto imageQuads = [&](Species::UiImage _image)
    {
      std::int64_t quads = 0;
      for (const Neuron::UiBatch& batch : batches)
      {
        if (batch.source == Neuron::UiSource::Image && batch.image == static_cast<std::uint16_t>(_image))
        {
          quads += static_cast<std::int64_t>(batch.quads.size());
        }
      }
      return quads;
    };
    // The meters: the stores, the Workers, the three priorities and the two queued sites, each a track and a fill but
    // for the empty site's.
    ExpectEqual(7 * 2 - 1, imageQuads(Species::UiImage::HudMeter), L"each meter's track and fill");
    ExpectEqual(2, imageQuads(Species::UiImage::NotificationCard), L"a card for each notice");
    ExpectEqual(1 + 1 + 2, imageQuads(Species::UiImage::IconDwelling), L"the dwelling on the bar, in the queue and on both cards");
    ExpectEqual(6, imageQuads(Species::UiImage::ArrowPointer), L"each priority's two arrows (ADR-153)");
    ExpectEqual(2, imageQuads(Species::UiImage::PanelAccent), L"the settlement and world panels in the accent strip");
    Assert::IsTrue(std::ranges::any_of(batches, [](const Neuron::UiBatch& _batch) { return _batch.source == Neuron::UiSource::Glyphs; }),
                   L"and its text");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp101TheHudShowsThePlacingTheRefusalAndTheSharesSent)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-101")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-036")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp101TheHudShowsThePlacingTheRefusalAndTheSharesSent)
  {
    Species::SettlementHud hud(CommittedPresentation(), CommittedTuning());
    Neuron::TextSystem text(Species::TextSystemDesc(CommittedPresentation()));
    const Neuron::StringTable strings;
    const Neuron::MessageLocale locale("en-US");
    const auto drawn = [&](const Species::HudVerbs& _verbs)
    {
      Species::HudModel model = SettlementModel();
      model.verbs = _verbs;
      hud.SetModel(model, MetricsAt({1920, 1080}));
      std::vector<Neuron::UiBatch> batches;
      hud.Draw(text, strings, locale, batches);
      return batches;
    };
    const auto count = [](const std::vector<Neuron::UiBatch>& _batches, Neuron::UiSource _source, std::uint16_t _image)
    {
      std::int64_t quads = 0;
      for (const Neuron::UiBatch& batch : _batches)
      {
        if (batch.source == _source && (_source != Neuron::UiSource::Image || batch.image == _image))
        {
          quads += static_cast<std::int64_t>(batch.quads.size());
        }
      }
      return quads;
    };
    const auto accent = static_cast<std::uint16_t>(Species::UiImage::PanelAccent);
    const std::vector<Neuron::UiBatch> idle = drawn({});
    Species::HudVerbs placing;
    placing.placing = Species::StructureKind::Storehouse;
    const std::vector<Neuron::UiBatch> placed = drawn(placing);
    ExpectEqual(count(idle, Neuron::UiSource::Image, accent) + 1, count(placed, Neuron::UiSource::Image, accent),
                L"the entry being placed in the accent strip");
    Assert::IsTrue(count(placed, Neuron::UiSource::Glyphs, 0) > count(idle, Neuron::UiSource::Glyphs, 0), L"and a line above the queue");
    Species::HudVerbs refused;
    refused.refusal = Species::PlacementRefusal::CannotAfford;
    refused.refusalAgeSeconds = CommittedPresentation().mvp.hud.noticeSeconds;
    ExpectEqual(count(idle, Neuron::UiSource::Glyphs, 0), count(drawn(refused), Neuron::UiSource::Glyphs, 0), L"a refusal past its time");
    refused.refusalAgeSeconds = 0.0;
    Assert::IsTrue(count(drawn(refused), Neuron::UiSource::Glyphs, 0) > count(idle, Neuron::UiSource::Glyphs, 0), L"a fresh refusal shows");
    // MVP-036: the shares sent fill the meters until a status holds them.
    Species::HudVerbs sent;
    sent.shares = std::array<std::uint8_t, 3>{100, 0, 0};
    const std::vector<Neuron::UiBatch> moved = drawn(sent);
    ExpectEqual(count(idle, Neuron::UiSource::Image, static_cast<std::uint16_t>(Species::UiImage::HudMeter)) - 2,
                count(moved, Neuron::UiSource::Image, static_cast<std::uint16_t>(Species::UiImage::HudMeter)),
                L"two empty priority meters draw no fill");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp102ANoticeStaysItsSecondsAndTheNewestAreKept)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-102")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp102ANoticeStaysItsSecondsAndTheNewestAreKept)
  {
    const auto& keys = CommittedPresentation().mvp.hud;
    std::vector<Species::SettlementNotice> arrived(static_cast<std::size_t>(keys.noticesMax) + 2);
    for (std::size_t index = 0; index < arrived.size(); ++index)
    {
      arrived[index].subject = index + 1;
    }
    std::vector<Species::HudNotice> notices = Species::AgeNotices({}, arrived, 10.0, CommittedPresentation());
    ExpectEqual(keys.noticesMax, static_cast<std::int64_t>(notices.size()), L"at most mvp.hud.notices_max");
    Assert::IsTrue(notices.back().notice.subject == arrived.back().subject, L"the newest kept");
    notices = Species::AgeNotices(std::move(notices), {}, 10.0 + keys.noticeSeconds - 0.01, CommittedPresentation());
    Assert::IsFalse(notices.empty(), L"shown until mvp.hud.notice_seconds");
    notices = Species::AgeNotices(std::move(notices), {}, 10.0 + keys.noticeSeconds, CommittedPresentation());
    Assert::IsTrue(notices.empty(), L"and gone then");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp050TheRelationsPanelListsAngerGrievancesClaimsAndTheWarning)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-050")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-102")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp050TheRelationsPanelListsAngerGrievancesClaimsAndTheWarning)
  {
    // ADR-166: the warning counts down to the end of mvp.conflict.warning_minutes; the steward's expedition and respite
    // follow; each other Cladan's anger is the sum of its grievances against the steward's settlement, listed under it
    // with a removed cause marked, the grievance the settlement itself holds not among them; then every claim. ADR-167:
    // a grievance carries its subject and weight for its tribute, a claim its id, and the areas the settlement may claim
    // follow the claims: here the central site, claimed by another and held by none (site 1's half of the pair is held by
    // no one in this status, so it is not claimable).
    using Kind = Species::RelationRowKind;
    const double warning = CommittedTuning().mvp.conflict.warningMinutes * 60.0;
    const std::vector<Species::RelationRow> rows = Species::RelationRowsOf(ValleyOfSiteZero(), 0, 1'000, 0.1, warning, 20);
    const auto warningLeft = static_cast<std::int64_t>(std::ceil(warning - 10.0)); // declared at tick 900, a second ago
    const std::vector<Species::RelationRow> expected{
      {Kind::Heading},
      {Kind::Warning, 2, warningLeft},
      {Kind::Expedition, 3, 0, static_cast<std::uint8_t>(Species::ExpeditionState::Released)},
      {Kind::Respite, 0, 2},
      {Kind::Cladan, 1, 42, 0, 0},
      {Kind::Grievance, 1, 12, 0, 0, 0, 12'400'000},
      {Kind::Grievance, 1, 30, 1, 1, 77, 30'000'000},
      {Kind::Cladan, 2, 40, 0, 0},
      {Kind::Grievance, 2, 40, 2, 0, 78, 40'000'000},
      {Kind::Cladan, 3, 0, 0, 1},
      {Kind::ClaimsHeading},
      {Kind::Claim, 1, 0, 0, 0, Species::MakeStableId(Species::REGION_OWNER_ID_FIRST, 77)},
      {Kind::Claim, 3, 0, Species::CLAIM_AREA_CENTRAL, 1, Species::MakeStableId(Species::REGION_OWNER_ID_FIRST, 80)},
      {Kind::Claimable, 0, 0, Species::CLAIM_AREA_CENTRAL}};
    ExpectEqual(static_cast<std::int64_t>(expected.size()), static_cast<std::int64_t>(rows.size()), L"every row");
    Assert::IsTrue(rows == expected, L"in order, with their values");
    const std::vector<Species::RelationRow> bounded = Species::RelationRowsOf(ValleyOfSiteZero(), 0, 1'000, 0.1, warning, 6);
    Assert::IsTrue(bounded.size() == 6 && bounded.back().kind == Kind::More && bounded.back().value == 9,
                   L"the bound says what it leaves out");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp102TheRelationsPanelAndTheNoticeCardsOfM3)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-102")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp102TheRelationsPanelAndTheNoticeCardsOfM3)
  {
    // ADR-166: with a valley status the relations panel is a zone at the bottom right, a row high each, and draws its
    // icons; each notice of M3's kinds has a card with its own icon.
    Species::HudModel model = SettlementModel();
    model.valley = ValleyOfSiteZero();
    model.tick = 1'000;
    model.warningSeconds = 300.0;
    Species::SettlementNotice claim;
    claim.kind = Species::NoticeKind::ClaimMade;
    claim.other = std::uint8_t{1};
    claim.detail = 0;
    Species::SettlementNotice ended;
    ended.kind = Species::NoticeKind::BattleEnded;
    ended.other = std::uint8_t{2};
    ended.detail = 1;
    model.notices = {{claim, 1.0}, {ended, 2.0}};
    Species::SettlementHud hud(CommittedPresentation(), CommittedTuning());
    const Neuron::UiMetrics metrics = MetricsAt({1920, 1080});
    hud.SetModel(model, metrics);
    const auto relations = std::ranges::find(hud.Zones(), Species::HudZone::Relations, &Species::HudZoneRect::zone);
    Assert::IsTrue(relations != hud.Zones().end(), L"the relations panel is a zone");
    const auto& keys = CommittedPresentation().mvp.hud;
    const double unit = 1080.0 / 600.0;
    Assert::AreEqual(std::round(keys.padding * unit) * 2.0 + std::round(keys.rowHeight * unit) * 14.0,
                     static_cast<double>(relations->rect.Height()), 1e-3, L"a row high for each of its 14 rows");
    Assert::AreEqual(1920.0 - std::round(keys.margin * unit), static_cast<double>(relations->rect.right), 1e-3, L"at the right");
    Assert::AreEqual(1080.0 - std::round(keys.margin * unit), static_cast<double>(relations->rect.bottom), 1e-3, L"at the bottom");
    Neuron::TextSystem text(Species::TextSystemDesc(CommittedPresentation()));
    const Neuron::StringTable strings;
    const Neuron::MessageLocale locale("en-US");
    std::vector<Neuron::UiBatch> batches;
    hud.Draw(text, strings, locale, batches);
    const auto imageQuads = [&](Species::UiImage _image)
    {
      std::int64_t quads = 0;
      for (const Neuron::UiBatch& batch : batches)
      {
        quads += batch.source == Neuron::UiSource::Image && batch.image == static_cast<std::uint16_t>(_image)
                   ? static_cast<std::int64_t>(batch.quads.size())
                   : 0;
      }
      return quads;
    };
    ExpectEqual(6, imageQuads(Species::UiImage::IconAttack), L"the warning, the expedition, the attack cause and three raids");
    ExpectEqual(2, imageQuads(Species::UiImage::IconRespite), L"its respite and another Cladan's");
    ExpectEqual(6, imageQuads(Species::UiImage::IconClaim), L"two claims, a claim cause, the claimable site and its button, the card");
    ExpectEqual(3, imageQuads(Species::UiImage::IconTribute), L"a tribute for each grievance against it");
    ExpectEqual(1, imageQuads(Species::UiImage::IconWithdraw), L"its expedition's withdrawal");
    ExpectEqual(1, imageQuads(Species::UiImage::IconContact), L"the pressure cause");
    ExpectEqual(1, imageQuads(Species::UiImage::IconBattle), L"the battle's card");
    ExpectEqual(3, imageQuads(Species::UiImage::PanelAccent), L"the settlement, world and relations panels");
    model.valley.reset();
    hud.SetModel(model, metrics);
    Assert::IsTrue(
      std::ranges::none_of(hud.Zones(), [](const Species::HudZoneRect& _zone) { return _zone.zone == Species::HudZone::Relations; }),
      L"no panel before the first valley status");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp101EachRowsActionsAreButtonsAtItsEnd)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-101")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-461")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp101EachRowsActionsAreButtonsAtItsEnd)
  {
    // ADR-167: a raid on each Cladan, a tribute for each grievance against the steward's settlement, the withdrawal of its
    // expedition and a claim on the claimable central site: each a zone at its row's end, found before the panel.
    Species::HudModel model = SettlementModel();
    model.valley = ValleyOfSiteZero();
    model.tick = 1'000;
    model.warningSeconds = 300.0;
    Species::SettlementHud hud(CommittedPresentation(), CommittedTuning());
    hud.SetModel(model, MetricsAt({1920, 1080}));
    std::vector<Species::HudZoneRect> buttons;
    std::ranges::copy_if(hud.Zones(), std::back_inserter(buttons),
                         [](const Species::HudZoneRect& _zone) { return _zone.zone == Species::HudZone::RelationAction; });
    ExpectEqual(8, static_cast<std::int64_t>(buttons.size()), L"three raids, three tributes, a withdrawal and a claim");
    const auto panel = std::ranges::find(hud.Zones(), Species::HudZone::Relations, &Species::HudZoneRect::zone);
    const double unit = 1080.0 / 600.0;
    const auto& keys = CommittedPresentation().mvp.hud;
    for (const Species::HudZoneRect& button : buttons)
    {
      const std::span<const Species::RelationRow> rows = hud.RelationRows();
      const Species::RelationRow& row = rows[button.index / 2U];
      Assert::IsTrue(Species::RelationActionsOf(row, 0)[button.index % 2U] != Species::RelationAction::None, L"a row's action");
      Assert::AreEqual(static_cast<double>(panel->rect.right) - std::round(keys.padding * unit), static_cast<double>(button.rect.right),
                       1e-3, L"its first button at the row's end");
      const Neuron::PointPx center{static_cast<std::int32_t>((button.rect.left + button.rect.right) * 0.5f),
                                   static_cast<std::int32_t>((button.rect.top + button.rect.bottom) * 0.5f)};
      Assert::IsTrue(hud.ZoneAt(center).value_or(Species::HudZoneRect{}).zone == Species::HudZone::RelationAction, L"over the panel");
    }
    Assert::IsTrue(Species::RelationActionsOf({Species::RelationRowKind::Claim, 0, 0, 2, 0}, 0) ==
                     std::array{Species::RelationAction::WithdrawClaim, Species::RelationAction::ClaimBattle},
                   L"an open claim of its own: withdrawn, or its battle declared");
    Assert::IsTrue(Species::RelationActionsOf({Species::RelationRowKind::Claim, 0, 0, 2, 1}, 0) ==
                     std::array{Species::RelationAction::WithdrawClaim, Species::RelationAction::None},
                   L"a waiting claim has no battle");
    Assert::IsTrue(Species::RelationActionsOf({Species::RelationRowKind::Claim, 1, 0, 2, 0}, 0)[0] == Species::RelationAction::None,
                   L"another's claim, none");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp062TheStandingAndTheTimeLeft)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-060")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-062")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp062TheStandingAndTheTimeLeft)
  {
    // ADR-169: the standing orders the settlements by their renown's total, the lower site first on a tie, as the
    // server's Standing does; the time left counts down to the run's end from the replica's tick, in the quiet minutes
    // from their first tick, and the run that ended has none. The world panel grows a row for each, and a heading.
    Species::ValleyStatus valley;
    valley.renown = {{0, 1, 0, 0, 2}, {1, 3, 3, 0, 0}, {2, 0, 0, 3, 0}, {3, 6, 0, 0, 0}};
    const std::vector<Species::RenownLine> standing = Species::StandingOf(valley);
    Assert::IsTrue(standing.size() == 4 && standing[0].site == 1 && standing[1].site == 3 && standing[2].site == 0 && standing[3].site == 2,
                   L"6, 6 by site, then 3 and 3 by site");
    ExpectEqual(6, Species::RenownTotal(standing[0]), L"its sources' sum");
    valley.runEndTick = 10'000;
    valley.quietTick = 9'000;
    using State = Species::RunTime::State;
    Assert::IsTrue(Species::RunTimeOf(valley, 1'000, 0.1) == Species::RunTime{State::Running, 900}, L"900 s left");
    Assert::IsTrue(Species::RunTimeOf(valley, 9'995, 0.1) == Species::RunTime{State::Quiet, 1}, L"half a second left rounds up");
    Assert::IsTrue(Species::RunTimeOf(valley, 10'000, 0.1).state == State::Ended, L"ended at its tick");
    valley.runEnded = true;
    Assert::IsTrue(Species::RunTimeOf(valley, 1'000, 0.1).state == State::Ended, L"or when the server says so");
    Species::HudModel model = SettlementModel();
    Species::SettlementHud hud(CommittedPresentation(), CommittedTuning());
    hud.SetModel(model, MetricsAt({1920, 1080}));
    const auto heightOf = [&]
    { return std::ranges::find(hud.Zones(), Species::HudZone::World, &Species::HudZoneRect::zone)->rect.Height(); };
    const float alone = heightOf();
    model.valley = ValleyOfSiteZero();
    hud.SetModel(model, MetricsAt({1920, 1080}));
    const double row = std::round(CommittedPresentation().mvp.hud.rowHeight * 1080.0 / 600.0);
    Assert::AreEqual(row * 6.0, static_cast<double>(heightOf() - alone), 1e-3, L"the time left, the heading and four settlements");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp048AResultsRowsAreItsRecord)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-048")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-046")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp048AResultsRowsAreItsRecord)
  {
    // ADR-168: the outcome for the steward's settlement, the end and the battle's length, each side's Workers lost, then
    // the stores, Essence, area, grievance, respite and renown that are not none, and the close with the results waiting.
    using Kind = Species::ResultRowKind;
    Species::BattleResultNotice raid;
    raid.attacker = 0;
    raid.defender = 1;
    raid.end = Species::BattleEnd::LossCap;
    raid.attackerWon = true;
    raid.startTick = 1'000;
    raid.endTick = 1'000 + 1'234; // 123.4 s at 0.1 s a tick
    raid.side = true;
    raid.attackerLost = 12;
    raid.defenderLost = 30;
    raid.storesMovedMilliRu = 45'600;
    raid.essenceMoved = 7;
    raid.grievanceMicro = 20'400'000;
    raid.respiteUntil = raid.endTick + 9'001; // 15 min and a tick
    raid.renown = 5;
    const std::vector<Species::ResultRow> rows = Species::ResultRowsOf(raid, 0, 0.1, 2);
    const std::vector<Species::ResultRow> expected{{Kind::Title, 1},    {Kind::End, 1, 123}, {Kind::Lost, 12, 30},
                                                   {Kind::Stores, 45},  {Kind::Essence, 7},  {Kind::Grievance, 20},
                                                   {Kind::Respite, 16}, {Kind::Renown, 5},   {Kind::Close, 2}};
    Assert::IsTrue(rows == expected, L"the attacker's victory, every line");
    // The defender of the same battle lost it; a claim battle that moved its area says so; nothing moved, nothing listed.
    ExpectEqual(0, Species::ResultRowsOf(raid, 1, 0.1, 0).front().value, L"a defeat for the defender");
    Species::BattleResultNotice claim;
    claim.attacker = 2;
    claim.defender = 0;
    claim.area = std::uint8_t{4};
    claim.areaMoved = true;
    claim.end = Species::BattleEnd::ObjectiveHeld;
    claim.attackerWon = true;
    claim.side = true;
    const std::vector<Species::ResultRow> lost = Species::ResultRowsOf(claim, 0, 0.1, 0);
    const std::vector<Species::ResultRow> plain{{Kind::Title, 0}, {Kind::End, 0, 0}, {Kind::Lost, 0, 0}, {Kind::Area}, {Kind::Close, 0}};
    Assert::IsTrue(lost == plain, L"a lost claim battle that moved the area");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp048TheResultCardStandsAtTheTopCenter)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-048")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-102")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp048TheResultCardStandsAtTheTopCenter)
  {
    // ADR-168: the first result waiting is a card mvp.hud.result_width wide at the top centre, a row high per row, a zone
    // of its own, drawn with icon.hud.result_card; with none waiting, no card.
    Species::HudModel model = SettlementModel();
    Species::BattleResultNotice result;
    result.attacker = 1;
    result.defender = 0;
    result.side = true;
    model.results = {result, result};
    Species::SettlementHud hud(CommittedPresentation(), CommittedTuning());
    hud.SetModel(model, MetricsAt({1920, 1080}));
    const auto card = std::ranges::find(hud.Zones(), Species::HudZone::Result, &Species::HudZoneRect::zone);
    Assert::IsTrue(card != hud.Zones().end(), L"the card is a zone");
    const auto& keys = CommittedPresentation().mvp.hud;
    const double unit = 1080.0 / 600.0;
    const double width = std::round(keys.resultWidth * unit);
    Assert::AreEqual(width, static_cast<double>(card->rect.Width()), 1e-3, L"mvp.hud.result_width wide");
    Assert::AreEqual(std::round((1920.0 - width) * 0.5), static_cast<double>(card->rect.left), 1e-3, L"centred");
    Assert::AreEqual(std::round(keys.margin * unit), static_cast<double>(card->rect.top), 1e-3, L"at the top");
    Assert::AreEqual(std::round(keys.padding * unit) * 2.0 + std::round(keys.rowHeight * unit) * 4.0,
                     static_cast<double>(card->rect.Height()), 1e-3, L"the title, end, losses and close rows");
    Neuron::TextSystem text(Species::TextSystemDesc(CommittedPresentation()));
    const Neuron::StringTable strings;
    const Neuron::MessageLocale locale("en-US");
    std::vector<Neuron::UiBatch> batches;
    hud.Draw(text, strings, locale, batches);
    std::int64_t frames = 0;
    for (const Neuron::UiBatch& batch : batches)
    {
      frames += batch.source == Neuron::UiSource::Image && batch.image == static_cast<std::uint16_t>(Species::UiImage::ResultCard)
                  ? static_cast<std::int64_t>(batch.quads.size())
                  : 0;
    }
    ExpectEqual(1, frames, L"one card, its frame drawn");
    model.results.clear();
    hud.SetModel(model, MetricsAt({1920, 1080}));
    Assert::IsTrue(
      std::ranges::none_of(hud.Zones(), [](const Species::HudZoneRect& _zone) { return _zone.zone == Species::HudZone::Result; }),
      L"no card with none waiting");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp102TheM3SentencesReadAsWritten)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-102")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp102TheM3SentencesReadAsWritten)
  {
    // ADR-166: the committed en-US strings give the sentences the HUD builds from its selectors and names.
    const Neuron::StringTable& strings = CommittedStrings();
    const Neuron::MessageLocale locale("en-US");
    using Argument = Neuron::FormatArgument;
    const auto format = [&](std::string_view _id, std::initializer_list<Argument> _arguments)
    { return strings.Format(_id, std::span<const Argument>(_arguments.begin(), _arguments.size()), locale); };
    ExpectText("Red", format("ui.hud.cladan", {Argument{"cladan", std::string_view("red")}}), L"a Cladan by its colour");
    ExpectText("the central site", format("ui.hud.area", {Argument{"area", std::string_view("central")}}), L"the central site");
    ExpectText("Green's half facing Red",
               format("ui.hud.area", {Argument{"area", std::string_view("half")}, Argument{"owner", std::string_view("Green")},
                                      Argument{"facing", std::string_view("Red")}}),
               L"a border half");
    ExpectText("Red claims Green's half facing Red",
               format("ui.hud.notice", {Argument{"kind", std::string_view("claim_theirs")}, Argument{"other", std::string_view("Red")},
                                        Argument{"area", std::string_view("Green's half facing Red")}}),
               L"a claim notice");
    ExpectText("You won the battle with Yellow",
               format("ui.hud.notice", {Argument{"kind", std::string_view("battle_won")}, Argument{"other", std::string_view("Yellow")},
                                        Argument{"area", std::string_view("")}}),
               L"a battle won");
    ExpectText("Yellow attacks in 4:05",
               format("ui.hud.warning", {Argument{"cladan", std::string_view("Yellow")}, Argument{"minutes", std::int64_t{4}},
                                         Argument{"seconds", std::string_view("05")}}),
               L"the warning's countdown");
    ExpectText("Claim 30, fading fast",
               format("ui.hud.grievance", {Argument{"cause", std::string_view("claim")}, Argument{"weight", std::int64_t{30}},
                                           Argument{"removed", std::string_view("yes")}}),
               L"a grievance whose cause is removed");
    ExpectText("Blue: anger 42",
               format("ui.hud.anger", {Argument{"cladan", std::string_view("Blue")}, Argument{"anger", std::int64_t{42}}}),
               L"a Cladan's anger");
    ExpectText(
      "Blue claims the central site: waiting",
      format("ui.hud.claim", {Argument{"claimant", std::string_view("Blue")}, Argument{"area", std::string_view("the central site")},
                              Argument{"state", std::string_view("waiting")}}),
      L"a claim");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp101TheConflictAndResultSentencesReadAsWritten)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-048")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-101")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp101TheConflictAndResultSentencesReadAsWritten)
  {
    // ADR-167 and ADR-168: the committed en-US strings of the declare line, a conflict refusal, a claimable area, the
    // result card and a public line.
    const Neuron::StringTable& strings = CommittedStrings();
    const Neuron::MessageLocale locale("en-US");
    using Argument = Neuron::FormatArgument;
    const auto format = [&](std::string_view _id, std::initializer_list<Argument> _arguments)
    { return strings.Format(_id, std::span<const Argument>(_arguments.begin(), _arguments.size()), locale); };
    ExpectText("Raid Red with 2 Leaders for 300, unprovoked: press again to declare",
               format("ui.hud.declare", {Argument{"kind", std::string_view("raid")}, Argument{"target", std::string_view("Red")},
                                         Argument{"area", std::string_view("")}, Argument{"leaders", std::int64_t{2}},
                                         Argument{"cost", std::int64_t{300}}, Argument{"unprovoked", std::string_view("yes")}}),
               L"the declare line");
    ExpectText("That area was claimed too recently",
               format("ui.hud.conflict_refused",
                      {Argument{"kind", std::string_view("claim")}, Argument{"reason", std::string_view("cooling_down")}}),
               L"a claim refused by its cooldown");
    ExpectText("Claim the central site", format("ui.hud.claimable", {Argument{"area", std::string_view("the central site")}}),
               L"a claimable area");
    ExpectText("Victory: Green raided Red",
               format("ui.hud.result_title", {Argument{"outcome", std::string_view("won")}, Argument{"kind", std::string_view("raid")},
                                              Argument{"attacker", std::string_view("Green")},
                                              Argument{"defender", std::string_view("Red")}, Argument{"area", std::string_view("")}}),
               L"a raid won");
    ExpectText("The attackers were routed after 2 min 3 s",
               format("ui.hud.result_end", {Argument{"end", std::string_view("rout")}, Argument{"minutes", std::int64_t{2}},
                                            Argument{"seconds", std::int64_t{3}}}),
               L"its end and length");
    ExpectText("Click to close: 1 more result", format("ui.hud.result_close", {Argument{"more", std::int64_t{1}}}), L"one more waiting");
    ExpectText("Click to close", format("ui.hud.result_close", {Argument{"more", std::int64_t{0}}}), L"none waiting");
    ExpectText("Blue beat Yellow in a raid",
               format("ui.hud.result_public", {Argument{"winner", std::string_view("Blue")}, Argument{"loser", std::string_view("Yellow")},
                                               Argument{"kind", std::string_view("raid")}, Argument{"area", std::string_view("")}}),
               L"another battle's public line");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp034TheBarShowsTheCostTheServerCharges)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-034")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp034TheBarShowsTheCostTheServerCharges)
  {
    const auto& cost = CommittedTuning().mvp.cost;
    ExpectEqual(std::llround(cost.dwellingRu * 1000.0),
                Species::StructureCostMilliRu(CommittedTuning(), Species::StructureKind::Dwelling, 0), L"the first dwelling");
    ExpectEqual(std::llround(cost.dwellingRu * cost.dwellingEscalation * cost.dwellingEscalation * 1000.0),
                Species::StructureCostMilliRu(CommittedTuning(), Species::StructureKind::Dwelling, 2), L"the third, escalated twice");
    ExpectEqual(std::llround(cost.turretRu * 1000.0), Species::StructureCostMilliRu(CommittedTuning(), Species::StructureKind::Turret, 5),
                L"a turret's does not escalate");
  }
};

} // namespace GameClientTests
