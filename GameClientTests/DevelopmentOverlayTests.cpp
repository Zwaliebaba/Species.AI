#include "pch.h"

#include "DevelopmentOverlay.h"
#include "TestSupport.h"
#include "Tuning.h"

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameClientTests
{

namespace
{

const Species::TuningPresentation& CommittedPresentation()
{
  static const Species::TuningPresentation PRESENTATION = []
  {
    const auto presentation = Species::ReadTuningPresentationSource(RepositoryContentRoot());
    Assert::IsTrue(presentation.has_value(), L"the committed presentation record reads whole");
    return presentation.value_or(Species::TuningPresentation{});
  }();
  return PRESENTATION;
}

Species::InterpolatedFigure Figure(std::uint8_t _team, Species::FigureKind _kind, bool _dead, const Neuron::Vec3& _position)
{
  Species::InterpolatedFigure figure;
  figure.state.team = _team;
  figure.state.kind = _kind;
  figure.state.dead = _dead;
  figure.position = _position;
  return figure;
}

} // namespace

TEST_CLASS(DevelopmentOverlayTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Dbg095TheFrameRateLineCountsTheObjectsWhileAWorldExists)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-095")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-576")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dbg095TheFrameRateLineCountsTheObjectsWhileAWorldExists)
  {
    Assert::AreEqual(std::string("60 fps"), Species::FrameRateText(60.0, std::nullopt));
    Assert::AreEqual(std::string("59 fps \xC2\xB7 412 objects"), Species::FrameRateText(59.4, std::size_t{412}));
    Assert::AreEqual(std::string("0 fps"), Species::FrameRateText(-1.0, std::nullopt), L"never below zero");
    // The anchor is the record's, in unscaled pixels from the bottom-right corner.
    const Neuron::OverlayLine line = Species::FrameRateLine(CommittedPresentation(), 144.0, std::nullopt);
    Assert::AreEqual(std::string("144 fps"), line.text);
    Assert::AreEqual(10.0f, line.fromRight);
    Assert::AreEqual(40.0f, line.fromBottom);
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RSim415ATeamsColorIsItsBaseColorByItsSlot)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-415")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-416")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RSim415ATeamsColorIsItsBaseColorByItsSlot)
  {
    const std::array<float, 4> green = Species::TeamColor(0);
    Assert::AreEqual(100.0f / 255.0f, green[0], 1e-6f);
    Assert::AreEqual(1.0f, green[1], 1e-6f);
    Assert::AreEqual(1.0f, green[3]);
    const std::array<float, 4> red = Species::TeamColor(1);
    Assert::AreEqual(1.0f, red[0], 1e-6f);
    Assert::AreEqual(50.0f / 255.0f, red[1], 1e-6f);
    const std::array<float, 4> pink = Species::TeamColor(7);
    Assert::AreEqual(150.0f / 255.0f, pink[1], 1e-6f);
    const std::array<float, 4> beyond = Species::TeamColor(8);
    Assert::IsTrue(beyond == std::array<float, 4>{1.0f, 1.0f, 1.0f, 1.0f}, L"white beyond the player colours");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dbg004EveryLivingFigureHasADotAboveItsFeet)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-004")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dbg004EveryLivingFigureHasADotAboveItsFeet)
  {
    const std::vector<Species::InterpolatedFigure> figures{
      Figure(0, Species::FigureKind::Worker, false, {100.0, 5.0, 200.0}),
      Figure(1, Species::FigureKind::Worker, true, {110.0, 5.0, 200.0}),
      Figure(1, Species::FigureKind::Leader, false, {120.0, 6.0, 210.0}),
    };
    const std::vector<Neuron::DevelopmentDot> dots = Species::FigureDots(figures);
    Assert::AreEqual(std::size_t{2}, dots.size(), L"a dead figure has none");
    Assert::AreEqual(5.0 + Species::DOT_LIFT_WU, dots[0].position.y);
    Assert::AreEqual(Species::WORKER_DOT_PIXELS, dots[0].sidePixels);
    Assert::IsTrue(dots[0].color == Species::TeamColor(0));
    Assert::AreEqual(120.0, dots[1].position.x);
    Assert::AreEqual(Species::LEADER_DOT_PIXELS, dots[1].sidePixels, L"a Leader's dot is larger");
    Assert::IsTrue(dots[1].color == Species::TeamColor(1));
  }
};

} // namespace GameClientTests
