#pragma once

#include <string>
#include <string_view>

namespace Neuron
{

// Strings are UTF-8 everywhere except at an operating-system call (PLT-090). An ill-formed sequence or an unpaired
// surrogate becomes U+FFFD, never an error: the text is shown or logged, not trusted.
[[nodiscard]] std::wstring ToUtf16(std::string_view _utf8);
[[nodiscard]] std::string ToUtf8(std::wstring_view _utf16);

} // namespace Neuron
