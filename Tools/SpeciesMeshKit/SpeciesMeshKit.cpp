#include "pch.h"

#include "CmoWriter.h"
#include "Recipes.h"

#include <cstdio>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

// The mesh kit (M2 task T2, ADR-128): a development tool that never ships. It builds the MVP's structure meshes and
// their collision assets from their recipes and writes each as a CMO file under the output folder, GameData/Meshes/ of
// the repository root unless --out names another, as Build/CheckMeshKit.py does. A developer runs it when a recipe
// changes and commits what it writes; the content build then checks and cooks the files like any authored mesh.
//
//   SpeciesMeshKit.exe [--root <repository root>] [--out <output folder>]

namespace
{

constexpr std::string_view MESH_FOLDER = "GameData/Meshes";

void Print(std::FILE* _stream, std::string_view _text)
{
  std::fwrite(_text.data(), 1, _text.size(), _stream);
  std::fputc('\n', _stream);
}

bool WriteFile(const std::filesystem::path& _path, const std::vector<std::byte>& _bytes)
{
  std::ofstream file(_path, std::ios::binary | std::ios::trunc);
  file.write(reinterpret_cast<const char*>(_bytes.data()), static_cast<std::streamsize>(_bytes.size()));
  return static_cast<bool>(file);
}

int Run(const std::vector<std::string_view>& _arguments)
{
  std::filesystem::path root = std::filesystem::current_path();
  std::filesystem::path out;
  if (_arguments.size() % 2 != 0)
  {
    Print(stderr, "SpeciesMeshKit: an option without its value");
    return 2;
  }
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
      Print(stderr, std::format("SpeciesMeshKit: unknown option {}", _arguments[i]));
      return 2;
    }
  }
  if (out.empty())
  {
    out = root / MESH_FOLDER;
  }
  std::error_code error;
  std::filesystem::create_directories(out, error);

  std::vector<MeshKit::MeshModel> models = MeshKit::StructureRecipes();
  for (MeshKit::MeshModel& model : MeshKit::TurretRecipes())
  {
    models.push_back(std::move(model));
  }
  int status = 0;
  for (const MeshKit::MeshModel& model : models)
  {
    const std::vector<std::byte> bytes = MeshKit::WriteCmo(model);
    if (!WriteFile(out / model.file, bytes))
    {
      Print(stderr, std::format("SpeciesMeshKit: {} cannot be written", (out / model.file).string()));
      status = 1;
      continue;
    }
    Print(stdout, std::format("SpeciesMeshKit: {}: {} nodes, {} sockets, {} triangles", model.file, model.nodes.size(),
                              model.sockets.size(), model.TriangleCount()));
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
    for (std::size_t i = 1; i < arguments.size(); ++i)
    {
      options.emplace_back(arguments[i]);
    }
    return Run(options);
  }
  catch (const std::exception& error)
  {
    Print(stderr, std::format("SpeciesMeshKit: {}", error.what()));
    return 1;
  }
}
