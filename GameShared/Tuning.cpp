#include "pch.h"

#include "AuthoringParser.h"
#include "Files.h"
#include "Tuning.h"

#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace Species
{

std::expected<TuningMvp, Neuron::AssetError> LoadTuningMvp(const Neuron::LooseContent& _content)
{
  auto entry = _content.Read(TUNING_MVP_KEY);
  if (!entry)
  {
    return std::unexpected(std::move(entry.error()));
  }
  auto tuning = ReadCookedTuningMvp(*entry);
  if (!tuning)
  {
    return std::unexpected(Neuron::AssetError{Neuron::AssetFault::Damaged, std::string(TUNING_MVP_KEY), std::move(tuning.error())});
  }
  return *tuning;
}

Neuron::RecordResult<TuningMvp> ReadTuningMvpSource(const std::filesystem::path& _contentRoot)
{
  const std::string file = std::string(TUNING_MVP_FILE);
  const auto bytes = Neuron::ReadWholeFile(_contentRoot / TUNING_MVP_FILE);
  if (!bytes)
  {
    Neuron::ContentDiagnostic missing;
    missing.diagnosticClass = Neuron::DiagnosticClass::MissingRecord;
    missing.at.file = file;
    missing.message = "the tuning record cannot be read";
    return std::unexpected(std::vector<Neuron::ContentDiagnostic>{std::move(missing)});
  }
  auto document = Neuron::AuthoringParser{}.Parse(*bytes, file);
  if (!document)
  {
    return std::unexpected(std::vector<Neuron::ContentDiagnostic>{std::move(document.error())});
  }
  return ReadTuningMvp(*document);
}

Neuron::Nanoseconds TickPeriodOf(const TuningMvp& _tuning) noexcept
{
  return Neuron::Nanoseconds{std::llround(_tuning.ref.sim.tickPeriod * 1'000'000'000.0)};
}

} // namespace Species
