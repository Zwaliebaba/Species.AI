#include "pch.h"

#include <cstdint>

// BLD-042 and ADR-001: the Agility SDK runtime that the Direct3D 12 loader takes from D3D12\ beside Species.exe. The
// linker exports these two constants under the names the loader reads, D3D12SDKVersion and D3D12SDKPath
// (Species.vcxproj, /EXPORT), so the names here follow AGENTS.md §1 and the SDK's spelling stays in the export table.
// The version is the minor version of the Microsoft.Direct3D.D3D12 package in packages.config, the one place it is
// pinned; Build\CheckProjectFiles.py fails when the two differ (BLD T7). The server exports neither.
extern "C" const std::uint32_t AGILITY_SDK_VERSION = 619;     // bld.pin.d3d12_sdk_version
extern "C" const char* const AGILITY_SDK_PATH = ".\\D3D12\\"; // bld.pin.d3d12_sdk_path
