#include "pch.h"

#include "MeshAsset.h"
#include "TestSupport.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

namespace
{

// One node with one lit draw of a triangle, and a flag socket on it.
Neuron::MeshAsset OneTriangle()
{
  Neuron::MeshAsset mesh;
  Neuron::MeshMaterial material;
  material.name = "team";
  mesh.materials.push_back(material);
  Neuron::MeshNode node;
  node.name = "leader_figure";
  node.restTransform = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  node.draws.push_back(Neuron::MeshDraw{0, 0, 3});
  node.boundsMin = {0.0F, 0.0F, 0.0F};
  node.boundsMax = {1.0F, 1.0F, 0.0F};
  node.sphereRadius = 0.75F;
  mesh.nodes.push_back(node);
  Neuron::MeshSocket socket;
  socket.kind = Neuron::SocketKind::Flag;
  socket.transform = node.restTransform;
  socket.transform[13] = 11.2F;
  mesh.sockets.push_back(socket);
  for (std::uint8_t corner = 0; corner < 3; ++corner)
  {
    Neuron::MeshVertex vertex;
    vertex.position = {corner == 1 ? 1.0F : 0.0F, corner == 2 ? 1.0F : 0.0F, 0.0F};
    vertex.normal = {0.0F, 0.0F, 1.0F};
    vertex.color = {100, 102, 113, 255};
    vertex.tint = 255;
    vertex.corner = corner;
    mesh.vertices.push_back(vertex);
  }
  return mesh;
}

} // namespace

TEST_CLASS(MeshAssetTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ast076ACookedMeshReadsBackAsWritten)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AST-076")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ast076ACookedMeshReadsBackAsWritten)
  {
    const std::vector<std::byte> bytes = Neuron::WriteMeshAsset(OneTriangle());
    const auto mesh = Neuron::ReadMeshAsset(bytes);
    Assert::IsTrue(mesh.has_value(), L"the entry reads");
    Assert::IsTrue(Neuron::WriteMeshAsset(*mesh) == bytes, L"and writes the same bytes again (AST-091)");
    ExpectEqual(1, static_cast<std::int64_t>(mesh->nodes.size()), L"one node");
    Assert::IsFalse(mesh->nodes[0].parent.has_value(), L"a root");
    ExpectText("team", mesh->materials[0].name, L"its material");
    Assert::IsTrue(mesh->sockets[0].kind == Neuron::SocketKind::Flag && mesh->sockets[0].transform[13] == 11.2F, L"its socket");
    ExpectEqual(3, static_cast<std::int64_t>(mesh->vertices.size()), L"three corners");
    ExpectEqual(255, mesh->vertices[2].tint, L"the tint weight");
    ExpectEqual(2, mesh->vertices[2].corner, L"the barycentric corner");
    Neuron::MeshAsset fewer = OneTriangle();
    fewer.vertices.clear();
    fewer.nodes[0].draws[0].vertexCount = 0;
    ExpectEqual(static_cast<std::int64_t>(Neuron::MESH_VERTEX_BYTES) * 3,
                static_cast<std::int64_t>(bytes.size() - Neuron::WriteMeshAsset(fewer).size()), L"32 bytes a vertex");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ast039SocketKindsAreTheTablesInItsOrder)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AST-039")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ast039SocketKindsAreTheTablesInItsOrder)
  {
    Assert::IsTrue(Neuron::SocketKindNamed("port") == Neuron::SocketKind::Port, L"the first kind");
    Assert::IsTrue(Neuron::SocketKindNamed("grab") == Neuron::SocketKind::Grab, L"the last kind");
    Assert::IsTrue(Neuron::SocketKindNamed("gate_entrance") == Neuron::SocketKind::GateEntrance, L"a kind with an underscore");
    Assert::IsFalse(Neuron::SocketKindNamed("Port").has_value(), L"names are exact");
    Assert::IsTrue(Neuron::SocketKindInfoOf(Neuron::SocketKind::Muzzle).indexed &&
                     Neuron::SocketKindInfoOf(Neuron::SocketKind::Muzzle).simulationRelevant,
                   L"a muzzle is indexed and simulation-relevant");
    Assert::IsFalse(Neuron::SocketKindInfoOf(Neuron::SocketKind::Flag).indexed, L"a flag is not indexed");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ast091ABrokenEntryIsRefused)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AST-091")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ast091ABrokenEntryIsRefused)
  {
    const std::vector<std::byte> bytes = Neuron::WriteMeshAsset(OneTriangle());
    std::vector<std::byte> other = bytes;
    other[0] = std::byte{2};
    Assert::IsFalse(Neuron::ReadMeshAsset(other).has_value(), L"another version");
    Assert::IsFalse(Neuron::ReadMeshAsset(std::span(bytes).first(bytes.size() - 1)).has_value(), L"a byte short");
    other = bytes;
    other.push_back(std::byte{0});
    Assert::IsFalse(Neuron::ReadMeshAsset(other).has_value(), L"a byte over");
    Neuron::MeshAsset mesh = OneTriangle();
    mesh.nodes[0].draws[0].vertexCount = 6;
    Assert::IsFalse(Neuron::ReadMeshAsset(Neuron::WriteMeshAsset(mesh)).has_value(), L"a draw past the vertices");
    mesh = OneTriangle();
    mesh.sockets[0].node = 1;
    Assert::IsFalse(Neuron::ReadMeshAsset(Neuron::WriteMeshAsset(mesh)).has_value(), L"a socket on no node");
  }
};

} // namespace NeuronCoreTests
