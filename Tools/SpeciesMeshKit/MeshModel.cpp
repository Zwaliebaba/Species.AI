#include "pch.h"

#include "MeshModel.h"

#include <algorithm>
#include <stdexcept>

namespace MeshKit
{

ModelNode& MeshModel::Node(const std::string& _name)
{
  const auto found = std::ranges::find(nodes, _name, &ModelNode::name);
  if (found != nodes.end())
  {
    return *found;
  }
  nodes.push_back(ModelNode{_name, {}});
  return nodes.back();
}

void MeshModel::AddSocket(std::string _name, const std::string& _node, const Point& _position)
{
  if (std::ranges::find(nodes, _node, &ModelNode::name) == nodes.end())
  {
    throw std::logic_error(file + ": the socket " + _name + " names the node " + _node + ", which the recipe has not made");
  }
  sockets.push_back(ModelSocket{std::move(_name), _node, _position});
}

std::size_t MeshModel::TriangleCount() const noexcept
{
  std::size_t count = 0;
  for (const ModelNode& node : nodes)
  {
    count += node.triangles.size();
  }
  return count;
}

} // namespace MeshKit
