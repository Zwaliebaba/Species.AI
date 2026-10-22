#pragma once

#include "Development.h"
#include "InputRouter.h"
#include "TextSystem.h"
#include "TuningPresentation.h"
#include "UiCore.h"
#include "UiLayout.h"
#include "WindowGeometry.h"
#include "WorldScene.h"

#include <optional>
#include <vector>

namespace Species
{

// The client's pages (ADR-149). M2's front end adds its own (T18).
enum class GamePage : Neuron::UiPageId
{
  None = 0,
#if ENGINE_DEVELOPMENT
  TestRoot,   // `--ui-test`: the test screen's root page
  TestSecond, // and the page it pushes
#endif
};

/// The client's UI (ADR-030 to ADR-033 at M2, ADR-149): the UI core with the game's pages, the page family's look from the
/// presentation record, and its input from the router's menu and pointer actions. It is updated before the camera and
/// the verbs, so that the actions a focused window takes reach no world action (UI-006). Main thread only.
class GameUi
{
public:
  explicit GameUi(const TuningPresentation& _presentation);

  GameUi(const GameUi&) = delete;
  GameUi& operator=(const GameUi&) = delete;

#if ENGINE_DEVELOPMENT
  // A Debug build's `--ui-test` (ADR-145, ADR-149): the test screen, from the next update.
  void OpenTestScreen();
#endif
  // The frame's update (UI-352) from _router's actions, laid out for a client area of _client at _dpiScale; while a
  // window has the focus, the world's camera and verb actions are suppressed for the rest of the frame.
  void Update(Neuron::InputRouter& _router, Neuron::SizePx _client, float _dpiScale, bool _windowFocused);
  void Draw(Neuron::TextSystem& _text, double _seconds, std::vector<Neuron::UiBatch>& _out) const;
  // UI-168: interface.ui_scale and interface.text_scale from the settings store; none takes the record's default. The
  // next update lays the windows out again (UI-172).
  void SetScales(std::optional<double> _uiScale, std::optional<double> _textScale);

  [[nodiscard]] const Neuron::UiCore& Core() const noexcept
  {
    return m_core;
  }
  // The test page's right box, where the test samples are drawn; none while the test screen is not shown.
  [[nodiscard]] std::optional<Neuron::UiRect> TestSampleArea() const;

private:
  const TuningPresentation& m_presentation;
  Neuron::UiStyle m_style;
  Neuron::UiCore m_core;
  Neuron::UiMetrics m_metrics;
  std::optional<double> m_uiScale;
  std::optional<double> m_textScale;
  bool m_pointerHeld = false;
};

} // namespace Species
