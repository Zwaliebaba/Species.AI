#include "pch.h"

#include "DdsHeader.h"
#include "TestSupport.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

namespace
{

void Put(std::vector<std::byte>& _file, std::size_t _offset, std::uint32_t _value)
{
  for (std::size_t i = 0; i < 4; ++i)
  {
    _file[_offset + i] = static_cast<std::byte>((_value >> (8 * i)) & 0xFF);
  }
}

// A DDS file with the DX10 header: a 2-D texture of one slice in the format, with its levels and payload.
std::vector<std::byte> Dx10File(std::uint32_t _width, std::uint32_t _height, std::uint32_t _levels, Neuron::TextureFormat _format,
                                std::size_t _payloadBytes)
{
  std::vector<std::byte> file(Neuron::DDS_HEADER_BYTES + Neuron::DDS_DX10_HEADER_BYTES + _payloadBytes);
  Put(file, 0, 0x20534444); // "DDS "
  Put(file, 4, 124);
  Put(file, 8, 0x1007 | 0x20000);
  Put(file, 12, _height);
  Put(file, 16, _width);
  Put(file, 28, _levels);
  Put(file, 76, 32);
  Put(file, 80, 0x4);        // DDPF_FOURCC
  Put(file, 84, 0x30315844); // "DX10"
  Put(file, 128, static_cast<std::uint32_t>(_format));
  Put(file, 132, Neuron::DDS_TEXTURE_2D);
  Put(file, 140, 1);
  return file;
}

} // namespace

TEST_CLASS(DdsHeaderTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ast068TheHeadersStateSizeLevelsAndFormat)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AST-068")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ast068TheHeadersStateSizeLevelsAndFormat)
  {
    // A 64 × 32 BC4 texture with its full chain: 6 + 1 levels of 8-byte 4 × 4 blocks, the last three one block each.
    const std::uint64_t payload = 128 * 8 + 32 * 8 + 8 * 8 + 2 * 8 + 1 * 8 + 1 * 8 + 1 * 8;
    const std::vector<std::byte> file = Dx10File(64, 32, 7, Neuron::TextureFormat::Bc4Unorm, payload);
    const auto header = Neuron::ReadDdsHeader(file);
    Assert::IsTrue(header.has_value(), L"the headers read");
    Assert::IsTrue(header->dx10 && header->Format() == Neuron::TextureFormat::Bc4Unorm, L"the DX10 header's format");
    ExpectEqual(64, header->width, L"width");
    ExpectEqual(32, header->height, L"height");
    ExpectEqual(7, header->mipLevels, L"levels");
    ExpectEqual(7, Neuron::FullMipLevels(64, 32), L"a full chain down to 1 x 1");
    ExpectEqual(static_cast<std::int64_t>(payload), static_cast<std::int64_t>(Neuron::ExpectedPayloadBytes(*header)), L"payload");
    ExpectEqual(static_cast<std::int64_t>(payload), static_cast<std::int64_t>(header->payloadBytes), L"the bytes after the headers");
    ExpectEqual(std::int64_t{64} * 64 * 4, static_cast<std::int64_t>(Neuron::LevelBytes(Neuron::TextureFormat::R8G8B8A8UnormSrgb, 64, 64)),
                L"an uncompressed level");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ast020FormatsAreNamedAsTheManifestSpellsThem)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AST-020")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ast020FormatsAreNamedAsTheManifestSpellsThem)
  {
    Assert::IsTrue(Neuron::TextureFormatNamed("BC7_UNORM_SRGB") == Neuron::TextureFormat::Bc7UnormSrgb, L"BC7 sRGB");
    Assert::IsTrue(Neuron::TextureFormatNamed("R8G8B8A8_UNORM_SRGB") == Neuron::TextureFormat::R8G8B8A8UnormSrgb, L"RGBA8 sRGB");
    Assert::IsFalse(Neuron::TextureFormatNamed("BC1_UNORM").has_value(), L"BC1 is outside ADR-017's table");
    ExpectText("BC4_UNORM", Neuron::TextureFormatName(Neuron::TextureFormat::Bc4Unorm), L"the spelling");
    Assert::IsTrue(Neuron::IsSrgb(Neuron::TextureFormat::Bc7UnormSrgb) && !Neuron::IsSrgb(Neuron::TextureFormat::Bc4Unorm), L"sRGB");
    Assert::IsTrue(Neuron::IsBlockCompressed(Neuron::TextureFormat::Bc4Unorm) && !Neuron::IsBlockCompressed(Neuron::TextureFormat::R8Unorm),
                   L"block-compressed");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ast071AFileThatIsNoDdsIsRefused)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AST-071")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ast071AFileThatIsNoDdsIsRefused)
  {
    std::vector<std::byte> file = Dx10File(4, 4, 1, Neuron::TextureFormat::Bc4Unorm, 8);
    Assert::IsFalse(Neuron::ReadDdsHeader(std::span(file).first(100)).has_value(), L"shorter than its header");
    Assert::IsFalse(Neuron::ReadDdsHeader(std::span(file).first(140)).has_value(), L"ends inside its DX10 header");
    file[0] = std::byte{'X'};
    Assert::IsFalse(Neuron::ReadDdsHeader(file).has_value(), L"a wrong magic");
    file = Dx10File(0, 4, 1, Neuron::TextureFormat::Bc4Unorm, 8);
    Assert::IsFalse(Neuron::ReadDdsHeader(file).has_value(), L"a width of 0");
    file = Dx10File(4, 4, 1, static_cast<Neuron::TextureFormat>(71), 8);
    const auto header = Neuron::ReadDdsHeader(file);
    Assert::IsTrue(header.has_value() && header->Format() == Neuron::TextureFormat::Unknown && header->dxgiFormat == 71,
                   L"a format outside the table reads as unknown, with its value kept");
  }
};

} // namespace NeuronCoreTests
