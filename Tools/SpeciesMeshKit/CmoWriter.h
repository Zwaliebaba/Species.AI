#pragma once

#include "MeshModel.h"

#include <cstddef>
#include <vector>

namespace MeshKit
{

// The CMO file of the Visual Studio mesh content pipeline that the content build's importer reads (ADR-019 as amended
// by O64, Tools/SpeciesContentBuild/CmoFile.h): one mesh per node, named as its node, with one material per surface it
// uses (`facet`, `team`, `unlit`; a collision asset's one material, `collision`, which nothing reads), one index and one
// vertex buffer with three vertices a triangle, its extents, and, on the first mesh when the asset has more than one
// node or any socket, the skeleton: a root bone at the identity for each node, then a bone `socket_<name>` for each
// socket, a child of its node's bone, translated to its position. Every value is written little-endian, and every
// float is the binary32 rounding of the recipe's binary64 value, made once.
[[nodiscard]] std::vector<std::byte> WriteCmo(const MeshModel& _model);

} // namespace MeshKit
