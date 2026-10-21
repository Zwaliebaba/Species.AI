#include "pch.h"

#include "CollisionAsset.h"
#include "MeshAsset.h"
#include "SimulationRecord.h"
#include "TestSupport.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
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

  BEGIN_TEST_METHOD_ATTRIBUTE(Ast077ACookedCollisionAssetReadsBackAsWritten)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AST-077")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AST-091")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ast077ACookedCollisionAssetReadsBackAsWritten)
  {
    Neuron::CollisionAsset asset;
    asset.nodes.push_back(Neuron::CollisionNode{"wellspring", {{0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F}, {0.0F, 2.0F, 0.0F}}, {}});
    asset.nodes[0].sphere = Neuron::BoundingSphere{{0.5F, 1.0F, 0.0F}, 1.118034F};
    asset.sphere = asset.nodes[0].sphere;
    const std::vector<std::byte> bytes = Neuron::WriteCollisionAsset(asset);
    const auto read = Neuron::ReadCollisionAsset(bytes);
    Assert::IsTrue(read.has_value(), L"it reads");
    Assert::IsTrue(read.has_value() && Neuron::WriteCollisionAsset(*read) == bytes, L"it reads back as written");
    Assert::IsTrue(read.has_value() && read->nodes.size() == 1 && read->nodes[0].corners.size() == 3 &&
                     read->nodes[0].corners[2][1] == 2.0F,
                   L"its triangle");
    Assert::IsFalse(Neuron::ReadCollisionAsset(std::span(bytes).first(bytes.size() - 1)).has_value(), L"a byte short");
    asset.nodes[0].corners.pop_back();
    Assert::IsFalse(Neuron::ReadCollisionAsset(Neuron::WriteCollisionAsset(asset)).has_value(), L"corners that are not whole triangles");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ast043ASimulationRecordPlacesItsSocketsThroughItsNodes)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AST-043")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AST-046")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AST-091")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ast043ASimulationRecordPlacesItsSocketsThroughItsNodes)
  {
    // A head at 10 wu up whose barrel mount sits 7.5 wu ahead of it: the mount rests at (0, 10, 7.5) in the mesh's space.
    const std::array<float, 16> identity{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    Neuron::SimulationRecord record;
    record.nodes.push_back(Neuron::SimulationNode{"turret_pedestal", std::nullopt, identity});
    record.nodes.push_back(Neuron::SimulationNode{"turret_head", 0U, identity});
    record.nodes[1].restTransform[13] = 10.0F;
    Neuron::SimulationSocket mount{Neuron::SocketKind::BarrelMount, 0, 1, identity};
    mount.transform[14] = 7.5F;
    record.sockets.push_back(mount);
    Neuron::SimulationSocket port{Neuron::SocketKind::Port, 1, 0, identity};
    port.transform[12] = 34.0F;
    record.sockets.push_back(port);
    port.index = 0;
    port.transform[12] = -34.0F;
    record.sockets.push_back(port);
    record.collision = "meshes/turretheadcollision";
    record.bounds = Neuron::BoundingSphere{{0.0F, 7.5F, 0.0F}, 10.0F};
    record.nodeSpheres.push_back(Neuron::SimulationSphere{1, Neuron::BoundingSphere{{0.0F, 10.5F, 0.0F}, 9.0F}});
    const std::vector<std::byte> bytes = Neuron::WriteSimulationRecord(record);
    const auto read = Neuron::ReadSimulationRecord(bytes);
    Assert::IsTrue(read.has_value() && Neuron::WriteSimulationRecord(*read) == bytes, L"it reads back as written");
    if (!read)
    {
      return;
    }
    const std::array<double, 3> at = Neuron::RestPositionOf(*read, read->sockets[0]);
    Assert::IsTrue(at[0] == 0.0 && at[1] == 10.0 && at[2] == 7.5, L"the mount through its node's rest transform");
    const std::vector<const Neuron::SimulationSocket*> ports = Neuron::SocketsOf(*read, Neuron::SocketKind::Port);
    Assert::IsTrue(ports.size() == 2 && ports[0]->index == 0 && ports[0]->transform[12] == -34.0F, L"a kind's sockets by index");
    Assert::IsTrue(read->collision == "meshes/turretheadcollision" && read->bounds.has_value(), L"its collision asset and sphere");

    Assert::IsFalse(Neuron::ReadSimulationRecord(std::span(bytes).first(bytes.size() - 1)).has_value(), L"a byte short");
    Neuron::SimulationRecord broken = record;
    broken.sockets[0].kind = Neuron::SocketKind::Flag;
    Assert::IsFalse(Neuron::ReadSimulationRecord(Neuron::WriteSimulationRecord(broken)).has_value(),
                    L"a socket the simulation never reads");
    broken = record;
    broken.sockets[0].node = 2;
    Assert::IsFalse(Neuron::ReadSimulationRecord(Neuron::WriteSimulationRecord(broken)).has_value(), L"a socket on no node");
    broken = record;
    broken.nodes[0].parent = 1U;
    Assert::IsFalse(Neuron::ReadSimulationRecord(Neuron::WriteSimulationRecord(broken)).has_value(), L"a parent after its child");
  }
};

} // namespace NeuronCoreTests
