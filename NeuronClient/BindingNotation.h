#pragma once

#include "InputRouter.h"

#include <expected>
#include <string>
#include <string_view>

namespace Neuron
{

// INP-071, INP-073: a binding from its JSON node, each node an object with exactly one node-kind property. Control and
// condition names are matched without regard to case (INP-055). A node with an unknown property, with no or two
// node-kind properties, without a required property, or naming an unknown control or condition is refused with a
// message naming the node's path and the word (INP-172); a gamepad leaf is refused, as the MVP has no gamepad
// (MVP-100). What a node names elsewhere (an action, a setting) and the node rules of INP-060 are the router's checks.
[[nodiscard]] std::expected<InputBinding, std::string> ParseBinding(std::string_view _json);

// INP-072: the binding's canonical JSON on one line, its properties in the order of INP-071's table and its controls
// by their canonical names, so that parsing it back gives the same binding.
[[nodiscard]] std::string WriteBinding(const InputBinding& _binding);

} // namespace Neuron
