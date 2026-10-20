#include "pch.h"

#include "AuthoringParser.h"
#include "Files.h"
#include "Tuning.h"

#include <cmath>
#include <string>
#include <vector>

namespace Species
{

Neuron::RecordResult<TuningMvp> LoadTuningMvp(const std::filesystem::path& _contentRoot)
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
