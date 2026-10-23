#pragma once

#include "InputEvents.h"
#include "InputRouter.h"

#include <optional>
#include <string_view>

namespace Neuron
{

// INP-055: the one name table of the bindable controls, tables 3.2-A and 3.2-B, and of the conditions of INP-051. A name
// is lower case; a look-up matches without regard to ASCII case, and the canonical name is the table's.
[[nodiscard]] std::optional<KeyCode> KeyOfName(std::string_view _name) noexcept;
[[nodiscard]] std::string_view NameOfKey(KeyCode _key) noexcept;
[[nodiscard]] std::optional<MouseControl> MouseControlOfName(std::string_view _name) noexcept;
[[nodiscard]] std::string_view NameOfMouseControl(MouseControl _control) noexcept;
[[nodiscard]] std::optional<Condition> ConditionOfName(std::string_view _name) noexcept;
[[nodiscard]] std::string_view NameOfCondition(Condition _condition) noexcept;

} // namespace Neuron
