#pragma once

#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Neuron
{

// How a pass uses a target of the frame (ADR-004, ADR-007), in engine terms: the recording layer maps each to the
// API's state, so the pass table names no Direct3D type and game code may describe passes.
enum class TargetAccess : std::uint8_t
{
  RenderTarget,
  DepthWrite,
  DepthRead,
  ShaderRead,
  ResolveSource,
  ResolveDestination,
  CopySource,
  CopyDestination,
  Present
};

// A target of the frame and the access it is in when the frame begins and must be in when it ends.
struct TargetDecl
{
  std::string_view name;
  TargetAccess initial = TargetAccess::ShaderRead;
  TargetAccess final = TargetAccess::ShaderRead;
};

struct TargetUse
{
  std::uint32_t target = 0; // an index into the frame's targets
  TargetAccess access = TargetAccess::ShaderRead;
};

struct PassDecl
{
  std::string_view name;
  std::vector<TargetUse> uses;
};

// One transition the recording layer issues before a pass, or after the last pass when the target ends the frame.
struct TargetTransition
{
  std::uint32_t target = 0;
  TargetAccess before = TargetAccess::ShaderRead;
  TargetAccess after = TargetAccess::ShaderRead;
};

/// The frame's barriers, derived from the passes' declared uses in pass order (ADR-004): before[p] holds the
/// transitions issued before pass p, and after the transitions back to each target's final access.
struct BarrierPlan
{
  std::vector<std::vector<TargetTransition>> before;
  std::vector<TargetTransition> after;
};

enum class PassTableFault : std::uint8_t
{
  UnknownTarget,   // a use names no target of the frame
  ReadBeforeWrite, // a pass reads a target that nothing wrote this frame and that does not begin the frame readable
  ConflictingUses, // one pass uses one target in two accesses
  WrittenNeverRead // a target written this frame that no later pass reads and that does not end the frame presented
};

struct PassTableError
{
  PassTableFault fault = PassTableFault::UnknownTarget;
  std::string pass;
  std::string target;
};

// Validates the ordered pass table and derives its barriers (ADR-004, ADR-007). A target that begins the frame in a
// read access counts as written before the frame (a texture uploaded earlier); a render or depth target that a pass
// writes is read by the pass that resolves, copies, presents or samples it.
[[nodiscard]] std::expected<BarrierPlan, PassTableError> PlanBarriers(std::span<const TargetDecl> _targets,
                                                                      std::span<const PassDecl> _passes);

[[nodiscard]] bool IsWrite(TargetAccess _access) noexcept;
[[nodiscard]] std::string_view AccessName(TargetAccess _access) noexcept;

} // namespace Neuron
