#include "pch.h"

#include "BuildReport.h"

#include <algorithm>
#include <format>

namespace ContentBuild
{

void BuildReport::Fail(std::string_view _file, std::string_view _message)
{
  m_findings.push_back(BuildFinding{std::string(_file), std::string(_message)});
}

void BuildReport::Fail(const Neuron::ContentDiagnostic& _diagnostic)
{
  // FormatDiagnostic starts with the file, so the finding keeps the rest of its line as the message.
  const std::string text = Neuron::FormatDiagnostic(_diagnostic);
  const std::string_view rest = std::string_view{text}.substr(std::min(text.size(), _diagnostic.at.file.size() + 1));
  m_findings.push_back(BuildFinding{_diagnostic.at.file, std::string(rest)});
}

std::string FormatFinding(const BuildFinding& _finding)
{
  return std::format("{}: {}", _finding.file, _finding.message);
}

} // namespace ContentBuild
