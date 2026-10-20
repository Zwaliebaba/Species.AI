#pragma once

#include <cstdint>
#include <string>

namespace Neuron
{

// What is wrong with an entry a program could not load (AST §6): it is not there, or it is there and not what the
// content build wrote. A start-up shows `ui.error.content_missing` for the first and `ui.error.content_damaged` for the
// second (AST-102).
enum class AssetFault : std::uint8_t
{
  Missing,
  Damaged
};

/// Why a cooked entry, or the folder that holds it, could not be loaded: the fault, the key it concerns (empty for the
/// folder itself) and a message of literal text (AST-008).
struct AssetError
{
  AssetFault fault = AssetFault::Missing;
  std::string key;
  std::string message;
};

// "<key>: <message>", or the message alone when there is no key, for the log and the console.
[[nodiscard]] std::string FormatAssetError(const AssetError& _error);

} // namespace Neuron
