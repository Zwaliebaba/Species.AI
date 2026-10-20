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

namespace
{

template <class Record, auto ReadCooked>
std::expected<Record, Neuron::AssetError> LoadRecord(const Neuron::LooseContent& _content, std::string_view _key)
{
  auto entry = _content.Read(_key);
  if (!entry)
  {
    return std::unexpected(std::move(entry.error()));
  }
  auto record = ReadCooked(*entry);
  if (!record)
  {
    return std::unexpected(Neuron::AssetError{Neuron::AssetFault::Damaged, std::string(_key), std::move(record.error())});
  }
  return *record;
}

template <class Record, auto Read>
Neuron::RecordResult<Record> ReadSource(const std::filesystem::path& _contentRoot, std::string_view _file, std::string_view _what)
{
  const std::string file = std::string(_file);
  const auto bytes = Neuron::ReadWholeFile(_contentRoot / _file);
  if (!bytes)
  {
    Neuron::ContentDiagnostic missing;
    missing.diagnosticClass = Neuron::DiagnosticClass::MissingRecord;
    missing.at.file = file;
    missing.message = std::string(_what) + " cannot be read";
    return std::unexpected(std::vector<Neuron::ContentDiagnostic>{std::move(missing)});
  }
  auto document = Neuron::AuthoringParser{}.Parse(*bytes, file);
  if (!document)
  {
    return std::unexpected(std::vector<Neuron::ContentDiagnostic>{std::move(document.error())});
  }
  return Read(*document);
}

} // namespace

std::expected<TuningMvp, Neuron::AssetError> LoadTuningMvp(const Neuron::LooseContent& _content)
{
  return LoadRecord<TuningMvp, &ReadCookedTuningMvp>(_content, TUNING_MVP_KEY);
}

Neuron::RecordResult<TuningMvp> ReadTuningMvpSource(const std::filesystem::path& _contentRoot)
{
  return ReadSource<TuningMvp, &ReadTuningMvp>(_contentRoot, TUNING_MVP_FILE, "the tuning record");
}

std::expected<TuningPresentation, Neuron::AssetError> LoadTuningPresentation(const Neuron::LooseContent& _content)
{
  return LoadRecord<TuningPresentation, &ReadCookedTuningPresentation>(_content, TUNING_PRESENTATION_KEY);
}

Neuron::RecordResult<TuningPresentation> ReadTuningPresentationSource(const std::filesystem::path& _contentRoot)
{
  return ReadSource<TuningPresentation, &ReadTuningPresentation>(_contentRoot, TUNING_PRESENTATION_FILE, "the presentation record");
}

Neuron::Nanoseconds TickPeriodOf(const TuningMvp& _tuning) noexcept
{
  return Neuron::Nanoseconds{std::llround(_tuning.ref.sim.tickPeriod * 1'000'000'000.0)};
}

} // namespace Species
