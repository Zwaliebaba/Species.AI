#include "pch.h"

#include "StructureRecords.h"

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
    RecordAt(records, place) = std::move(*record);
  }
  return records;
}

} // namespace Species
