#include "pch.h"

#include "RegionSimulation.h"
#include "StructureAdvance.h"

#include <algorithm>

namespace Species
{

bool AdvanceStructure(RegionSimulation& _region, Structure& _structure)
{
  const auto& port = _region.Tuning().ref.bldg.port;
  TeamMask everyTeam;
  everyTeam.set();
  for (std::uint32_t index = 0; index < _structure.ports.size(); ++index)
  {
    StructurePort& evaluated = _structure.ports[index];
    evaluated.occupant.reset();
    // ActorGrid::InRange is the strict horizontal test, in result order (DEC-016, DEC-260).
    for (const ActorRef& ref : _region.InRange(evaluated.position.x, evaluated.position.z, port.radiusWu, everyTeam))
    {
      const Figure* worker = _region.Resolve(ref);
      if (worker != nullptr && worker->kind == FigureKind::Worker && !worker->dead && worker->workerState == WorkerState::Manning &&
          worker->portStructureId == _structure.id && worker->portIndex == index)
      {
        evaluated.occupant = ref;
        break;
      }
    }
    if (evaluated.occupant)
    {
      evaluated.reservations.clear();
      continue;
    }
    for (std::uint8_t& count : evaluated.reservations)
    {
      count = static_cast<std::uint8_t>(std::max<std::int64_t>(0, std::int64_t{count} - port.reservationDecayPerTick));
    }
  }
  return false;
}

void OperatePort(StructurePort& _port, std::uint8_t _team, const TuningMvp& _tuning)
{
  if (_port.reservations.size() <= _team)
  {
    _port.reservations.resize(std::size_t{_team} + 1, 0);
  }
  std::uint8_t& count = _port.reservations[_team];
  count = static_cast<std::uint8_t>(std::min<std::int64_t>(std::int64_t{count} + 1, _tuning.ref.bldg.port.reservationCap));
}

bool PortAvailable(const StructurePort& _port, std::uint8_t _team, const TuningMvp& _tuning) noexcept
{
  const std::int64_t count = _team < _port.reservations.size() ? _port.reservations[_team] : 0;
  return !_port.occupant && count < _tuning.ref.bldg.port.reservationLimit;
}

StructurePort* TargetPort(RegionSimulation& _region, const Figure& _worker)
{
  Structure* structure = _region.ResolveStructure(_worker.portStructureSlot, _worker.portStructureId);
  return structure != nullptr && _worker.portIndex < structure->ports.size() ? &structure->ports[_worker.portIndex] : nullptr;
}

void SendToPort(Figure& _worker, const Structure& _structure, std::uint32_t _structureSlot, std::uint32_t _port)
{
  _worker.portStructureId = _structure.id;
  _worker.portStructureSlot = _structureSlot;
  _worker.portIndex = _port;
  _worker.waypoint = _structure.ports[_port].position;
  _worker.workerState = WorkerState::ToPort;
}

} // namespace Species
