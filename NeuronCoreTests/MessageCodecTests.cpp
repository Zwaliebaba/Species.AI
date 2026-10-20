#include "pch.h"

#include "MessageCodec.h"
#include "StreamFraming.h"
#include "TestSupport.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

namespace
{

// A message as a generated codec would write it (ADR-062): a bounded count, a signed value, a flag and a name.
struct TestMessage
{
  static constexpr Neuron::MessageTypeId TYPE = 7;
  static constexpr std::uint8_t VERSION = 2;
  static constexpr std::size_t MAX_BYTES = 64;
  static constexpr std::uint64_t MAX_COUNT = 100;
  static constexpr std::size_t MAX_NAME_BYTES = 16;

  std::uint64_t count = 0;
  std::int64_t offset = 0;
  bool flag = false;
  std::string name;

  void Encode(Neuron::BitWriter& _writer) const
  {
    _writer.WriteBounded(count, MAX_COUNT);
    _writer.WriteSigned(offset, 12);
    _writer.WriteBool(flag);
    _writer.WriteString(name, MAX_NAME_BYTES);
  }

  void Decode(Neuron::BitReader& _reader)
  {
    count = _reader.ReadBounded(MAX_COUNT);
    offset = _reader.ReadSigned(12);
    flag = _reader.ReadBool();
    name = std::string(_reader.ReadString(MAX_NAME_BYTES));
  }
};

// SplitMix64: a fixed sequence for every run, so a failure reproduces.
class TestRandom
{
public:
  explicit TestRandom(std::uint64_t _seed) noexcept
    : m_state(_seed)
  {
  }

