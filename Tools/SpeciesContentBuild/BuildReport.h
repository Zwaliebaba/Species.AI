#pragma once

#include "ContentDiagnostic.h"

#include <string>
#include <string_view>
#include <vector>

namespace ContentBuild
{

/// One fault the content build found: the file it is in, relative to the repository root with forward slashes, and
/// what is wrong, naming the rule (AST §6).
struct BuildFinding
{
  std::string file;
  std::string message;
};

/// Every fault of a run, not only the first, so one build reports all that is wrong; a run with any fault writes
/// nothing (ADR-059).
class BuildReport
{
public:
  void Fail(std::string_view _file, std::string_view _message);
  // A finding of the authoring parser or a generated reader, whose location already names its file and line.
  void Fail(const Neuron::ContentDiagnostic& _diagnostic);

  [[nodiscard]] bool Failed() const noexcept
  {
    return !m_findings.empty();
  }
  [[nodiscard]] const std::vector<BuildFinding>& Findings() const noexcept
  {
    return m_findings;
  }

private:
  std::vector<BuildFinding> m_findings;
};

// "<file>: <message>", one line, as the build prints it.
[[nodiscard]] std::string FormatFinding(const BuildFinding& _finding);

} // namespace ContentBuild
