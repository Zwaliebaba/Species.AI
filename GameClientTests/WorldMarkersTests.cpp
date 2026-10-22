#include "pch.h"

#include "CrowdLook.h"
#include "DevelopmentOverlay.h"
#include "FrameView.h"
#include "Geometry.h"
#include "StableId.h"
#include "TestSupport.h"
#include "WorldMarkers.h"

#include <cmath>
#include <numbers>
#include <optional>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameClientTests
{

namespace
{

constexpr std::uint8_t OWN = 0;
constexpr Neuron::SizePx TARGET{1920, 1080};
constexpr Neuron::Vec3 CAMERA{5'000.0, 300.0, 5'000.0};
constexpr Neuron::Vec3 WORLD_UP{0.0, 1.0, 0.0};

Species::StableId IdOf(std::uint64_t _counter)
{
  return Species::MakeStableId(Species::REGION_OWNER_ID_FIRST, _counter);
}

// A level camera at CAMERA, its yaw and pitch 0, and the view of _step seconds with the pointer at _pointer.
Species::MarkerView ViewOf(double _step, Neuron::PointPx _pointer = {})
{
  Neuron::CameraPose pose;
  pose.position = CAMERA;
  Species::MarkerView view;
  view.frame = Neuron::MakeFrameView(pose, static_cast<double>(TARGET.width) / static_cast<double>(TARGET.height), {});
  view.camera = CAMERA;
  view.cameraFront = Neuron::ViewDirection(pose);
  view.size = TARGET;
  view.pointer = _pointer;
  view.realStepSeconds = _step;
  return view;
}

Neuron::Vec3 Front()
{
  return ViewOf(0.0).cameraFront;
}

Neuron::Vec3 Right()
{
  return Neuron::Normalized(Neuron::Cross(Front(), WORLD_UP));
}

Species::InterpolatedFigure LeaderAt(std::uint64_t _counter, const Neuron::Vec3& _at, std::uint8_t _team = OWN)
{
  Species::InterpolatedFigure figure;
  figure.id = IdOf(_counter);
  figure.state.team = _team;
  figure.state.kind = Species::FigureKind::Leader;
  figure.position = _at;
  return figure;
}

// Behind the camera and to its right or left, beyond the near distance of R-CTRL-385.
Neuron::Vec3 BehindRight()
{
  return CAMERA - Front() * 600.0 + Right() * 900.0;
}

Neuron::Vec3 BehindLeft()
{
  return CAMERA - Front() * 1'200.0 - Right() * 900.0;
}

Neuron::Vec3 Ahead()
{
  return CAMERA + Front() * 300.0 - WORLD_UP * 40.0;
}

} // namespace

TEST_CLASS(WorldMarkersTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(RCtrl381EachOwnLivingLeaderHasAMarker)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-381")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-388")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RCtrl381EachOwnLivingLeaderHasAMarker)
  {
    Species::WorldMarkers markers(CommittedPresentation());
    Species::InterpolatedFigure dead = LeaderAt(4, Ahead());
    dead.state.dead = true;
    Species::InterpolatedFigure worker = LeaderAt(5, Ahead());
    worker.state.kind = Species::FigureKind::Worker;
    const std::vector<Species::InterpolatedFigure> figures{LeaderAt(1, Ahead()), LeaderAt(2, BehindRight()), LeaderAt(3, Ahead(), 1), dead,
                                                           worker};
    markers.Update(figures, OWN, std::nullopt, ViewOf(0.1));
    ExpectEqual(2, static_cast<std::int64_t>(markers.Count()), L"the two living Leaders of the local team");
    // On screen the banner of pass 7b is the marker's icon, so pass 16 draws only the one off screen.
    ExpectEqual(5, static_cast<std::int64_t>(markers.Quads().size()), L"one marker's five layers");
    markers.Update(std::vector{LeaderAt(1, Ahead())}, OWN, std::nullopt, ViewOf(0.1));
    ExpectEqual(1, static_cast<std::int64_t>(markers.Count()), L"a marker leaves with its Leader");
    markers.Update(std::vector{LeaderAt(1, Ahead())}, std::nullopt, std::nullopt, ViewOf(0.1));
    ExpectEqual(0, static_cast<std::int64_t>(markers.Count()), L"none without a team");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RCtrl383AnOffScreenMarkerIsHeldAtTheEdge)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-383")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-385")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-386")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-387")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-357")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RCtrl383AnOffScreenMarkerIsHeldAtTheEdge)
  {
    Species::WorldMarkers markers(CommittedPresentation());
    markers.Update(std::vector{LeaderAt(2, BehindRight())}, OWN, std::nullopt, ViewOf(0.1));
    const std::vector<Neuron::OverlayQuad> quads = markers.Quads();
    Assert::IsTrue(quads.size() == 5, L"five layers");
    const std::array<Species::SceneTexture, 5> layers{Species::SceneTexture::TeamDiscShadow, Species::SceneTexture::EdgeCompassShadow,
                                                      Species::SceneTexture::EdgeCompass, Species::SceneTexture::TeamDisc,
                                                      Species::SceneTexture::IconLeader};
    for (std::size_t n = 0; n < layers.size(); ++n)
    {
      Assert::IsTrue(quads[n].texture == static_cast<std::uint16_t>(layers[n]), L"in R-CTRL-386's order");
      Assert::IsTrue((quads[n].blend == Neuron::OverlayBlend::Alpha) == (n < 2), L"the shadows blended, the rest added (RND-357)");
    }
    // Inset 50 px from the right edge, the compass turned a quarter clockwise to point right, towards the Leader.
    const Neuron::OverlayQuad& icon = quads[4];
    Assert::AreEqual(1'920.0f - 50.0f, icon.centerX, 1.0f, L"held 50 px in from the right edge");
    Assert::AreEqual(static_cast<float>(std::numbers::pi / 2.0), quads[2].rotationRadians, 0.05f, L"the compass points right");
    Assert::AreEqual(1'080.0f / 20.0f / 2.0f, icon.halfSize, 1e-3f, L"the icon: the height ÷ 20");
    Assert::AreEqual((1'080.0f / 20.0f + 10.0f) / 2.0f, quads[0].halfSize, 1e-3f, L"the shadow: the icon + 10 px");
    Assert::AreEqual((1'080.0f / 20.0f + 1'080.0f / 30.0f) / 2.0f, quads[2].halfSize, 1e-3f, L"the compass: the icon + height ÷ 30");
    Assert::IsFalse(markers.UnderPointer().has_value(), L"the pointer is elsewhere");
    // R-CTRL-387: the pointer over its icon makes it the marker under the pointer, drawn × 1.2.
    const Neuron::PointPx over{static_cast<std::int32_t>(icon.centerX), static_cast<std::int32_t>(icon.centerY)};
    markers.Update(std::vector{LeaderAt(2, BehindRight())}, OWN, std::nullopt, ViewOf(0.1, over));
    Assert::IsTrue(markers.UnderPointer() == IdOf(2), L"its Leader is under the pointer");
    Assert::AreEqual(icon.halfSize * 1.2f, markers.Quads()[4].halfSize, 1e-3f, L"hovered: × ref.ctrl.marker.size.hover_scale");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RCtrl384ASeenLeaderMarkerNoLongerShowsOffScreen)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-384")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RCtrl384ASeenLeaderMarkerNoLongerShowsOffScreen)
  {
    Species::WorldMarkers markers(CommittedPresentation());
    markers.Update(std::vector{LeaderAt(2, Ahead())}, OWN, std::nullopt, ViewOf(0.6));
    markers.Update(std::vector{LeaderAt(2, BehindRight())}, OWN, std::nullopt, ViewOf(0.6));
    ExpectEqual(5, static_cast<std::int64_t>(markers.Quads().size()), L"0.6 s on screen: not yet seen, so shown off screen");
    markers.Update(std::vector{LeaderAt(2, Ahead())}, OWN, std::nullopt, ViewOf(0.6));
    markers.Update(std::vector{LeaderAt(2, BehindRight())}, OWN, std::nullopt, ViewOf(0.6));
    Assert::IsTrue(markers.Quads().empty(), L"1.2 s on screen: seen, and no longer shown off screen");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RCtrl382TheNearestOldMarkerIsDrawnLast)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-382")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RCtrl382TheNearestOldMarkerIsDrawnLast)
  {
    Species::WorldMarkers markers(CommittedPresentation());
    // The nearer marker is made first; once both are older than ref.ctrl.marker.new_age, the farther is drawn first.
    const std::vector<Species::InterpolatedFigure> figures{LeaderAt(2, BehindRight()), LeaderAt(3, BehindLeft())};
    for (int frame = 0; frame < 5; ++frame)
    {
      markers.Update(figures, OWN, std::nullopt, ViewOf(1.0));
    }
    const std::vector<Neuron::OverlayQuad>& quads = markers.Quads();
    Assert::IsTrue(quads.size() == 10, L"both off screen");
    Assert::IsTrue(quads[4].centerX < 960.0f && quads[9].centerX > 960.0f, L"the farther, on the left, first; the nearer last");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RCtrl366TheBadgeShowsTheSelection)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-366")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RCtrl366TheBadgeShowsTheSelection)
  {
    Assert::IsTrue(Species::CursorBadge(false, false, OWN, {100, 100}, TARGET, CommittedPresentation()).empty(), L"none unselected");
    const std::vector<Neuron::OverlayQuad> leader = Species::CursorBadge(true, true, OWN, {100, 100}, TARGET, CommittedPresentation());
    Assert::IsTrue(leader.size() == 3 && leader[0].texture == static_cast<std::uint16_t>(Species::SceneTexture::TeamDiscShadow) &&
                     leader[1].texture == static_cast<std::uint16_t>(Species::SceneTexture::TeamDisc) &&
                     leader[2].texture == static_cast<std::uint16_t>(Species::SceneTexture::IconLeader),
                   L"a selected Leader first: its icon on the team's disc over its shadow");
    // The icon's centre 5 + 50 px along Normalize(0.85, 1), its size the height ÷ 24.
    const double along = 55.0 / std::hypot(0.85, 1.0);
    Assert::AreEqual(static_cast<float>(100.0 + 0.85 * along), leader[2].centerX, 1e-3f, L"down and to the right");
    Assert::AreEqual(static_cast<float>(100.0 + along), leader[2].centerY, 1e-3f);
    Assert::AreEqual(1'080.0f / 24.0f / 2.0f, leader[2].halfSize, 1e-3f, L"ref.ctrl.cursor.icon_divisor");
    Assert::IsTrue(leader[1].color == Species::TeamColor(OWN), L"the team's colour");
    const std::vector<Neuron::OverlayQuad> workers = Species::CursorBadge(false, true, OWN, {100, 100}, TARGET, CommittedPresentation());
    Assert::IsTrue(workers.size() == 3 && workers[2].texture == static_cast<std::uint16_t>(Species::SceneTexture::IconWorkers),
                   L"selected Workers: icon.power.workers");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RCtrl366TheBadgeCountsTheSelectedWorkers)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-366")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-100")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RCtrl366TheBadgeCountsTheSelectedWorkers)
  {
    // ADR-127, ADR-145: the count's line starts at the right edge of the badge's shadow, its middle level with the badge's
    // centre, in physical pixels; none with no badge or no Worker.
    const std::vector<Neuron::OverlayQuad> badge = Species::CursorBadge(false, true, OWN, {100, 100}, TARGET, CommittedPresentation());
    const std::optional<Species::BadgeLabel> count = Species::BadgeCount(badge, 18);
    Assert::IsTrue(count.has_value(), L"a count");
    const Species::BadgeLabel label = count.value_or(Species::BadgeLabel{});
    Assert::IsTrue(label.text == "18", L"18");
    Assert::AreEqual(badge[0].centerX + badge[0].halfSize, label.left, 1e-4f, L"at the shadow's right edge");
    Assert::AreEqual(badge[0].centerY, label.middle, 1e-4f, L"level with the badge");
    Assert::IsFalse(Species::BadgeCount(badge, 0).has_value(), L"none for no Worker");
    Assert::IsFalse(Species::BadgeCount({}, 5).has_value(), L"none without a badge");
  }
};

} // namespace GameClientTests
