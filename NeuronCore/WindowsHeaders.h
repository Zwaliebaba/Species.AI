#pragma once

// The one header that includes <windows.h> (PLT-132, AGENTS.md §4). No project file and no other header defines any
// of these macros; every source file that needs a Windows declaration includes this header instead of <windows.h>,
// before any Direct3D 12, DXGI, XAudio2 or GameInput header.
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define NOMCX
#define NOSERVICE
#define NOHELP

#include <windows.h>
