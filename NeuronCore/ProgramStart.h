#pragma once

#include <cstdint>
#include <expected>
#include <filesystem>
#include <span>
#include <string>

namespace Neuron
{

// A program's command line: its path and each argument quoted so that CommandLineToArgvW reads them back unchanged.
[[nodiscard]] std::wstring CommandLineOf(const std::filesystem::path& _program, std::span<const std::wstring> _arguments);

// ADR-191: starts _program with _arguments in a console window of its own, in the program's folder, inheriting no handle
// of this process, so that it runs on when this process ends. Its process id, or why it did not start.
[[nodiscard]] std::expected<std::uint32_t, std::string> StartInOwnConsole(const std::filesystem::path& _program,
                                                                          std::span<const std::wstring> _arguments);

// Whether the process of _processId runs now.
[[nodiscard]] bool ProcessRunning(std::uint32_t _processId);
// This process's id.
[[nodiscard]] std::uint32_t CurrentProcessId() noexcept;

} // namespace Neuron
