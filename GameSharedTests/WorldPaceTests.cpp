#include "pch.h"

#include "TestSupport.h"
#include "Tuning.h"
#include "WorldPace.h"

#include <cstddef>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameSharedTests
{

namespace
{

Species::TuningMvp CommittedRecord()
{
  const Neuron::RecordResult<Species::TuningMvp> tuning = Species::ReadTuningMvpSource(RepositoryContentRoot());
  Assert::IsTrue(tuning.has_value(), L"the authoring record reads whole");
  return tuning.value_or(Species::TuningMvp{});
}

// The values of the MVP scope's table 3 whose pace is `scaled`, read or written as one array, in the table's order.
constexpr std::size_t SCALED_VALUES = 14;
std::vector<double*> ScaledValues(Species::TuningMvp& _tuning)
{
  Species::TuningMvp::Mvp& mvp = _tuning.mvp;
  return {&mvp.essence.wellspringIntervalS,
          &mvp.gather.loadRu,
          &mvp.construction.workPerRu,
          &mvp.upkeep.workerRuPerHour,
          &mvp.upkeep.structureRuPerHour,
          &mvp.upkeep.inactiveStepS,
          &mvp.protection.respiteMinutes,
          &mvp.grievance.pressurePerHour,
          &mvp.grievance.fadeSharePerHour,
          &mvp.claim.uncontestedMinutes,
          &mvp.claim.cooldownMinutes,
          &mvp.run.lengthHours,
          &mvp.run.quietMinutes,
          &mvp.stats.sampleMinutes};
}

// Whether each value is a duration (multiplied) or an amount per event or per hour (divided), in the same order.
constexpr bool DURATION[SCALED_VALUES]{true, false, true, false, false, true, true, false, false, true, true, true, true, true};

} // namespace

TEST_CLASS(WorldPaceTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Pil101TheScaledValuesConvertByTheSessionFactorAndNoOtherDoes)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-101")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-103")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-071")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Pil101TheScaledValuesConvertByTheSessionFactorAndNoOtherDoes)
  {
    Species::TuningMvp session = CommittedRecord();
    Assert::IsTrue(session.mvp.pace.sessionFactor == 24.0, L"mvp.pace.session_factor");
    Species::TuningMvp alwaysOn = Species::PacedTuning(session, Species::WorldPace::AlwaysOn);
    const std::vector<double*> from = ScaledValues(session);
    const std::vector<double*> to = ScaledValues(alwaysOn);
    Assert::IsTrue(from.size() == SCALED_VALUES && to.size() == SCALED_VALUES, L"the fourteen scaled values");
    for (std::size_t index = 0; index < SCALED_VALUES; ++index)
    {
      const double expected = DURATION[index] ? *from[index] * 24.0 : *from[index] / 24.0;
      Assert::IsTrue(*to[index] == expected, L"each scaled value is multiplied or divided by the session factor");
    }
    // With the scaled values put back, the always-on record cooks to the session record's bytes: nothing else changed.
    for (std::size_t index = 0; index < SCALED_VALUES; ++index)
    {
      *to[index] = *from[index];
    }
    Assert::IsTrue(Species::CookTuningMvp(alwaysOn) == Species::CookTuningMvp(session), L"no other value converts");
    Assert::IsTrue(Species::CookTuningMvp(Species::PacedTuning(session, Species::WorldPace::Session)) == Species::CookTuningMvp(session),
                   L"the session pace reads the record unchanged");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Pil104APaceIsNamedInAWorldsDescription)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-104")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Pil104APaceIsNamedInAWorldsDescription)
  {
    for (const Species::WorldPace pace : {Species::WorldPace::Session, Species::WorldPace::AlwaysOn})
    {
      Assert::IsTrue(Species::PaceNamed(Species::PaceName(pace)) == pace, L"each pace reads back from its name");
    }
    Assert::IsTrue(Species::PaceName(Species::WorldPace::AlwaysOn) == "always_on", L"the always-on pace's name");
    Assert::IsFalse(Species::PaceNamed("always-on").has_value(), L"a name that is no pace's");
  }
};

} // namespace GameSharedTests
