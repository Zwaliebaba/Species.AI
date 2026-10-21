#include "pch.h"

#include "PassTable.h"

#include <algorithm>

namespace Neuron
{

bool IsWrite(TargetAccess _access) noexcept
{
  switch (_access)
  {
  case TargetAccess::RenderTarget:
  case TargetAccess::DepthWrite:
  case TargetAccess::ResolveDestination:
  case TargetAccess::CopyDestination:
    return true;
  case TargetAccess::DepthRead:
  case TargetAccess::ShaderRead:
  case TargetAccess::ResolveSource:
  case TargetAccess::CopySource:
  case TargetAccess::Present:
    break;
  }
  return false;
}

std::string_view AccessName(TargetAccess _access) noexcept
{
  switch (_access)
  {
  case TargetAccess::RenderTarget:
    return "render target";
  case TargetAccess::DepthWrite:
    return "depth write";
  case TargetAccess::DepthRead:
    return "depth read";
  case TargetAccess::ShaderRead:
    return "shader read";
  case TargetAccess::ResolveSource:
    return "resolve source";
  case TargetAccess::ResolveDestination:
    return "resolve destination";
  case TargetAccess::CopySource:
    return "copy source";
  case TargetAccess::CopyDestination:
    return "copy destination";
  case TargetAccess::Present:
    return "present";
  }
  return "unknown";
}

std::expected<BarrierPlan, PassTableError> PlanBarriers(std::span<const TargetDecl> _targets, std::span<const PassDecl> _passes)
{
  std::vector<TargetAccess> current;
  std::vector<bool> written;
  std::vector<bool> readSinceWrite;
  for (const TargetDecl& target : _targets)
  {
    current.push_back(target.initial);
    written.push_back(!IsWrite(target.initial)); // a target that begins readable holds what an earlier frame or upload wrote
    readSinceWrite.push_back(true);
  }
  BarrierPlan plan;
  plan.before.resize(_passes.size());
  for (std::size_t p = 0; p < _passes.size(); ++p)
  {
    const PassDecl& pass = _passes[p];
    for (std::size_t u = 0; u < pass.uses.size(); ++u)
    {
      const TargetUse& use = pass.uses[u];
      if (use.target >= _targets.size())
      {
        return std::unexpected(PassTableError{PassTableFault::UnknownTarget, std::string(pass.name), {}});
      }
      const std::string targetName(_targets[use.target].name);
      for (std::size_t other = 0; other < u; ++other)
      {
        if (pass.uses[other].target == use.target && pass.uses[other].access != use.access)
        {
          return std::unexpected(PassTableError{PassTableFault::ConflictingUses, std::string(pass.name), targetName});
        }
      }
      if (!IsWrite(use.access) && !written[use.target])
      {
        return std::unexpected(PassTableError{PassTableFault::ReadBeforeWrite, std::string(pass.name), targetName});
      }
      if (current[use.target] != use.access)
      {
        plan.before[p].push_back({use.target, current[use.target], use.access});
        current[use.target] = use.access;
      }
      if (IsWrite(use.access))
      {
        // A clear-and-draw pass both writes and later reads its own target; the next pass that reads it ends the write.
        written[use.target] = true;
        readSinceWrite[use.target] = use.access == TargetAccess::DepthWrite;
      }
      else
      {
        readSinceWrite[use.target] = true;
      }
    }
  }
  for (std::uint32_t t = 0; t < _targets.size(); ++t)
  {
    if (!readSinceWrite[t] && _targets[t].final != TargetAccess::Present && !IsWrite(_targets[t].final))
    {
      return std::unexpected(PassTableError{PassTableFault::WrittenNeverRead, {}, std::string(_targets[t].name)});
    }
    if (current[t] != _targets[t].final)
    {
      plan.after.push_back({t, current[t], _targets[t].final});
    }
  }
  return plan;
}

} // namespace Neuron
