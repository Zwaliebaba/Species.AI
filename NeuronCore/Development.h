#pragma once

// The development switch (BLD-014, table 3.1-B): 1 in Debug, where NDEBUG is not defined, and 0 in Release. This is
// its one definition: no project, property or source file sets ENGINE_DEVELOPMENT or any other switch by hand, and
// every switch of the table except optimisation follows from it (ADR-056). A switch exists only for code that must
// not be present in a Release binary.
#if defined(NDEBUG)
#   define ENGINE_DEVELOPMENT 0
#else
#   define ENGINE_DEVELOPMENT 1
#endif
