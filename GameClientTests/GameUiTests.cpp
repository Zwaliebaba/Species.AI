#include "pch.h"

#include "GameUi.h"
#include "InputBindings.h"
#include "TestSupport.h"
#include "UiLook.h"

#include <chrono>
#include <cstdint>
#include <set>
#include <utility>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameClientTests
{

namespace
{

// A back-end whose key presses the test queues.
class KeysBackEnd final : public Neuron::DeviceBackEnd
{
public:
  void Press(Neuron::KeyCode _key, bool _press = true)
  {
    Neuron::InputEvent event;
    event.kind = _press ? Neuron::InputEvent::Kind::Press : Neuron::InputEvent::Kind::Release;
    event.control = static_cast<std::uint16_t>(_key);
    m_pending.push_back(event);
  }
  void Collect(std::vector<Neuron::InputEvent>& _queue) override
  {
    _queue.insert(_queue.end(), m_pending.begin(), m_pending.end());
    m_pending.clear();
  }
  void ReleaseAll(std::vector<Neuron::InputEvent>& /*_queue*/) override {}

private:
  std::vector<Neuron::InputEvent> m_pending;
};

constexpr Neuron::SizePx CLIENT{1280, 720};

} // namespace

TEST_CLASS(GameUiTests)
{
public:
#if ENGINE_DEVELOPMENT
  BEGIN_TEST_METHOD_ATTRIBUTE(Ui006AFocusedWindowWithholdsTheWorldsActions)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-006")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui006AFocusedWindowWithholdsTheWorldsActions)
  {
    KeysBackEnd keys;
    const std::vector<Neuron::ActionBindings> bindings = Species::DefaultBindings();
    Neuron::InputRouter router(bindings, keys, nullptr);
    Species::GameUi ui(CommittedPresentation());
    keys.Press(Neuron::KeyCode::Left);
    router.AdvanceFrame({}, CLIENT, std::chrono::seconds(1));
    ui.Update(router, CLIENT, 1.0f, true);
    Assert::IsTrue(router.Query(Species::ActionOf(Species::GameAction::CameraPanLeft)), L"without a window the arrow pans");

    ui.OpenTestScreen();
    router.AdvanceFrame({}, CLIENT, std::chrono::seconds(2));
    ui.Update(router, CLIENT, 1.0f, true);
    Assert::IsTrue(ui.Core().Focused() != nullptr, L"the test screen has the focus");
    Assert::IsFalse(router.Query(Species::ActionOf(Species::GameAction::CameraPanLeft)), L"and the held arrow no longer pans");
    Assert::IsTrue(Neuron::ValidateWindow(*ui.Core().Focused()).empty(), L"UI-372: the test page passes the validator");

    // INP-133: the arrow pressed again moves the cursor once, and its repeat once the delay has passed.
    const Neuron::UiHandle first = ui.Core().Focused()->Cursor();
    keys.Press(Neuron::KeyCode::Down);
    router.AdvanceFrame({}, CLIENT, std::chrono::seconds(3));
    ui.Update(router, CLIENT, 1.0f, true);
    const Neuron::UiHandle second = ui.Core().Focused()->Cursor();
    Assert::IsTrue(second != first, L"down moved the cursor");
    router.AdvanceFrame({}, CLIENT, std::chrono::milliseconds(3100));
    ui.Update(router, CLIENT, 1.0f, true);
    Assert::IsTrue(ui.Core().Focused()->Cursor() == second, L"not again before the initial delay");
    router.AdvanceFrame({}, CLIENT, std::chrono::milliseconds(3350));
    ui.Update(router, CLIENT, 1.0f, true);
    Assert::IsTrue(ui.Core().Focused()->Cursor() != second, L"the repeat moved it on");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ui115TheTestScreenDrawsEveryPipelineOfTheUiPass)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-115")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-184")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui115TheTestScreenDrawsEveryPipelineOfTheUiPass)
  {
    KeysBackEnd keys;
    const std::vector<Neuron::ActionBindings> bindings = Species::DefaultBindings();
    Neuron::InputRouter router(bindings, keys, nullptr);
    Species::GameUi ui(CommittedPresentation());
    ui.OpenTestScreen();
    router.AdvanceFrame({}, CLIENT);
    ui.Update(router, CLIENT, 1.0f, true);
    Neuron::TextSystem text(Species::TextSystemDesc(CommittedPresentation()));
    std::vector<Neuron::UiBatch> batches;
    ui.Draw(text, 1.0, batches);
    std::set<std::pair<Neuron::UiSource, Neuron::UiBlend>> drawn;
    bool gradient = false;
    for (const Neuron::UiBatch& batch : batches)
    {
      drawn.emplace(batch.source, batch.blend);
      for (const Neuron::UiQuad& quad : batch.quads)
      {
        gradient = gradient || quad.colorTo.has_value();
      }
    }
    Assert::IsTrue(drawn.contains({Neuron::UiSource::Solid, Neuron::UiBlend::Ink}), L"UI-115: the fill boxes' darkening halo");
    Assert::IsTrue(drawn.contains({Neuron::UiSource::Image, Neuron::UiBlend::Additive}), L"UI-115: their drifting glow");
    Assert::IsTrue(drawn.contains({Neuron::UiSource::Image, Neuron::UiBlend::Alpha}), L"UI-114, UI-115: the beam and the blurred bodies");
    Assert::IsTrue(drawn.contains({Neuron::UiSource::Solid, Neuron::UiBlend::Alpha}), L"UI-108: the cursor's focus band");
    Assert::IsTrue(gradient, L"UI-184: the beam and the halo are gradients");
    Assert::IsTrue(ui.TestSampleArea().has_value(), L"the samples' box while the root page shows");
  }
#endif
};

} // namespace GameClientTests
