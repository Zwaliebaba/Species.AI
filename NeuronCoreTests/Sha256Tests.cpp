#include "pch.h"

#include "Sha256.h"
#include "TestSupport.h"

#include <cstddef>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

namespace
{

std::string Hex(const Neuron::Sha256Digest& _digest)
{
  std::string text;
  for (const std::byte value : _digest)
  {
    text += std::format("{:02x}", static_cast<unsigned>(value));
  }
  return text;
}

std::span<const std::byte> BytesOf(std::string_view _text)
{
  return std::as_bytes(std::span(_text.data(), _text.size()));
}

} // namespace

TEST_CLASS(Sha256Tests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ter252TheDigestIsFips180Sha256)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"TER-252")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ter252TheDigestIsFips180Sha256)
  {
    // The examples of FIPS 180-4 and its test vectors: the empty message, "abc", two blocks of 448 bits, and a million
    // a's given in parts.
    ExpectText("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", Hex(Neuron::Sha256Of({})), L"the empty message");
    ExpectText("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", Hex(Neuron::Sha256Of(BytesOf("abc"))), L"abc");
    ExpectText("248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1",
               Hex(Neuron::Sha256Of(BytesOf("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"))), L"448 bits");
    const std::string thousand(1000, 'a');
    Neuron::Sha256 hasher;
    for (int part = 0; part < 1000; ++part)
    {
      hasher.Update(BytesOf(thousand));
    }
    ExpectText("cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0", Hex(hasher.Finish()), L"a million a's in parts");
    ExpectText("e3b0c44298fc1c14", Neuron::ShortHex(hasher.Finish()), L"a finished hasher starts again; logs show 16 digits");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ter252PartsGiveTheDigestOfTheWhole)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"TER-252")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ter252PartsGiveTheDigestOfTheWhole)
  {
    // Every split of a message across the block boundaries of 55, 56, 63 and 64 bytes gives one digest.
    std::vector<std::byte> message(200);
    for (std::size_t n = 0; n < message.size(); ++n)
    {
      message[n] = static_cast<std::byte>(n * 7 + 3);
    }
    for (const std::size_t length : {std::size_t{55}, std::size_t{56}, std::size_t{63}, std::size_t{64}, std::size_t{200}})
    {
      const std::span<const std::byte> whole(message.data(), length);
      const Neuron::Sha256Digest expected = Neuron::Sha256Of(whole);
      for (std::size_t split = 0; split <= length; split += 9)
      {
        Neuron::Sha256 hasher;
        hasher.Update(whole.first(split));
        hasher.Update(whole.subspan(split));
        Assert::IsTrue(hasher.Finish() == expected, L"the same digest whatever the parts");
      }
    }
  }
};

} // namespace NeuronCoreTests
