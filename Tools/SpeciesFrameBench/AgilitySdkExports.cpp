#include "pch.h"

#include <cstdint>

// BLD-042 and ADR-001, as Species.exe exports them: the frame bench draws with the same Agility SDK runtime that the
// Direct3D 12 loader takes from D3D12\ beside the executables, so that it measures what the game draws with (ADR-126).
// The linker exports both constants under the loader's names (SpeciesFrameBench.vcxproj, /EXPORT); the version is the
// minor version of the Microsoft.Direct3D.D3D12 package in packages.config, which Build\CheckProjectFiles.py holds it to.
extern "C" const std::uint32_t AGILITY_SDK_VERSION = 619;     // bld.pin.d3d12_sdk_version
extern "C" const char* const AGILITY_SDK_PATH = ".\\D3D12\\"; // bld.pin.d3d12_sdk_path
