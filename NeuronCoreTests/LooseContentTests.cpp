#include "pch.h"

#include "Development.h"
#include "Files.h"
#include "LooseContent.h"
#include "TestSupport.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

namespace
{

Neuron::CookedEntry Entry(std::string_view _key, Neuron::ContentType _type, bool _simulationRelevant, std::string_view _text)
{
  Neuron::CookedEntry cooked;
  cooked.entry.key = std::string(_key);
  cooked.entry.type = _type;
  cooked.entry.simulationRelevant = _simulationRelevant;
  if (_type == Neuron::ContentType::Record)
  {
    cooked.entry.schemaId = "tuning.mvp";
    cooked.entry.schemaVersion = {1, 0};
  }
  const auto bytes = std::as_bytes(std::span(_text.data(), _text.size()));
  cooked.bytes.assign(bytes.begin(), bytes.end());
  return cooked;
}

// A folder of a record and a texture, written as the content build writes it.
std::filesystem::path WriteTwo(const TemporaryFolder& _folder)
{
  std::vector<Neuron::CookedEntry> entries;
  entries.push_back(Entry("textures/cursor/world", Neuron::ContentType::Texture, false, "texture bytes"));
  entries.push_back(Entry("records/tuning/mvp", Neuron::ContentType::Record, true, "record bytes"));
  const std::filesystem::path folder = _folder.Path() / L"content";
  Assert::IsTrue(Neuron::WriteLooseContent(folder, std::move(entries)).has_value(), L"the folder is written");
  return folder;
}

void Flip(const std::filesystem::path& _file)
{
  auto bytes = Neuron::ReadWholeFile(_file);
  Assert::IsTrue(bytes.has_value() && !bytes->empty());
  bytes->front() ^= std::byte{0x01};
  Assert::IsTrue(Neuron::ReplaceFileAtomically(_file, *bytes).has_value());
}

} // namespace

TEST_CLASS(LooseContentTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ast105AFolderOfCookedEntriesMountsAndReads)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AST-105")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ast105AFolderOfCookedEntriesMountsAndReads)
  {
    const TemporaryFolder folder;
    const auto content = Neuron::LooseContent::Mount(WriteTwo(folder));
    Assert::IsTrue(content.has_value(), L"the folder mounts");
    ExpectEqual(2, static_cast<std::int64_t>(content->Entries().size()), L"two entries");
    ExpectText("records/tuning/mvp", content->Entries()[0].key, L"in key order");
    const Neuron::ContentEntry* record = content->Find("Records\\Tuning\\MVP");
    Assert::IsTrue(record != nullptr && record->type == Neuron::ContentType::Record && record->schemaId == "tuning.mvp" &&
                     record->simulationRelevant,
                   L"a request in another case and with backslashes finds the entry (AST-015)");
    const auto bytes = content->Read("records/tuning/mvp");
    Assert::IsTrue(bytes.has_value() && bytes->size() == 12, L"its bytes read");
    const auto none = content->Read("records/tuning/other");
    Assert::IsTrue(!none.has_value() && none.error().fault == Neuron::AssetFault::Missing, L"a key the folder lacks is missing");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ast107AChangedEntryIsDamaged)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AST-107")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ast107AChangedEntryIsDamaged)
  {
    const TemporaryFolder folder;
    const std::filesystem::path written = WriteTwo(folder);
    Flip(written / L"records" / L"tuning" / L"mvp");
    Flip(written / L"textures" / L"cursor" / L"world");
    const auto content = Neuron::LooseContent::Mount(written);
    Assert::IsTrue(content.has_value());
    const auto record = content->Read("records/tuning/mvp");
    Assert::IsTrue(!record.has_value() && record.error().fault == Neuron::AssetFault::Damaged,
                   L"a simulation-relevant entry is checked on every load");
    const auto texture = content->Read("textures/cursor/world");
    Assert::IsTrue(texture.has_value() != (ENGINE_DEVELOPMENT != 0), L"another entry is checked in Debug builds only");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ast102AMissingOrDamagedIndexIsRefused)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AST-102")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ast102AMissingOrDamagedIndexIsRefused)
  {
    const TemporaryFolder folder;
    const auto missing = Neuron::LooseContent::Mount(folder.Path() / L"none");
    Assert::IsTrue(!missing.has_value() && missing.error().fault == Neuron::AssetFault::Missing, L"no folder is missing content");
    const std::filesystem::path written = WriteTwo(folder);
    Flip(written / Neuron::LOOSE_INDEX_FILE);
    const auto damaged = Neuron::LooseContent::Mount(written);
    Assert::IsTrue(!damaged.has_value() && damaged.error().fault == Neuron::AssetFault::Damaged, L"a changed index is damaged");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ast100TheIdentityCoversTheSimulationRelevantEntries)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AST-100")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ast100TheIdentityCoversTheSimulationRelevantEntries)
  {
    std::vector<Neuron::ContentEntry> entries{Entry("records/tuning/mvp", Neuron::ContentType::Record, true, "a").entry,
                                              Entry("textures/x", Neuron::ContentType::Texture, false, "b").entry};
    entries[0].hash[0] = std::byte{1};
    const Neuron::Sha256Digest identity = Neuron::SimulationIdentity(entries);
    entries[1].hash[0] = std::byte{2};
    Assert::IsTrue(Neuron::SimulationIdentity(entries) == identity, L"a texture changes no simulation identity");
    entries[0].hash[0] = std::byte{3};
    Assert::IsTrue(Neuron::SimulationIdentity(entries) != identity, L"a record does");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ast105TheWriterEmptiesOnlyItsOwnFolder)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AST-105")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ast105TheWriterEmptiesOnlyItsOwnFolder)
  {
    const TemporaryFolder folder;
    Assert::IsTrue(Neuron::ReplaceFileAtomically(folder.Path() / L"keep.txt", {}).has_value());
    Assert::IsFalse(Neuron::WriteLooseContent(folder.Path(), {}).has_value(), L"a folder of other files is not emptied");
    Assert::IsTrue(std::filesystem::exists(folder.Path() / L"keep.txt"), L"and keeps them");
    const std::filesystem::path written = WriteTwo(folder);
    Assert::IsTrue(Neuron::WriteLooseContent(written, {}).has_value(), L"a folder the content build wrote is written again");
    Assert::IsFalse(std::filesystem::exists(written / L"records"), L"without the entries it no longer holds");
    std::vector<Neuron::CookedEntry> twice;
    twice.push_back(Entry("a/b", Neuron::ContentType::Texture, false, "1"));
    twice.push_back(Entry("a/b", Neuron::ContentType::Texture, false, "2"));
    Assert::IsFalse(Neuron::WriteLooseContent(written, std::move(twice)).has_value(), L"a key written twice");
  }
};

} // namespace NeuronCoreTests
