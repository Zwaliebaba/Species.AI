#include "pch.h"

#include "SecureRandom.h"
#include "SipHash.h"
#include "TestSupport.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

namespace
{

// The published test vectors of SipHash-2-4 (Aumasson and Bernstein, 2012, appendix A and vectors.h): the key is the
// bytes 0 to 15, and message n is the bytes 0 to n - 1.
constexpr std::array<std::uint64_t, 64> SIPHASH24_VECTORS{
  0x726fdb47dd0e0e31, 0x74f839c593dc67fd, 0x0d6c8009d9a94f5a, 0x85676696d7fb7e2d, 0xcf2794e0277187b7, 0x18765564cd99a68d,
  0xcbc9466e58fee3ce, 0xab0200f58b01d137, 0x93f5f5799a932462, 0x9e0082df0ba9e4b0, 0x7a5dbbc594ddb9f3, 0xf4b32f46226bada7,
  0x751e8fbc860ee5fb, 0x14ea5627c0843d90, 0xf723ca908e7af2ee, 0xa129ca6149be45e5, 0x3f2acc7f57c29bdb, 0x699ae9f52cbe4794,
  0x4bc1b3f0968dd39c, 0xbb6dc91da77961bd, 0xbed65cf21aa2ee98, 0xd0f2cbb02e3b67c7, 0x93536795e3a33e88, 0xa80c038ccd5ccec8,
  0xb8ad50c6f649af94, 0xbce192de8a85b8ea, 0x17d835b85bbb15f3, 0x2f2e6163076bcfad, 0xde4daaaca71dc9a5, 0xa6a2506687956571,
  0xad87a3535c49ef28, 0x32d892fad841c342, 0x7127512f72f27cce, 0xa7f32346f95978e3, 0x12e0b01abb051238, 0x15e034d40fa197ae,
  0x314dffbe0815a3b4, 0x027990f029623981, 0xcadcd4e59ef40c4d, 0x9abfd8766a33735c, 0x0e3ea96b5304a7d0, 0xad0c42d6fc585992,
  0x187306c89bc215a9, 0xd4a60abcf3792b95, 0xf935451de4f21df2, 0xa9538f0419755787, 0xdb9acddff56ca510, 0xd06c98cd5c0975eb,
  0xe612a3cb9ecba951, 0xc766e62cfcadaf96, 0xee64435a9752fe72, 0xa192d576b245165a, 0x0a8787bf8ecb74b2, 0x81b3e73d20b49b6f,
  0x7fa8220ba3b2ecea, 0x245731c13ca42499, 0xb78dbfaf3a8d83bd, 0xea1ad565322a1a0b, 0x60e61c23a3795013, 0x6606d7e446282b93,
  0x6ca4ecb15c5f91e1, 0x9f626da15c9625f3, 0xe51b38608ef25f57, 0x958a324ceb064572,
};

constexpr Neuron::SipHashKey VECTOR_KEY{0x0706050403020100, 0x0f0e0d0c0b0a0908};

} // namespace

TEST_CLASS(SipHashTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp093SipHashMatchesThePublishedVectors)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-093")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp093SipHashMatchesThePublishedVectors)
  {
    std::vector<std::byte> message;
    for (std::size_t n = 0; n < SIPHASH24_VECTORS.size(); ++n)
    {
      Assert::IsTrue(Neuron::SipHash24(VECTOR_KEY, message) == SIPHASH24_VECTORS[n], L"a published vector");
      message.push_back(static_cast<std::byte>(n));
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp093WordsAreTheirLittleEndianBytes)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-093")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp093WordsAreTheirLittleEndianBytes)
  {
    const std::array<std::uint64_t, 5> words{1, 0x0123456789abcdef, 0xfedcba9876543210, 600, 0};
    std::vector<std::byte> bytes;
    for (const std::uint64_t word : words)
    {
      for (int i = 0; i < 8; ++i)
      {
        bytes.push_back(static_cast<std::byte>((word >> (8 * i)) & 0xff));
      }
    }
    Assert::IsTrue(Neuron::SipHash24Words(VECTOR_KEY, {1, 0x0123456789abcdef, 0xfedcba9876543210, 600, 0}) ==
                     Neuron::SipHash24(VECTOR_KEY, bytes),
                   L"the 40-byte message of ADR-082 item 2");
    Assert::IsTrue(Neuron::SipHash24Words(VECTOR_KEY, {}) == SIPHASH24_VECTORS[0], L"no words is the empty message");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp093ANewSecretKeyIsDrawnEachTime)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-093")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp093ANewSecretKeyIsDrawnEachTime)
  {
    const Neuron::SipHashKey first = Neuron::DrawSecretKey();
    const Neuron::SipHashKey second = Neuron::DrawSecretKey();
    Assert::IsTrue(first.low != second.low || first.high != second.high, L"two keys from the system generator differ");
  }
};

} // namespace NeuronCoreTests
