#include "pch.h"

#include "CookedRecord.h"
#include "Files.h"
#include "LooseContent.h"
#include "TestSupport.h"
#include "Tuning.h"

#include <algorithm>
#include <cstdint>
#include <span>
#include <string>
#include <utility>
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

Species::TuningMvp SourceRecord(const std::filesystem::path& _contentRoot)
{
  const Neuron::RecordResult<Species::TuningMvp> tuning = Species::ReadTuningMvpSource(_contentRoot);
  Assert::IsTrue(tuning.has_value(), L"the authoring record reads whole");
  return tuning.value_or(Species::TuningMvp{});
}

// A loose-content folder holding the record cooked as the content build cooks it (ADR-108).
std::filesystem::path CookedFolder(const TemporaryFolder& _folder, const Species::TuningMvp& _tuning)
{
  Neuron::CookedEntry cooked;
  cooked.entry.key = std::string(Species::TUNING_MVP_KEY);
  cooked.entry.type = Neuron::ContentType::Record;
  cooked.entry.schemaId = std::string(Species::TUNING_MVP_SCHEMA_ID);
  cooked.entry.schemaVersion = Species::TUNING_MVP_VERSION;
  cooked.entry.simulationRelevant = Species::TUNING_MVP_SIMULATION_RELEVANT;
  cooked.bytes = Species::CookTuningMvp(_tuning);
  std::vector<Neuron::CookedEntry> entries;
  entries.push_back(std::move(cooked));
  const std::filesystem::path folder = _folder.Path() / L"content";
  Assert::IsTrue(Neuron::WriteLooseContent(folder, std::move(entries)).has_value(), L"the loose-content folder is written");
  return folder;
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
    const Species::TuningMvp values = SourceRecord(RepositoryContentRoot());
    // A few values of the MVP scope's table 3 and ADR-079, each under its tuning key.
    Assert::IsTrue(values.mvp.pace.sessionFactor == 24.0, L"mvp.pace.session_factor");
    ExpectEqual(3, values.mvp.world.aiCount, L"mvp.world.ai_count");
    ExpectEqual(100, values.mvp.gather.portTicks, L"mvp.gather.port_ticks");
    Assert::IsTrue(values.mvp.cost.dwellingEscalation == 1.15, L"mvp.cost.dwelling_escalation");
    ExpectEqual(3, values.mvp.persist.snapshotsKept, L"mvp.persist.snapshots_kept");
    Assert::IsTrue(values.ref.sim.tickPeriod == 0.1, L"ref.sim.tick_period");
    ExpectEqual(100'000'000, Species::TickPeriodOf(values).count(), L"10 ticks per second (MVP-092)");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dat050TheCookedRecordReadsAsTheAuthoredRecord)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DAT-050")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dat050TheCookedRecordReadsAsTheAuthoredRecord)
  {
    // DAT-010: the content build rounds and cooks once; the server reads the same bits with no parsing.
    const Species::TuningMvp authored = SourceRecord(RepositoryContentRoot());
    const std::vector<std::byte> cooked = Species::CookTuningMvp(authored);
    const auto read = Species::ReadCookedTuningMvp(cooked);
    Assert::IsTrue(read.has_value(), L"the cooked entry reads");
    const Species::TuningMvp values = read.value_or(Species::TuningMvp{});
    Assert::IsTrue(Species::CookTuningMvp(values) == cooked, L"every field reads back as cooked");
    Assert::IsTrue(values.ref.sim.tickPeriod == authored.ref.sim.tickPeriod, L"ref.sim.tick_period");
    ExpectEqual(authored.mvp.world.aiCount, values.mvp.world.aiCount, L"mvp.world.ai_count");
    Assert::IsTrue(values.wld.travel.valleyPathFactor == authored.wld.travel.valleyPathFactor, L"wld.travel.valley_path_factor");

    // DAT-014 and DAT-017: a reader accepts its own schema at its own version, with its own payload length.
    std::vector<std::byte> other = cooked;
    other[1] = std::byte{'X'};
    Assert::IsFalse(Species::ReadCookedTuningMvp(other).has_value(), L"another schema id is refused");
    const std::vector<std::byte> later =
      Neuron::CookRecordEntry(Species::TUNING_MVP_SCHEMA_ID, {1, 1}, 1, std::span(cooked).subspan(cooked.size() - std::size_t{16}));
    Assert::IsFalse(Species::ReadCookedTuningMvp(later).has_value(), L"another version is refused");
    const std::vector<std::byte> shorter(cooked.begin(), cooked.end() - 8);
    Assert::IsFalse(Species::ReadCookedTuningMvp(shorter).has_value(), L"a payload that ends early is refused");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp099AValueChangedInDataIsTheValueRead)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-099")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp099AValueChangedInDataIsTheValueRead)
  {
    // The authoring file changed, cooked and mounted: the server's load reads the new period, with no rebuild of code.
    const TemporaryFolder folder;
    const Species::TuningMvp changed = SourceRecord(RecordWith(folder, "\"tick_period\": 0.1", "\"tick_period\": 0.2"));
    const auto content = Neuron::LooseContent::Mount(CookedFolder(folder, changed));
    Assert::IsTrue(content.has_value(), L"the folder mounts");
    const auto tuning = Species::LoadTuningMvp(*content);
    Assert::IsTrue(tuning.has_value(), L"the cooked record loads");
    ExpectEqual(200'000'000, Species::TickPeriodOf(tuning.value_or(Species::TuningMvp{})).count(), L"a period of 0.2 s");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ast107AMissingOrDamagedRecordIsRefused)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AST-107")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ast107AMissingOrDamagedRecordIsRefused)
  {
    {
      const TemporaryFolder folder;
      const std::filesystem::path empty = folder.Path() / L"content";
      Assert::IsTrue(Neuron::WriteLooseContent(empty, {}).has_value());
      const auto content = Neuron::LooseContent::Mount(empty);
      Assert::IsTrue(content.has_value(), L"a folder with no entry mounts");
      const auto tuning = Species::LoadTuningMvp(*content);
      Assert::IsTrue(!tuning.has_value() && tuning.error().fault == Neuron::AssetFault::Missing, L"no record is a missing record");
    }
    {
      // The record is simulation-relevant (AST-098), so its hash is checked on every load, in Release too.
      const TemporaryFolder folder;
      const std::filesystem::path cooked = CookedFolder(folder, SourceRecord(RepositoryContentRoot()));
      const std::filesystem::path file = cooked / L"records" / L"tuning" / L"mvp";
      auto bytes = Neuron::ReadWholeFile(file);
      Assert::IsTrue(bytes.has_value() && !bytes->empty());
      bytes->back() ^= std::byte{0x01};
      Assert::IsTrue(Neuron::ReplaceFileAtomically(file, *bytes).has_value());
      const auto content = Neuron::LooseContent::Mount(cooked);
      Assert::IsTrue(content.has_value());
      const auto tuning = Species::LoadTuningMvp(*content);
      Assert::IsTrue(!tuning.has_value() && tuning.error().fault == Neuron::AssetFault::Damaged, L"a changed byte is a damaged record");
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dat024ABrokenRecordIsRefusedWithItsFindings)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DAT-024")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dat024ABrokenRecordIsRefusedWithItsFindings)
  {
    {
      const TemporaryFolder folder;
      const auto tuning = Species::ReadTuningMvpSource(RecordWith(folder, "\"ai_count\": 3", "\"ai_count\": 2.5, \"ai_cuont\": 3"));
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
      const auto tuning = Species::ReadTuningMvpSource(RecordWith(folder, "\"tick_period\": 0.1", "\"tick_period\": 0"));
      Assert::IsTrue(!tuning.has_value() && Has(tuning.error(), Neuron::DiagnosticClass::OutOfRange, "ref.sim.tick_period"),
                     L"a period of 0 is no period");
    }
    {
      const TemporaryFolder folder;
      const auto tuning = Species::ReadTuningMvpSource(folder.Path());
      Assert::IsTrue(!tuning.has_value() && tuning.error().size() == 1 &&
                       tuning.error()[0].diagnosticClass == Neuron::DiagnosticClass::MissingRecord,
                     L"no file is a missing record");
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp081ThePresentationRecordReadsWholeAndCooksAsAuthored)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-081")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp081ThePresentationRecordReadsWholeAndCooksAsAuthored)
  {
    // O75, ADR-116: the client-only record reads whole, holds the look's values and is not simulation-relevant.
    const Neuron::RecordResult<Species::TuningPresentation> source = Species::ReadTuningPresentationSource(RepositoryContentRoot());
    Assert::IsTrue(source.has_value(), L"the authoring record reads whole");
    const Species::TuningPresentation values = source.value_or(Species::TuningPresentation{});
    Assert::IsFalse(Species::TUNING_PRESENTATION_SIMULATION_RELEVANT);
    Assert::AreEqual(15'000.0, values.rnd.view.farPlane);
    Assert::AreEqual(1.0 / 3.0, values.rnd.fog.startFraction, L"the binary64 nearest 1/3 (RND-222)");
    Assert::AreEqual(0.33333, values.ter.color.centerFactor, L"not 1/3 (TER-181)");
    Assert::AreEqual(0.2, values.ter.sea.blurTaps.tap10);
    Assert::AreEqual(1.85, values.ter.sea.wave.zTerms.second.perSecond);
    Assert::AreEqual(1.2, values.mvp.light.key.color.r);

    Neuron::CookedEntry cooked;
    cooked.entry.key = std::string(Species::TUNING_PRESENTATION_KEY);
    cooked.entry.type = Neuron::ContentType::Record;
    cooked.entry.schemaId = std::string(Species::TUNING_PRESENTATION_SCHEMA_ID);
    cooked.entry.schemaVersion = Species::TUNING_PRESENTATION_VERSION;
    cooked.entry.simulationRelevant = Species::TUNING_PRESENTATION_SIMULATION_RELEVANT;
    cooked.bytes = Species::CookTuningPresentation(values);
    std::vector<Neuron::CookedEntry> entries;
    entries.push_back(std::move(cooked));
    const TemporaryFolder folder;
    const std::filesystem::path content = folder.Path() / L"content";
    Assert::IsTrue(Neuron::WriteLooseContent(content, std::move(entries)).has_value());
    const auto mounted = Neuron::LooseContent::Mount(content);
    Assert::IsTrue(mounted.has_value());
    const auto loaded = Species::LoadTuningPresentation(*mounted);
    Assert::IsTrue(loaded.has_value(), L"the cooked record loads");
    Assert::IsTrue(Species::CookTuningPresentation(loaded.value_or(Species::TuningPresentation{})) ==
                     Species::CookTuningPresentation(values),
                   L"the cooked record reads as the authored record (DAT-050)");
  }
};

} // namespace GameSharedTests
