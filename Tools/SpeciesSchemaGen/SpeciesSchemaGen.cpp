#include "pch.h"

#include "AuthoringParser.h"
#include "Files.h"
#include "HeaderWriter.h"
#include "SchemaModel.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

// The schema code generator (ADR-062, ADR-098): a development tool that never ships. It reads every schema under
// GameData/Schemas/ in name order and writes the header of each into the project folder its x-species-project names,
// under the output root (the repository root unless --out names another, as Build/CheckGeneratedSchemas.py does).
//
//   SpeciesSchemaGen.exe [--root <repository root>] [--out <output root>]

namespace
{

constexpr std::string_view SCHEMA_FOLDER = "GameData/Schemas";
constexpr std::string_view SCHEMA_SUFFIX = ".schema.json";

void Print(std::FILE* _stream, std::string_view _text)
{
  std::fwrite(_text.data(), 1, _text.size(), _stream);
  std::fputc('\n', _stream);
}

int Run(const std::vector<std::string_view>& _arguments)
{
  std::filesystem::path root = std::filesystem::current_path();
  std::filesystem::path out;
  for (std::size_t i = 0; i + 1 < _arguments.size(); i += 2)
  {
    if (_arguments[i] == "--root")
    {
      root = std::filesystem::path(_arguments[i + 1]);
    }
    else if (_arguments[i] == "--out")
    {
      out = std::filesystem::path(_arguments[i + 1]);
    }
    else
    {
      Print(stderr, std::format("SpeciesSchemaGen: unknown option {}", _arguments[i]));
      return 2;
    }
  }
  if (_arguments.size() % 2 != 0)
  {
    Print(stderr, "SpeciesSchemaGen: an option without its value");
    return 2;
  }
  if (out.empty())
  {
    out = root;
  }

  std::vector<std::filesystem::path> schemas;
  std::error_code error;
  for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(root / SCHEMA_FOLDER, error))
  {
    const std::string name = entry.path().filename().string();
    if (entry.is_regular_file() && name.ends_with(SCHEMA_SUFFIX))
    {
      schemas.push_back(entry.path());
    }
  }
  if (error)
  {
    Print(stderr, std::format("SpeciesSchemaGen: {} cannot be read", (root / SCHEMA_FOLDER).string()));
    return 1;
  }
  std::ranges::sort(schemas);

  int status = 0;
  const Neuron::AuthoringParser parser;
  for (const std::filesystem::path& schema : schemas)
  {
    const std::string file = std::format("{}/{}", SCHEMA_FOLDER, schema.filename().string());
    const auto bytes = Neuron::ReadWholeFile(schema);
    if (!bytes)
    {
      Print(stderr, std::format("SpeciesSchemaGen: {} cannot be read", file));
      status = 1;
      continue;
    }
    const auto document = parser.Parse(*bytes, file);
    if (!document)
    {
      Print(stderr, Neuron::FormatDiagnostic(document.error()));
      status = 1;
      continue;
    }
    const auto model = SchemaGen::ReadSchema(*document);
    if (!model)
    {
      for (const Neuron::ContentDiagnostic& diagnostic : model.error())
      {
        Print(stderr, Neuron::FormatDiagnostic(diagnostic));
      }
      status = 1;
      continue;
    }
    const auto header = SchemaGen::WriteHeader(*model);
    if (!header)
    {
      Print(stderr, std::format("SpeciesSchemaGen: {}: {}", file, header.error()));
      status = 1;
      continue;
    }
    const std::filesystem::path target = out / SchemaGen::HeaderPath(*model);
    std::filesystem::create_directories(target.parent_path(), error);
    const auto written = Neuron::ReplaceFileAtomically(target, std::as_bytes(std::span(header->data(), header->size())));
    if (!written)
    {
      Print(stderr, std::format("SpeciesSchemaGen: {} cannot be written", target.string()));
      status = 1;
      continue;
    }
    Print(stdout, std::format("SpeciesSchemaGen: {} -> {}", file, SchemaGen::HeaderPath(*model)));
  }
  return status;
}

} // namespace

int main(int _argc, char** _argv)
{
  try
  {
    const std::span arguments(_argv, static_cast<std::size_t>(_argc));
    std::vector<std::string_view> options;
    for (const char* argument : arguments.subspan(1))
    {
      options.emplace_back(argument);
    }
    return Run(options);
  }
  catch (const std::exception& error)
  {
    std::fputs("SpeciesSchemaGen: ", stderr);
    std::fputs(error.what(), stderr);
    std::fputc('\n', stderr);
  }
  catch (...)
  {
    std::fputs("SpeciesSchemaGen: an unknown exception\n", stderr);
  }
  return 1;
}
