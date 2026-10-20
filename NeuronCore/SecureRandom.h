#pragma once

#include "SipHash.h"

#include <cstddef>
#include <span>

namespace Neuron
{

// Fills the bytes from the operating system's cryptographic generator (PLT, ADR-083 item 1); throws
// std::runtime_error when the generator fails, which only a broken system does.
void FillSecureRandom(std::span<std::byte> _bytes);

// A new secret key, such as a world's simulation key (ADR-082 item 1).
[[nodiscard]] SipHashKey DrawSecretKey();

} // namespace Neuron
