#include "pch.h"

#include "CookedRecord.h"
#include "TestSupport.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

TEST_CLASS(CookedRecordTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dat014ValuesAreLittleEndianAndReadBack)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DAT-014")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dat014ValuesAreLittleEndianAndReadBack)
  {
    Neuron::CookedWriter writer;
    writer.U16(0x0102);
    writer.I64(-2);
    writer.F64(0.1);
    writer.F32(-1.5F);
    writer.Text8("abc");
    const std::span<const std::byte> bytes = writer.View();
    Assert::IsTrue(bytes[0] == std::byte{0x02} && bytes[1] == std::byte{0x01}, L"the low byte first");
    ExpectEqual(2 + 8 + 8 + 4 + 1 + 3, static_cast<std::int64_t>(bytes.size()), L"no padding");

    Neuron::CookedReader reader(bytes);
    ExpectEqual(0x0102, reader.U16(), L"a 16-bit value");
    ExpectEqual(-2, reader.I64(), L"a negative 64-bit value");
    Assert::IsTrue(reader.F64() == 0.1, L"a binary64 value, bit for bit");
    Assert::IsTrue(reader.F32() == -1.5F, L"a binary32 value");
    ExpectText("abc", reader.Text8(), L"a text after its length");
    Assert::IsTrue(reader.Finished(), L"every byte read");
    ExpectEqual(0, reader.U8(), L"a read past the end reads 0");
    Assert::IsTrue(reader.Failed() && !reader.Finished(), L"and fails the reader");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dat014TheHeaderNamesTheSchemaVersionLengthAndCount)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DAT-014")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dat014TheHeaderNamesTheSchemaVersionLengthAndCount)
  {
    const std::vector<std::byte> payload{std::byte{1}, std::byte{2}, std::byte{3}};
    const std::vector<std::byte> entry = Neuron::CookRecordEntry("tuning.mvp", {1, 0}, 1, payload);
    const auto opened = Neuron::OpenCookedRecord(entry, "tuning.mvp", {1, 0}, 1);
    Assert::IsTrue(opened.has_value() && opened->size() == 3 && (*opened)[2] == std::byte{3}, L"the payload is a view of its bytes");
    Assert::IsFalse(Neuron::OpenCookedRecord(entry, "actor.stats", {1, 0}, 1).has_value(), L"another schema");
    Assert::IsFalse(Neuron::OpenCookedRecord(entry, "tuning.mvp", {1, 1}, 1).has_value(), L"another version (DAT-017)");
    Assert::IsFalse(Neuron::OpenCookedRecord(entry, "tuning.mvp", {1, 0}, 2).has_value(), L"another record count");
    const std::vector<std::byte> shorter(entry.begin(), entry.end() - 1);
    Assert::IsFalse(Neuron::OpenCookedRecord(shorter, "tuning.mvp", {1, 0}, 1).has_value(), L"a payload shorter than its length");
    Assert::IsFalse(Neuron::OpenCookedRecord(std::span(entry).first(4), "tuning.mvp", {1, 0}, 1).has_value(), L"a header cut short");
  }
};

} // namespace NeuronCoreTests
