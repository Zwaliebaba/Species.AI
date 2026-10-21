#include "pch.h"

#include "Settlement.h"
#include "StateValues.h"

#include <algorithm>
#include <cmath>

namespace Species
{

namespace
{

// Bounds for the reader of a Cladan's modifiers: more than a later stage defines, and a tuning key's length.
constexpr std::uint64_t MODIFIERS_MAX = 255;
constexpr std::size_t MODIFIER_KEY_MAX_BYTES = 128;

} // namespace

std::int64_t MilliRuOf(double _resourceUnits) noexcept
{
  return std::llround(_resourceUnits * 1000.0);
}

std::int64_t StoreCapacityMilliRu(const TuningMvp& _tuning, std::uint32_t _activeStorehouses) noexcept
{
  const auto& store = _tuning.mvp.store;
  return MilliRuOf(store.wellspringCapacityRu) + std::int64_t{_activeStorehouses} * MilliRuOf(store.storehouseCapacityRu);
}

bool Deposit(Settlement& _settlement, std::int64_t _amountMilliRu, std::int64_t _capacityMilliRu) noexcept
{
  if (_amountMilliRu < 0 || _amountMilliRu > _capacityMilliRu - _settlement.storesMilliRu)
  {
    return false;
  }
  _settlement.storesMilliRu += _amountMilliRu;
  return true;
}

bool Withdraw(Settlement& _settlement, std::int64_t _amountMilliRu) noexcept
{
  if (_amountMilliRu < 0 || _amountMilliRu > _settlement.storesMilliRu)
  {
    return false;
  }
  _settlement.storesMilliRu -= _amountMilliRu;
  return true;
}

std::uint32_t PopulationLimit(const TuningMvp& _tuning, std::uint32_t _activeDwellings) noexcept
{
  const auto& settlement = _tuning.mvp.settlement;
  const std::int64_t limit = settlement.populationBase + std::int64_t{_activeDwellings} * _tuning.mvp.dwelling.population;
  return static_cast<std::uint32_t>(std::clamp<std::int64_t>(limit, 0, settlement.populationMax));
}

void WriteSettlement(Neuron::BitWriter& _writer, const Settlement& _settlement)
{
  _writer.WriteBits(_settlement.site, 8);
  _writer.WriteBits(_settlement.team, 8);
  _writer.WriteBool(_settlement.stewarded);
  _writer.WriteBits(_settlement.cladan.color, 8);
  _writer.WriteBounded(static_cast<std::uint64_t>(_settlement.cladan.temperament), TEMPERAMENT_MAX);
  _writer.WriteBounded(_settlement.cladan.modifiers.size(), MODIFIERS_MAX);
  for (const CladanModifier& modifier : _settlement.cladan.modifiers)
  {
    _writer.WriteString(modifier.key, MODIFIER_KEY_MAX_BYTES);
    WriteDouble(_writer, modifier.factor);
  }
  _writer.WriteBounded(static_cast<std::uint64_t>(_settlement.fidelity), FIDELITY_MAX);
  _writer.WriteBits(_settlement.wellspringId, 64);
  _writer.WriteBits(_settlement.wellspringSlot, 32);
  _writer.WriteSigned(_settlement.storesMilliRu, 64);
  WriteDouble(_writer, _settlement.territoryRadius);
}

bool ReadSettlement(Neuron::BitReader& _reader, Settlement& _settlement)
{
  _settlement.site = static_cast<std::uint8_t>(_reader.ReadBits(8));
  _settlement.team = static_cast<std::uint8_t>(_reader.ReadBits(8));
  _settlement.stewarded = _reader.ReadBool();
  _settlement.cladan.color = static_cast<std::uint8_t>(_reader.ReadBits(8));
  _settlement.cladan.temperament = static_cast<Temperament>(_reader.ReadBounded(TEMPERAMENT_MAX));
  const std::uint64_t modifiers = _reader.ReadBounded(MODIFIERS_MAX);
  _settlement.cladan.modifiers.clear();
  bool finite = true;
  for (std::uint64_t index = 0; index < modifiers && !_reader.Failed(); ++index)
  {
    CladanModifier& modifier = _settlement.cladan.modifiers.emplace_back();
    modifier.key = std::string(_reader.ReadString(MODIFIER_KEY_MAX_BYTES));
    finite = ReadDouble(_reader, modifier.factor) && finite;
  }
  _settlement.fidelity = static_cast<Fidelity>(_reader.ReadBounded(FIDELITY_MAX));
  _settlement.wellspringId = _reader.ReadBits(64);
  _settlement.wellspringSlot = static_cast<std::uint32_t>(_reader.ReadBits(32));
  _settlement.storesMilliRu = _reader.ReadSigned(64);
  finite = ReadDouble(_reader, _settlement.territoryRadius) && finite;
  return !_reader.Failed() && finite && _settlement.wellspringId != 0 && _settlement.storesMilliRu >= 0 &&
         _settlement.territoryRadius >= 0.0;
}

} // namespace Species
