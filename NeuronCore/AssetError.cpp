#include "pch.h"

#include "AssetError.h"

#include <format>

namespace Neuron
{

std::string FormatAssetError(const AssetError& _error)
{
  return _error.key.empty() ? _error.message : std::format("{}: {}", _error.key, _error.message);
}

} // namespace Neuron
