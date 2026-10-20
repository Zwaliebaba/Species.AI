#include "pch.h"

#include "TestSupport.h"
#include "Tuning.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameSharedTests
{

namespace
{

// The committed record with one value's text replaced, written under a temporary content root.
std::filesystem::path RecordWith(const TemporaryFolder& _folder, std::string_view _from, std::string_view _to)
{
  std::string text = ReadText(RepositoryContentRoot() / Species::TUNING_MVP_FILE);
  const std::size_t at = text.find(_from);
  Assert::IsTrue(at != std::string::npos, L"the committed record holds the text to replace");
  text.replace(at, _from.size(), _to);
  WriteText(_folder.Path() / Species::TUNING_MVP_FILE, text);
  return _folder.Path();
}

bool Has(const std::vector<Neuron::ContentDiagnostic>& _found, Neuron::DiagnosticClass _class, std::string_view _path)
{
  return std::ranges::any_of(_found,
                             [&](const Neuron::ContentDiagnostic& _d) { return _d.diagnosticClass == _class && _d.at.fieldPath == _path; });
}

} // namespace

TEST_CLASS(TuningTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp099TheCommittedRecordReadsWhole)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-099")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp099TheCommittedRecordReadsWhole)
  {
    const Neuron::RecordResult<Species::TuningMvp> tuning = Species::LoadTuningMvp(RepositoryContentRoot());
    Assert::IsTrue(tuning.has_value(), L"GameData/Records/tuning/mvp.json reads with no finding");
    const Species::TuningMvp values = tuning.value_or(Species::TuningMvp{});
    // A few values of the MVP scope's table 3 and ADR-079, each under its tuning key.
    Assert::IsTrue(values.mvp.pace.sessionFactor == 24.0, L"mvp.pace.session_factor");
    ExpectEqual(3, values.mvp.world.aiCount, L"mvp.world.ai_count");
    ExpectEqual(100, values.mvp.gather.portTicks, L"mvp.gather.port_ticks");
    Assert::IsTrue(values.mvp.cost.dwellingEscalation == 1.15, L"mvp.cost.dwelling_escalation");
    ExpectEqual(3, values.mvp.persist.snapshotsKept, L"mvp.persist.snapshots_kept");
    Assert::IsTrue(values.ref.sim.tickPeriod == 0.1, L"ref.sim.tick_period");
    ExpectEqual(100'000'000, Species::TickPeriodOf(values).count(), L"10 ticks per second (MVP-092)");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp099AValueChangedInDataIsTheValueRead)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-099")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp099AValueChangedInDataIsTheValueRead)
  {
    const TemporaryFolder folder;
    const Neuron::RecordResult<Species::TuningMvp> tuning =
      Species::LoadTuningMvp(RecordWith(folder, "\"tick_period\": 0.1", "\"tick_period\": 0.2"));
    Assert::IsTrue(tuning.has_value());
    ExpectEqual(200'000'000, Species::TickPeriodOf(tuning.value_or(Species::TuningMvp{})).count(), L"a period of 0.2 s, with no rebuild");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dat024ABrokenRecordIsRefusedWithItsFindings)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DAT-024")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dat024ABrokenRecordIsRefusedWithItsFindings)
  {
    {
      const TemporaryFolder folder;
      const auto tuning = Species::LoadTuningMvp(RecordWith(folder, "\"ai_count\": 3", "\"ai_count\": 2.5, \"ai_cuont\": 3"));
      Assert::IsFalse(tuning.has_value(), L"refused");
      const std::vector<Neuron::ContentDiagnostic> found = tuning.has_value() ? std::vector<Neuron::ContentDiagnostic>{} : tuning.error();
      Assert::IsTrue(Has(found, Neuron::DiagnosticClass::WrongType, "mvp.world.ai_count"), L"a fraction for a count");
      Assert::IsTrue(Has(found, Neuron::DiagnosticClass::UnknownField, "mvp.world.ai_cuont"), L"a misspelt key");
      Assert::IsTrue(std::ranges::all_of(found, [](const Neuron::ContentDiagnostic& _d)
                                         { return _d.at.file == Species::TUNING_MVP_FILE && _d.at.line > 0; }),
                     L"each finding names the file and its line");
    }
    {
      const TemporaryFolder folder;
      const auto tuning = Species::LoadTuningMvp(RecordWith(folder, "\"tick_period\": 0.1", "\"tick_period\": 0"));
      Assert::IsTrue(!tuning.has_value() && Has(tuning.error(), Neuron::DiagnosticClass::OutOfRange, "ref.sim.tick_period"),
                     L"a period of 0 is no period");
    }
    {
      const TemporaryFolder folder;
      const auto tuning = Species::LoadTuningMvp(folder.Path());
      Assert::IsTrue(!tuning.has_value() && tuning.error().size() == 1 &&
                       tuning.error()[0].diagnosticClass == Neuron::DiagnosticClass::MissingRecord,
                     L"no file is a missing record");
    }
  }
};

} // namespace GameSharedTests
