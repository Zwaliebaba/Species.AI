#include "pch.h"

#include "StructureRecords.h"

#include <format>
#include <string>
#include <utility>

namespace Species
{

Neuron::SimulationRecord& RecordAt(StructureRecords& _records, std::size_t _place) noexcept
{
  switch (_place)
  {
  case 0:
    return _records.wellspring;
  case 1:
    return _records.dwelling;
  case 2:
    return _records.storehouse;
  case 3:
    return _records.resourceSite;
  case 4:
    return _records.resourceSiteCentral;
  case 5:
    return _records.turretMount;
  case 6:
    return _records.turretHead;
  default:
    return _records.turretBarrel;
  }
}

const Neuron::SimulationRecord& RecordAt(const StructureRecords& _records, std::size_t _place) noexcept
{
  switch (_place)
  {
  case 0:
    return _records.wellspring;
  case 1:
    return _records.dwelling;
  case 2:
    return _records.storehouse;
  case 3:
    return _records.resourceSite;
  case 4:
    return _records.resourceSiteCentral;
  case 5:
    return _records.turretMount;
  case 6:
    return _records.turretHead;
  default:
    return _records.turretBarrel;
  }
}

std::expected<StructureRecords, Neuron::AssetError> LoadStructureRecords(const Neuron::LooseContent& _content)
{
  StructureRecords records;
  for (std::size_t place = 0; place < STRUCTURE_RECORD_KEYS.size(); ++place)
  {
    const std::string_view key = STRUCTURE_RECORD_KEYS[place];
    auto entry = _content.Read(key);
    if (!entry)
    {
      return std::unexpected(std::move(entry.error()));
    }
    auto record = Neuron::ReadSimulationRecord(*entry);
    if (!record)
    {
      return std::unexpected(Neuron::AssetError{Neuron::AssetFault::Damaged, std::string(key), std::move(record.error())});
    }
    if (!record->collision.empty())
    {
      auto collisionEntry = _content.Read(record->collision);
      if (!collisionEntry)
      {
        return std::unexpected(std::move(collisionEntry.error()));
      }
      auto collision = Neuron::ReadCollisionAsset(*collisionEntry);
      if (!collision)
      {
        return std::unexpected(Neuron::AssetError{Neuron::AssetFault::Damaged, record->collision, std::move(collision.error())});
      }
      records.collisions[place] = std::move(*collision);
    }
    RecordAt(records, place) = std::move(*record);
  }
  return records;
}

std::optional<Neuron::AssetError> CheckStructureRecords(const StructureRecords& _records, const TuningMvp& _tuning)
{
  if (Neuron::SocketsOf(_records.wellspring, Neuron::SocketKind::Door).empty())
  {
    return Neuron::AssetError{Neuron::AssetFault::Damaged, std::string(STRUCTURE_RECORD_KEYS[0]),
                              "the Wellspring's mesh has no door socket, where its new Workers appear (MVP-021)"};
  }
  const std::int64_t perUnit = _tuning.mvp.resource.portsPerUnit;
  const auto& construction = _tuning.mvp.construction;
  // By the records' places (STRUCTURE_RECORD_KEYS).
  const std::array<std::pair<std::size_t, std::int64_t>, 4> ports{
    {{1, construction.portsDwelling}, {2, construction.portsStorehouse}, {3, perUnit}, {4, perUnit * _tuning.wld.resource.centralFactor}}};
  for (const auto& [place, expected] : ports)
  {
    const auto found = static_cast<std::int64_t>(Neuron::SocketsOf(RecordAt(_records, place), Neuron::SocketKind::Port).size());
    if (found != expected)
    {
      return Neuron::AssetError{
        Neuron::AssetFault::Damaged, std::string(STRUCTURE_RECORD_KEYS[place]),
        std::format("the mesh has {} port sockets where the tuning gives it {} ports (MVP-030, MVP-033)", found, expected)};
    }
  }
  return std::nullopt;
}

} // namespace Species
