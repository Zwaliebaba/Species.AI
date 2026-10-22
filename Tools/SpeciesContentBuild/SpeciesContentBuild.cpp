#include "pch.h"

#include "BuildReport.h"
#include "ContentBuild.h"
#include "LooseContent.h"
#include "Sha256.h"
#include "TextEncoding.h"

#include <array>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// The content build tool (ADR-059, ADR-108): a development tool that never ships. It cooks the sources under the
// repository's GameData/ by the asset manifest and writes the loose-content folder that the programs mount (AST-105).
// The solution build runs it after linking it, into the content folder beside the executables; a run with any fault
// prints every fault, writes nothing and fails the build.
//
//   SpeciesContentBuild.exe --root <repository root> --out <loose-content folder>

namespace
{

constexpr std::string_view PRODUCT_NAME = "SpeciesContentBuild";

void Print(std::FILE* _stream, std::string_view _text)
{
  const std::string line = std::format("{}: {}\n", PRODUCT_NAME, _text);
  std::fwrite(line.data(), 1, line.size(), _stream);
  std::fflush(_stream);
}

// "texture 44, font 2": the entries of each content type, for the summary line.
std::string Census(std::span<const Neuron::ContentEntry> _entries)
{
  std::array<std::size_t, Neuron::CONTENT_TYPE_COUNT> counts{};
  for (const Neuron::ContentEntry& entry : _entries)
  {
    ++counts[static_cast<std::size_t>(entry.type)];
  }
  std::string text;
  for (std::size_t type = 0; type < counts.size(); ++type)
  {
    if (counts[type] != 0)
    {
      text +=
        std::format("{}{} {}", text.empty() ? "" : ", ", Neuron::ContentTypeName(static_cast<Neuron::ContentType>(type)), counts[type]);
    }
  }
  return text;
}

int Run(std::span<const std::wstring_view> _arguments)
{
  std::filesystem::path root = std::filesystem::current_path();
  std::filesystem::path out;
  for (std::size_t i = 0; i + 1 < _arguments.size(); i += 2)
  {
    if (_arguments[i] == L"--root")
    {
      root = std::filesystem::path(_arguments[i + 1]);
    }
    else if (_arguments[i] == L"--out")
    {
      out = std::filesystem::path(_arguments[i + 1]);
    }
    else
    {
      Print(stderr, std::format("unknown option {}", Neuron::ToUtf8(_arguments[i])));
      return 2;
    }
  }
  if (_arguments.size() % 2 != 0 || out.empty())
  {
    Print(stderr, "usage: SpeciesContentBuild.exe --root <repository root> --out <loose-content folder>");
    return 2;
  }

  ContentBuild::BuildReport report;
  ContentBuild::BuildResult result = ContentBuild::CookContent(root, report);
  if (report.Failed())
  {
    for (const ContentBuild::BuildFinding& finding : report.Findings())
    {
      Print(stderr, ContentBuild::FormatFinding(finding));
    }
    Print(stderr, std::format("{} faults; nothing was written", report.Findings().size()));
    return 1;
  }
  const std::size_t rowsWithoutFile = result.rowsWithoutFile;
  const auto written = Neuron::WriteLooseContent(out, std::move(result.entries));
  if (!written)
  {
    Print(stderr, written.error());
    return 1;
  }
  Print(stdout, std::format("{} entries ({}) into {}", written->size(), Census(*written), Neuron::ToUtf8(out.wstring())));
  Print(stdout, std::format("{} manifest rows have no authored file; {} of them have a generated stand-in (AST-082, ADR-140)",
                            rowsWithoutFile, result.placeholders));
  Print(stdout, std::format("simulation content identity {}, reported as modified while the content is loose (AST-100, AST-105)",
                            Neuron::ShortHex(Neuron::SimulationIdentity(*written))));
  return 0;
}

} // namespace

int wmain(int _argc, wchar_t** _argv)
{
  try
  {
    const std::span arguments(_argv, static_cast<std::size_t>(_argc));
    std::vector<std::wstring_view> options;
    for (const wchar_t* argument : arguments.subspan(1))
    {
      options.emplace_back(argument);
    }
    return Run(options);
  }
  catch (const std::exception& error)
  {
    std::fputs("SpeciesContentBuild: ", stderr);
    std::fputs(error.what(), stderr);
    std::fputc('\n', stderr);
  }
  catch (...)
  {
    std::fputs("SpeciesContentBuild: an unknown exception\n", stderr);
  }
  return 1;
}
