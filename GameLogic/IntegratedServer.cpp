#include "pch.h"

#include "Check.h"
#include "InProcessTransport.h"
#include "IntegratedServer.h"
#include "ServerHost.h"
#include "StructureRecords.h"
#include "Tuning.h"
#include "WorldServer.h"

#include <utility>

namespace Species
{

namespace
{

// ADR-155: the steward's reason, from the newest snapshot refused.
ContinueFault ContinueFaultOf(const WorldServer::OpenOutcome& _opened) noexcept
{
  if (_opened.refusals.empty())
  {
    return ContinueFault::NoSnapshot;
  }
  const std::optional<Neuron::SnapshotError> error = _opened.refusals.front().error;
  if (!error.has_value())
  {
    return ContinueFault::Damaged;
  }
  switch (*error)
  {
  case Neuron::SnapshotError::Unreadable:
    return ContinueFault::Unreadable;
  case Neuron::SnapshotError::FormatVersion:
  case Neuron::SnapshotError::StateVersion:
    return ContinueFault::Version;
  case Neuron::SnapshotError::NotASnapshot:
  case Neuron::SnapshotError::Truncated:
  case Neuron::SnapshotError::Damaged:
  case Neuron::SnapshotError::MalformedState:
    break;
  }
  return ContinueFault::Damaged;
}

} // namespace

// In the order they are made and the reverse of the order they end: the listener and the world outlive the host.
struct IntegratedServer::Parts
{
  Neuron::InProcessListener listener{Neuron::InProcessTransport::Desc{}};
  std::unique_ptr<WorldServer> world;
  std::unique_ptr<Neuron::ServerHost> host;
};

IntegratedServer::IntegratedServer(const Desc& _desc)
  : m_desc(_desc),
    m_parts(std::make_unique<Parts>())
{
  ENGINE_CHECK(m_desc.clock != nullptr);
}

IntegratedServer::~IntegratedServer() = default;

std::expected<void, StartFault> IntegratedServer::Start()
{
  ENGINE_CHECK(m_parts->host == nullptr, "the integrated server starts once");
  ENGINE_CHECK(m_desc.content != nullptr, "the integrated server loads its tuning from mounted content");
  // MVP-099: the tuning values come from data when the server starts, never from code; the content build cooked them
  // (ADR-108).
  std::expected<TuningMvp, Neuron::AssetError> tuning = LoadTuningMvp(*m_desc.content);
  if (!tuning)
  {
    return std::unexpected(std::move(tuning.error()));
  }
  // ADR-129: the structures' ports and spheres come from their meshes' simulation data records, never from a mesh.
  std::expected<StructureRecords, Neuron::AssetError> structures = LoadStructureRecords(*m_desc.content);
  if (!structures)
  {
    return std::unexpected(std::move(structures.error()));
  }
  if (std::optional<Neuron::AssetError> fault = CheckStructureRecords(*structures, *tuning))
  {
    return std::unexpected(std::move(*fault));
  }
  WorldServer::Desc world;
  world.tuning = *tuning;
  world.structures = std::move(*structures);
  world.worldSeed = m_desc.worldSeed;
  world.sandbox = m_desc.sandbox;
  world.watched = m_desc.watched;
  world.syntheticFigures = m_desc.syntheticFigures;
  world.stewardSite = m_desc.stewardSite;
  world.aiSettlements = m_desc.aiSettlements;
  world.snapshotFolder = m_desc.snapshotFolder;
  world.restoreOnly = m_desc.restoreOnly;
  world.calendar = [clock = m_desc.clock] { return clock->CalendarUtc(); }; // ADR-137: the world record's wall times
  m_parts->world = std::make_unique<WorldServer>(world);
  Neuron::ServerHost::Desc host;
  host.clock = m_desc.clock;
  host.tickPeriod = TickPeriodOf(*tuning);
  host.workers = m_desc.workers;
  host.listener = &m_parts->listener;
  m_parts->host = std::make_unique<Neuron::ServerHost>(std::move(host), *m_parts->world);
  m_parts->host->Start();
  // ADR-155: a world that cannot be continued stops at once; it opened without its folder, so the freeze saves nothing.
  if (m_desc.restoreOnly && !m_parts->world->Opened().restored)
  {
    m_parts->host->Stop();
    return std::unexpected(ContinueFaultOf(m_parts->world->Opened()));
  }
  return {};
}

void IntegratedServer::Stop()
{
  if (m_parts->host)
  {
    m_parts->host->Stop();
  }
}

std::unique_ptr<Neuron::Transport> IntegratedServer::Connect()
{
  return m_parts->listener.Connect();
}

} // namespace Species
