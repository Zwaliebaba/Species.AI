# P11_terrain_world — Landscape, water, clouds, level file, global world

Scope: `code/landscape.{h,cpp}`, `code/landscape_renderer.{h,cpp}`, `code/water.{h,cpp}`, `code/water_reflection.{h,cpp}`, `code/clouds.{h,cpp}`, `code/level_file.{h,cpp}`, `code/mapfile.{h,cpp}`, `code/mapcrc.h`, `code/global_world.{h,cpp}`, `code/global_internet.{h,cpp}`; plus usage of `data/terrain/*`, `data/locations.txt`, representative `data/levels/*`. Supporting library code read to make the terrain reproducible: `code/lib/2d_array.{h,cpp}`, `code/lib/2d_surface_map.{h,cpp}`, `code/lib/math_utils.cpp:Log2`, `contrib/systemiv/lib/math/random_number.cpp`, `contrib/systemiv/lib/math/math_stubs.cpp:iv_trunc_low_nibble`. Build baseline: see build_matrix.md (`Release|Win32` OpenGL + `TARGET_PC_FULLGAME` = [B:FULL]; `USE_DIRECT3D` = [B:D3D]).

(Draft in progress — sections appended incrementally.)
