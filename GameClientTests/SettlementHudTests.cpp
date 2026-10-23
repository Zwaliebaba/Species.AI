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
