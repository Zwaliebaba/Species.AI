#pragma once

#include <cstdint>
#include <string>

namespace Neuron
{

// Start-up step 6 (PLT-036): a 64-bit value from the operating system's cryptographic generator, which names the run's
// log file and matches a report, a log and a dump. It identifies no player and no installation.
[[nodiscard]] std::uint64_t DrawRunId();

// The run id as it is written everywhere: 16 lowercase hexadecimal digits.
[[nodiscard]] std::string FormatRunId(std::uint64_t _runId);

} // namespace Neuron
