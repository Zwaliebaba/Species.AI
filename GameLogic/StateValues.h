#pragma once

#include "BitReader.h"
#include "BitWriter.h"
#include "Vec3.h"

#include <bit>
#include <cmath>
#include <cstdint>

namespace Species
{

// The canonical form of a binary64 value in a region's state (ADR-081 item 7): its 64 bits.
inline void WriteDouble(Neuron::BitWriter& _writer, double _value)
{
  _writer.WriteBits(std::bit_cast<std::uint64_t>(_value), 64);
}

inline void WriteVec3(Neuron::BitWriter& _writer, const Neuron::Vec3& _value)
{
  WriteDouble(_writer, _value.x);
  WriteDouble(_writer, _value.y);
  WriteDouble(_writer, _value.z);
}

// R-SIM-107: no simulation field holds NaN or an infinity, so a value that does is no state.
[[nodiscard]] inline bool ReadDouble(Neuron::BitReader& _reader, double& _value)
{
  _value = std::bit_cast<double>(_reader.ReadBits(64));
  return std::isfinite(_value);
}

[[nodiscard]] inline bool ReadVec3(Neuron::BitReader& _reader, Neuron::Vec3& _value)
{
  const bool x = ReadDouble(_reader, _value.x);
  const bool y = ReadDouble(_reader, _value.y);
  const bool z = ReadDouble(_reader, _value.z);
  return x && y && z;
}

} // namespace Species
