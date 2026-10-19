#pragma once

namespace Neuron
{

/// One CPU marker: a PIX event from construction to destruction, in Debug and Release alike (ADR-068, DBG-080). Safe
/// on any thread; the events nest per thread. The name is a marker of the catalogue (DBG-045), a string literal.
class ProfileScope
{
public:
  explicit ProfileScope(const char* _name) noexcept;
  ~ProfileScope();

  ProfileScope(const ProfileScope&) = delete;
  ProfileScope& operator=(const ProfileScope&) = delete;
};

} // namespace Neuron

#define ENGINE_PROFILE_JOIN_INNER(first, second) first##second
#define ENGINE_PROFILE_JOIN(first, second) ENGINE_PROFILE_JOIN_INNER(first, second)

// A CPU marker for the rest of the enclosing scope (ADR-068). No other code calls the PIX runtime.
#define ENGINE_PROFILE_SCOPE(name) const ::Neuron::ProfileScope ENGINE_PROFILE_JOIN(profileScope, __LINE__)(name)
