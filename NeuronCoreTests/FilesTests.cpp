#include "pch.h"

#include "Files.h"
#include "TestSupport.h"

#include <array>
#include <cstring>
#include <filesystem>
#include <span>
#include <string_view>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

namespace
{

std::span<const std::byte> BytesOf(std::string_view _text)
{
  return std::as_bytes(std::span<const char>(_text.data(), _text.size()));
}

// The temporary files that ReplaceFileAtomically made for the target: its name, a dot and a writer-unique suffix.
int TemporaryFileCount(const std::filesystem::path& _target)
{
  int count = 0;
  const std::wstring prefix = _target.filename().wstring() + L".";
  for (const auto& entry : std::filesystem::directory_iterator(_target.parent_path()))
  {
    if (entry.path().filename().wstring().starts_with(prefix))
    {
      ++count;
    }
  }
  return count;
}

} // namespace

TEST_CLASS(FilesTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt089ReplaceCreatesAndThenReplacesTheTarget)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-089")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt089ReplaceCreatesAndThenReplacesTheTarget)
  {
    const TemporaryFolder folder;
    const std::filesystem::path target = folder.Path() / L"settings.json";
    Assert::IsTrue(Neuron::ReplaceFileAtomically(target, BytesOf("first")).has_value(), L"a new target is written");
    ExpectText("first", ReadText(target), L"new target");
    Assert::IsTrue(Neuron::ReplaceFileAtomically(target, BytesOf("second, longer")).has_value(), L"an old target is replaced");
    ExpectText("second, longer", ReadText(target), L"replaced target");
    ExpectEqual(0, TemporaryFileCount(target), L"temporary files left");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt089FailedReplaceKeepsTheOldContent)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-089")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt089FailedReplaceKeepsTheOldContent)
  {
    // The replacement fails after the temporary file is written: the target keeps its content (PLT T18).
    const TemporaryFolder folder;
    const std::filesystem::path target = folder.Path() / L"map.json";
    Assert::IsTrue(Neuron::ReplaceFileAtomically(target, BytesOf("old")).has_value(), L"the first write");
    HANDLE held = CreateFileW(target.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    Assert::IsTrue(held != INVALID_HANDLE_VALUE, L"the target is held open with no sharing");
    const auto result = Neuron::ReplaceFileAtomically(target, BytesOf("new"));
    CloseHandle(held);
    Assert::IsFalse(result.has_value(), L"the replacement reports its failure");
    ExpectText("old", ReadText(target), L"the target after the failure");
    ExpectEqual(0, TemporaryFileCount(target), L"temporary files left after the failure");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt090ReadWholeFileReportsAMissingFile)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt090ReadWholeFileReportsAMissingFile)
  {
    const TemporaryFolder folder;
    const auto missing = Neuron::ReadWholeFile(folder.Path() / L"missing.json");
    Assert::IsFalse(missing.has_value(), L"a missing file is an error");
    Assert::IsTrue(missing.error() == Neuron::FileError::NotFound, L"reported as not found");
    Assert::IsTrue(Neuron::ReplaceFileAtomically(folder.Path() / L"empty.json", {}).has_value(), L"an empty file is written");
    const auto empty = Neuron::ReadWholeFile(folder.Path() / L"empty.json");
    Assert::IsTrue(empty.has_value() && empty->empty(), L"an empty file reads as no bytes");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp071AFilesStampChangesWhenOneIsWritten)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-071")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp071AFilesStampChangesWhenOneIsWritten)
  {
    // ADR-186: the stamp of a build's programs is the same while neither changes, and another once one is written; a
    // missing file still counts by its name.
    const TemporaryFolder folder;
    const std::array<std::wstring_view, 2> names{L"First.exe", L"Second.exe"};
    const Neuron::Sha256Digest none = Neuron::FilesStamp(folder.Path(), names);
    Assert::IsTrue(Neuron::ReplaceFileAtomically(folder.Path() / L"First.exe", BytesOf("one")).has_value(), L"the first is written");
    Assert::IsTrue(Neuron::ReplaceFileAtomically(folder.Path() / L"Second.exe", BytesOf("two")).has_value(), L"the second is written");
    const Neuron::Sha256Digest built = Neuron::FilesStamp(folder.Path(), names);
    Assert::IsTrue(built != none, L"files written change it");
    Assert::IsTrue(Neuron::FilesStamp(folder.Path(), names) == built, L"unchanged files keep it");
    Assert::IsTrue(Neuron::ReplaceFileAtomically(folder.Path() / L"Second.exe", BytesOf("two, rebuilt")).has_value(),
                   L"the second is written again");
    Assert::IsTrue(Neuron::FilesStamp(folder.Path(), names) != built, L"a file written again changes it");
    const std::array<std::wstring_view, 2> other{L"First.exe", L"Third.exe"};
    Assert::IsTrue(Neuron::FilesStamp(folder.Path(), other) != Neuron::FilesStamp(folder.Path(), names), L"the names count");
  }
};

} // namespace NeuronCoreTests
