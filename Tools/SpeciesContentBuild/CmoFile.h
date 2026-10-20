#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <vector>

namespace ContentBuild
{

// The texture slots of a CMO material.
inline constexpr std::size_t CMO_TEXTURE_SLOTS = 8;

struct CmoMaterial
{
  std::string name;
  std::array<float, 4> ambient{};
  std::array<float, 4> diffuse{};
  std::array<float, 4> specular{};
  float specularPower = 0.0F;
  std::array<float, 4> emissive{};
  std::array<float, 16> uvTransform{};
  std::string pixelShader;
  std::array<std::string, CMO_TEXTURE_SLOTS> textures;
};

struct CmoSubmesh
{
  std::uint32_t material = 0;
  std::uint32_t indexBuffer = 0;
  std::uint32_t vertexBuffer = 0;
  std::uint32_t startIndex = 0;
  std::uint32_t primitiveCount = 0;
};

/// A vertex of the Visual Studio mesh content pipeline, 52 bytes: position, normal, tangent, an R8G8B8A8_UNORM colour
/// with red in the low byte, and texture coordinates (ADR-019 as amended by O64).
struct CmoVertex
{
  std::array<float, 3> position{};
  std::array<float, 3> normal{};
  std::array<float, 4> tangent{};
  std::uint32_t color = 0;
  std::array<float, 2> textureCoordinates{};
};

// A bone: its name, its parent (-1 for a root) and its matrices, row-major with row vectors.
struct CmoBone
{
  std::string name;
  std::int32_t parent = -1;
  std::array<float, 16> inverseBindPose{};
  std::array<float, 16> bindPose{};
  std::array<float, 16> localTransform{};
};

struct CmoMesh
{
  std::string name;
  std::vector<CmoMaterial> materials;
  bool skeleton = false;
  std::vector<CmoSubmesh> submeshes;
  std::vector<std::vector<std::uint16_t>> indexBuffers;
  std::vector<std::vector<CmoVertex>> vertexBuffers;
  std::array<float, 10> extents{}; // centre, radius, minimum and maximum
  std::vector<CmoBone> bones;
};

/// A CMO file of the Visual Studio mesh content pipeline, as the importer reads it: its meshes, each with its
/// materials, submeshes, buffers and, when it carries the skeleton, bones.
struct CmoFile
{
  std::vector<CmoMesh> meshes;
};

// The importer of ADR-019 as amended by O64, which is Species code with no third-party reader. It reads a CMO file and
// applies the amendment's checks in their order:
//   1. every count and string length leads to data inside the file, and reading ends exactly at its end;
//   2. every string is UTF-16 of the length stated, ending in its one NUL;
//   3. every submesh's material, index buffer and vertex buffer exist, and its index range lies in its index buffer;
//   4. every index is below its vertex buffer's count, and every index buffer's count is a multiple of 3;
//   5. every float is finite;
//   6. every bone's parent is -1 or a bone before it, so the bones form disjoint trees;
//   7. bone names and mesh names are unique, and every mesh's name is a bone's when the file has a skeleton;
//   8. there are no skinning buffers and no animation clips, and at most one mesh carries the skeleton.
// The file, or the first failure: its byte offset, the rule and the value found.
[[nodiscard]] std::expected<CmoFile, std::string> ReadCmoFile(std::span<const std::byte> _bytes);

} // namespace ContentBuild
