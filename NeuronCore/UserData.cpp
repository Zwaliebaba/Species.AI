#include "pch.h"

#include <shlobj.h>

#include "Files.h"
#include "UserData.h"

#include <array>
#include <cstddef>
#include <span>
#include <system_error>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")

namespace Neuron
{

namespace
{

// The marker file a root is proved writable with (PLT-018), written and removed at once.
constexpr std::wstring_view WRITE_PROBE = L"write.probe";

// The Local AppData known folder, or none.
std::optional<std::filesystem::path> LocalAppDataFolder()
{
  PWSTR raw = nullptr;
  const HRESULT result = SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, nullptr, &raw);
  std::optional<std::filesystem::path> folder;
  if (SUCCEEDED(result) && raw != nullptr)
  {
    folder = std::filesystem::path(raw);
  }
  CoTaskMemFree(raw);
  return folder;
}

// Whether the folder exists or could be created, and takes a file.
bool CreateWritable(const std::filesystem::path& _folder)
{
  std::error_code error;
  std::filesystem::create_directories(_folder, error);
  if (error || !std::filesystem::is_directory(_folder, error))
  {
    return false;
  }
  const std::filesystem::path probe = _folder / WRITE_PROBE;
  constexpr std::array<std::byte, 1> BYTES{std::byte{'1'}};
  if (!ReplaceFileAtomically(probe, BYTES).has_value())
  {
    return false;
  }
  // A probe left behind is harmless: the next start replaces it.
  [[maybe_unused]] const std::expected<void, FileError> removed = RemoveFile(probe);
  return true;
}

std::filesystem::path Normal(const std::filesystem::path& _path)
{
  std::error_code error;
  std::filesystem::path normal = std::filesystem::weakly_canonical(std::filesystem::absolute(_path, error), error);
  if (error)
  {
    normal = std::filesystem::absolute(_path, error).lexically_normal();
  }
  return normal;
}

} // namespace

bool IsInsideFolder(const std::filesystem::path& _path, const std::filesystem::path& _folder)
{
  const std::filesystem::path path = Normal(_path);
  const std::filesystem::path folder = Normal(_folder);
  auto inner = path.begin();
  for (const std::filesystem::path& part : folder)
  {
    if (part.empty())
    {
      continue; // the empty last element of a path that ends in a separator
    }
    if (inner == path.end())
    {
      return false;
    }
    const std::wstring& left = inner->native();
    const std::wstring& right = part.native();
    if (CompareStringOrdinal(left.data(), static_cast<int>(left.size()), right.data(), static_cast<int>(right.size()), TRUE) != CSTR_EQUAL)
    {
      return false;
    }
    ++inner;
  }
  return true;
}

std::expected<std::filesystem::path, UserDataFault> ResolveUserDataRoot(const std::optional<std::filesystem::path>& _option,
                                                                        std::wstring_view _product)
{
  std::filesystem::path root;
  if (_option.has_value())
  {
    if (IsInsideFolder(*_option, ExecutableFolder()))
    {
      return std::unexpected(UserDataFault::InsideInstall);
    }
    root = Normal(*_option);
  }
  else
  {
    const std::optional<std::filesystem::path> local = LocalAppDataFolder();
    if (!local.has_value())
    {
      return std::unexpected(UserDataFault::Unavailable);
    }
    root = *local / _product;
  }
  if (!CreateWritable(root))
  {
    return std::unexpected(UserDataFault::Unavailable);
  }
  return root;
}

std::optional<std::filesystem::path> UserDataFolder(const std::filesystem::path& _root, std::wstring_view _name)
{
  std::filesystem::path folder = _root / _name;
  std::error_code error;
  std::filesystem::create_directories(folder, error);
  if (error || !std::filesystem::is_directory(folder, error))
  {
    return std::nullopt;
  }
  return folder;
}

} // namespace Neuron