  std::uint64_t operator()() noexcept
  {
    std::uint64_t value = (m_state += 0x9E37'79B9'7F4A'7C15);
    value = (value ^ (value >> 30)) * 0xBF58'476D'1CE4'E5B9;
    value = (value ^ (value >> 27)) * 0x94D0'49BB'1331'11EB;
    return value ^ (value >> 31);
  }

private:
  std::uint64_t m_state;
};

std::vector<std::byte> Bytes(std::initializer_list<unsigned> _values)
{
  std::vector<std::byte> bytes;
  for (const unsigned value : _values)
  {
    bytes.push_back(static_cast<std::byte>(value));
  }
  return bytes;
}

} // namespace

TEST_CLASS(MessageCodecTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp090BitFieldsRoundTrip)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp090BitFieldsRoundTrip)
  {
    TestRandom random(20261020);
    std::vector<std::uint64_t> values;
    Neuron::BitWriter writer;
    for (std::uint32_t bits = 0; bits <= 64; ++bits)
    {
      const std::uint64_t value = bits == 64 ? random() : random() & ((std::uint64_t{1} << bits) - 1);
      values.push_back(value);
      writer.WriteBits(value, bits);
    }
    writer.WriteBool(true);
    writer.WriteSigned(-5, 4);
    writer.WriteSigned(-(std::int64_t{1} << 62), 64);
    writer.WriteBounded(5, 5);
    const std::array<std::byte, 3> raw{std::byte{1}, std::byte{2}, std::byte{255}};
    writer.WriteBytes(raw, 8);
    writer.WriteString("Gr\xC3\xBC\xC3\x9F", 8);
    ExpectEqual(static_cast<std::int64_t>((writer.BitCount() + 7) / 8), static_cast<std::int64_t>(writer.Bytes().size()), L"bytes");

    const std::vector<std::byte> bytes = writer.Take();
    Neuron::BitReader reader(bytes);
    for (std::uint32_t bits = 0; bits <= 64; ++bits)
    {
      Assert::IsTrue(reader.ReadBits(bits) == values[bits], L"an unsigned field of every width");
    }
    Assert::IsTrue(reader.ReadBool(), L"a flag");
    ExpectEqual(-5, reader.ReadSigned(4), L"a negative field");
    ExpectEqual(-(std::int64_t{1} << 62), reader.ReadSigned(64), L"a 64-bit field");
    ExpectEqual(5, static_cast<std::int64_t>(reader.ReadBounded(5)), L"a bounded field");
    const std::span<const std::byte> read = reader.ReadBytes(8);
    Assert::IsTrue(std::equal(read.begin(), read.end(), raw.begin(), raw.end()), L"bytes");
    ExpectText("Gr\xC3\xBC\xC3\x9F", reader.ReadString(8), L"UTF-8 text");
    Assert::IsTrue(reader.Finish(), L"the whole message is read");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp090ReaderRejectsWhatBreaksItsBounds)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp090ReaderRejectsWhatBreaksItsBounds)
  {
    const auto fails = [](const std::vector<std::byte>& _bytes, auto _read)
    {
      Neuron::BitReader reader(_bytes);
      _read(reader);
      return !reader.Finish() && reader.Failed();
    };
    Assert::IsTrue(fails(Bytes({0xFF}), [](Neuron::BitReader& _r) { static_cast<void>(_r.ReadBits(9)); }), L"past the end");
    Assert::IsTrue(fails(Bytes({0x07}), [](Neuron::BitReader& _r) { static_cast<void>(_r.ReadBounded(5)); }), L"7 in a field of at most 5");
    Assert::IsTrue(fails(Bytes({0x05, 1, 2, 3, 4, 5}), [](Neuron::BitReader& _r) { static_cast<void>(_r.ReadBytes(4)); }),
                   L"a length above its limit");
    Assert::IsTrue(fails(Bytes({0x03, 1, 2}), [](Neuron::BitReader& _r) { static_cast<void>(_r.ReadBytes(4)); }), L"a length past the end");
    Assert::IsTrue(fails(Bytes({0x01, 0xC0}), [](Neuron::BitReader& _r) { static_cast<void>(_r.ReadString(4)); }), L"an overlong form");
    Assert::IsTrue(fails(Bytes({0x03, 0xED, 0xA0, 0x80}), [](Neuron::BitReader& _r) { static_cast<void>(_r.ReadString(4)); }),
                   L"a surrogate");
    Assert::IsTrue(fails(Bytes({0x03}), [](Neuron::BitReader& _r) { static_cast<void>(_r.ReadBits(1)); }), L"a one in the padding");
    Assert::IsTrue(fails(Bytes({0x01, 0x00}), [](Neuron::BitReader& _r) { static_cast<void>(_r.ReadBits(8)); }), L"a byte left over");
    Assert::IsTrue(fails(Bytes({}),
                         [](Neuron::BitReader& _r)
                         {
                           static_cast<void>(_r.Enter(2));
                           static_cast<void>(_r.Enter(2));
                           static_cast<void>(_r.Enter(2));
                         }),
                   L"nesting deeper than its limit");
    Assert::IsTrue(fails(Bytes({}), [](Neuron::BitReader& _r) { static_cast<void>(_r.Enter(2)); }), L"a nested value left open");
    Assert::IsTrue(fails(Bytes({}), [](Neuron::BitReader& _r) { _r.Leave(); }), L"a nested value closed twice");

    Neuron::BitReader failed(Bytes({0xFF}));
    static_cast<void>(failed.ReadBits(16));
    Assert::IsTrue(failed.ReadBits(4) == 0 && !failed.ReadBool() && failed.ReadBytes(4).empty(), L"a failed reader reads nothing more");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp090DispatcherDeliversOnlyWholeMessages)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp090DispatcherDeliversOnlyWholeMessages)
  {
    Neuron::MessageDispatcher dispatcher;
    std::vector<TestMessage> delivered;
    dispatcher.Register<TestMessage>([&delivered](const TestMessage& _message) { delivered.push_back(_message); });

    const TestMessage sent{42, -1000, true, "Leader"};
    const std::vector<std::byte> bytes = Neuron::EncodeMessage(sent);
    Assert::IsTrue(dispatcher.Dispatch(bytes), L"a whole message is delivered");
    Assert::IsTrue(delivered.size() == 1 && delivered[0].count == 42 && delivered[0].offset == -1000 && delivered[0].flag &&
                     delivered[0].name == "Leader",
                   L"with its fields");

    Neuron::BitWriter unknown;
    Neuron::WriteMessageHeader(unknown, {8, 1});
    Assert::IsFalse(dispatcher.Dispatch(unknown.Take()), L"an unknown type");
    Neuron::BitWriter oldVersion;
    Neuron::WriteMessageHeader(oldVersion, {TestMessage::TYPE, 1});
    sent.Encode(oldVersion);
    Assert::IsFalse(dispatcher.Dispatch(oldVersion.Take()), L"another version");
    std::vector<std::byte> oversized = bytes;
    oversized.resize(TestMessage::MAX_BYTES + 1);
    Assert::IsFalse(dispatcher.Dispatch(oversized), L"larger than its type allows");
    Assert::IsFalse(dispatcher.Dispatch(std::span(bytes).first(bytes.size() - 1)), L"cut short");
    std::vector<std::byte> trailing = bytes;
    trailing.push_back(std::byte{0});
    Assert::IsFalse(dispatcher.Dispatch(trailing), L"a byte too many");
    Assert::IsFalse(dispatcher.Dispatch({}), L"empty");

    const Neuron::MessageDropCounts& drops = dispatcher.Drops();
    ExpectEqual(1, static_cast<std::int64_t>(drops.unknown), L"unknown");
    ExpectEqual(1, static_cast<std::int64_t>(drops.wrongVersion), L"wrong version");
    ExpectEqual(1, static_cast<std::int64_t>(drops.oversized), L"oversized");
    ExpectEqual(3, static_cast<std::int64_t>(drops.malformed), L"malformed");
    ExpectEqual(1, static_cast<std::int64_t>(delivered.size()), L"no malformed message reached the handler");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp090RandomInputIsCountedAndNeverFatal)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp090RandomInputIsCountedAndNeverFatal)
  {
    // The first form of the parser fuzzing of ADR-088 item 5: random buffers and single-bit mutations of a valid
    // message, each either delivered whole or dropped with a reason.
    Neuron::MessageDispatcher dispatcher;
    std::uint64_t delivered = 0;
    dispatcher.Register<TestMessage>(
      [&delivered](const TestMessage& _message)
      { delivered += _message.count <= TestMessage::MAX_COUNT && Neuron::IsWellFormedUtf8(_message.name) ? 1 : 1000000; });
    TestRandom random(7);
    const std::vector<std::byte> valid = Neuron::EncodeMessage(TestMessage{99, 2047, false, "abc"});
    constexpr std::uint64_t INPUTS = 100'000;
    for (std::uint64_t input = 0; input < INPUTS; ++input)
    {
      std::vector<std::byte> bytes;
      if (input % 2 == 0)
      {
        bytes.resize(random() % 80);
        for (std::byte& byte : bytes)
        {
          byte = static_cast<std::byte>(random() & 0xFF);
        }
      }
      else
      {
        bytes = valid;
        const std::size_t bit = random() % (bytes.size() * 8);
        bytes[bit / 8] ^= static_cast<std::byte>(1u << (bit % 8));
      }
      static_cast<void>(dispatcher.Dispatch(bytes));
    }
    Assert::IsTrue(delivered < 1000000, L"no delivered message broke its own bounds");
    ExpectEqual(static_cast<std::int64_t>(INPUTS), static_cast<std::int64_t>(delivered + dispatcher.Drops().Total()),
                L"every input was delivered or counted");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp090StreamFramesSurviveAnySplit)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp090StreamFramesSurviveAnySplit)
  {
    TestRandom random(11);
    std::vector<std::vector<std::byte>> messages;
    std::vector<std::byte> stream;
    for (std::size_t index = 0; index < 60; ++index)
    {
      std::vector<std::byte> message(1 + random() % (index % 3 == 0 ? 5000 : 100));
      for (std::byte& byte : message)
      {
        byte = static_cast<std::byte>(random() & 0xFF);
      }
      Neuron::AppendFrame(stream, message);
      messages.push_back(std::move(message));
    }
    for (const std::size_t largestPiece : {std::size_t{1}, std::size_t{7}, std::size_t{4096}})
    {
      Neuron::StreamFrameReader reader(Neuron::MESSAGE_MAX_BYTES);
      std::size_t next = 0;
      for (std::size_t offset = 0; offset < stream.size();)
      {
        const std::size_t piece = std::min(stream.size() - offset, 1 + random() % largestPiece);
        reader.Feed(std::span(stream).subspan(offset, piece));
        offset += piece;
        while (const std::optional<std::vector<std::byte>> message = reader.Next())
        {
          Assert::IsTrue(next < messages.size() && *message == messages[next], L"each message arrives whole and in order");
          ++next;
        }
        Assert::IsTrue(reader.PendingBytes() <= 5000 + Neuron::FRAME_LENGTH_MAX_BYTES + largestPiece, L"the buffer stays bounded");
      }
      ExpectEqual(static_cast<std::int64_t>(messages.size()), static_cast<std::int64_t>(next), L"every message");
      Assert::IsFalse(reader.Corrupt(), L"a well-formed stream");
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp090BadFrameLengthStopsTheStream)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp090BadFrameLengthStopsTheStream)
  {
    const auto corrupts = [](const std::vector<std::byte>& _stream)
    {
      Neuron::StreamFrameReader reader(1000);
      reader.Feed(_stream);
      return !reader.Next().has_value() && reader.Corrupt();
    };
    Assert::IsTrue(corrupts(Bytes({0x00})), L"a length of zero");
    Assert::IsTrue(corrupts(Bytes({0xE9, 0x07})), L"1001, above the limit");
    Assert::IsTrue(corrupts(Bytes({0x81, 0x00})), L"a length that is not minimal");
    Assert::IsTrue(corrupts(Bytes({0x80, 0x80, 0x80, 0x01})), L"a length of four bytes");
    Neuron::StreamFrameReader waiting(1000);
    waiting.Feed(Bytes({0x03, 0x01}));
    Assert::IsTrue(!waiting.Next().has_value() && !waiting.Corrupt(), L"a message not yet whole waits");
  }
};

} // namespace NeuronCoreTests
