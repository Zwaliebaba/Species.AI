#include "pch.h"

#include "PassTable.h"
#include "TestSupport.h"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronClientTests
{

namespace
{

using Neuron::TargetAccess;

constexpr std::uint32_t SCENE_COLOR = 0;
constexpr std::uint32_t SCENE_DEPTH = 1;
constexpr std::uint32_t BACK_BUFFER = 2;
constexpr std::uint32_t RAMP = 3;

const std::array<Neuron::TargetDecl, 4> TARGETS{{
  {"scene_color", TargetAccess::ResolveSource, TargetAccess::ResolveSource},
  {"scene_depth", TargetAccess::DepthWrite, TargetAccess::DepthWrite},
  {"back_buffer", TargetAccess::Present, TargetAccess::Present},
  {"terrain_ramp", TargetAccess::ShaderRead, TargetAccess::ShaderRead},
}};

} // namespace

TEST_CLASS(FrameGraphTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Adr004TheBarriersFollowTheDeclaredUses)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-080")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr004TheBarriersFollowTheDeclaredUses)
  {
    // The world frame of M1 task T13: draw into the scene targets, resolve into the back buffer, present.
    const std::vector<Neuron::PassDecl> passes{
      {"world", {{SCENE_COLOR, TargetAccess::RenderTarget}, {SCENE_DEPTH, TargetAccess::DepthWrite}, {RAMP, TargetAccess::ShaderRead}}},
      {"resolve", {{SCENE_COLOR, TargetAccess::ResolveSource}, {BACK_BUFFER, TargetAccess::ResolveDestination}}},
    };
    const auto plan = Neuron::PlanBarriers(TARGETS, passes);
    Assert::IsTrue(plan.has_value(), L"a valid table");
    const Neuron::BarrierPlan barriers = plan.value_or(Neuron::BarrierPlan{});
    ExpectEqual(1, static_cast<std::int64_t>(barriers.before[0].size()), L"the scene colour becomes a target; depth and the ramp stay");
    Assert::IsTrue(barriers.before[0][0].target == SCENE_COLOR && barriers.before[0][0].after == TargetAccess::RenderTarget, L"to render");
    ExpectEqual(2, static_cast<std::int64_t>(barriers.before[1].size()), L"both sides of the resolve");
    Assert::IsTrue(barriers.before[1][1].before == TargetAccess::Present && barriers.before[1][1].after == TargetAccess::ResolveDestination,
                   L"the back buffer from present");
    ExpectEqual(1, static_cast<std::int64_t>(barriers.after.size()), L"the back buffer back to present at the end");
    Assert::IsTrue(barriers.after[0].target == BACK_BUFFER && barriers.after[0].after == TargetAccess::Present, L"presented");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr004AnInvalidTableIsRefusedWithItsFault)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-080")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr004AnInvalidTableIsRefusedWithItsFault)
  {
    const auto fault = [](const std::vector<Neuron::PassDecl>& _passes)
    {
      const auto plan = Neuron::PlanBarriers(TARGETS, _passes);
      return plan.has_value() ? std::optional<Neuron::PassTableFault>{} : std::optional<Neuron::PassTableFault>{plan.error().fault};
    };
    Assert::IsTrue(fault({{"bad", {{9, TargetAccess::ShaderRead}}}}) == Neuron::PassTableFault::UnknownTarget, L"an unknown target");
    Assert::IsTrue(fault({{"bad", {{SCENE_COLOR, TargetAccess::RenderTarget}, {SCENE_COLOR, TargetAccess::ShaderRead}}}}) ==
                     Neuron::PassTableFault::ConflictingUses,
                   L"two uses of one target in one pass");
    const std::array<Neuron::TargetDecl, 2> fresh{
      {{"blur", TargetAccess::RenderTarget, TargetAccess::RenderTarget}, {"out", TargetAccess::ShaderRead, TargetAccess::ShaderRead}}};
    const auto readFirst = Neuron::PlanBarriers(fresh, std::vector<Neuron::PassDecl>{{"read", {{0, TargetAccess::ShaderRead}}}});
    Assert::IsTrue(!readFirst.has_value() && readFirst.error().fault == Neuron::PassTableFault::ReadBeforeWrite,
                   L"read before it is written");
    const std::array<Neuron::TargetDecl, 1> lost{{{"lost", TargetAccess::ShaderRead, TargetAccess::ShaderRead}}};
    const auto unread = Neuron::PlanBarriers(lost, std::vector<Neuron::PassDecl>{{"write", {{0, TargetAccess::RenderTarget}}}});
    Assert::IsTrue(!unread.has_value() && unread.error().fault == Neuron::PassTableFault::WrittenNeverRead, L"written and never read");
  }
};

} // namespace NeuronClientTests
