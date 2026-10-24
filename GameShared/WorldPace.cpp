#include "pch.h"

#include "WorldPace.h"

namespace Species
{

TuningMvp PacedTuning(const TuningMvp& _tuning, WorldPace _pace)
{
  TuningMvp paced = _tuning;
  // A factor that is not above 0 converts nothing (the record's minimum is 0).
  const double factor = _tuning.mvp.pace.sessionFactor;
  if (_pace == WorldPace::Session || factor <= 0.0)
  {
    return paced;
  }
  // The values of table 3 whose pace is `scaled`, and nothing else.
  TuningMvp::Mvp& mvp = paced.mvp;
  // Durations, multiplied.
  mvp.essence.wellspringIntervalS *= factor;
  mvp.construction.workPerRu *= factor;
  mvp.upkeep.inactiveStepS *= factor;
  mvp.protection.respiteMinutes *= factor;
  mvp.claim.uncontestedMinutes *= factor;
  mvp.claim.cooldownMinutes *= factor;
  mvp.run.lengthHours *= factor;
  mvp.run.quietMinutes *= factor;
  mvp.stats.sampleMinutes *= factor;
  // Amounts per event and per hour, divided.
  mvp.gather.loadRu /= factor;
  mvp.upkeep.workerRuPerHour /= factor;
  mvp.upkeep.structureRuPerHour /= factor;
  mvp.grievance.pressurePerHour /= factor;
  mvp.grievance.fadeSharePerHour /= factor;
  return paced;
}

std::string_view PaceName(WorldPace _pace) noexcept
{
  return _pace == WorldPace::AlwaysOn ? "always_on" : "session";
}

std::optional<WorldPace> PaceNamed(std::string_view _name) noexcept
{
  if (_name == "session")
  {
    return WorldPace::Session;
  }
  if (_name == "always_on")
  {
    return WorldPace::AlwaysOn;
  }
  return std::nullopt;
}

} // namespace Species
