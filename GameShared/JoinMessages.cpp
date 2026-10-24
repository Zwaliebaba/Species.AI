#include "pch.h"

#include "BitReader.h"
#include "BitWriter.h"
#include "Files.h"
#include "JoinMessages.h"

namespace Species
{

namespace
{

void WriteIdentity(Neuron::BitWriter& _writer, const JoinIdentity& _identity)
{
  _writer.WriteBits(_identity.protocol, 16);
  for (const Neuron::Sha256Digest* digest : {&_identity.build, &_identity.content})
  {
    for (const std::byte value : *digest)
    {
      _writer.WriteBits(static_cast<std::uint64_t>(value), 8);
    }
  }
}

JoinIdentity ReadIdentity(Neuron::BitReader& _reader)
{
  JoinIdentity identity;
  identity.protocol = static_cast<std::uint16_t>(_reader.ReadBits(16));
  for (Neuron::Sha256Digest* digest : {&identity.build, &identity.content})
  {
    for (std::byte& value : *digest)
    {
      value = static_cast<std::byte>(_reader.ReadBits(8));
    }
  }
  return identity;
}

} // namespace

JoinIdentity OwnJoinIdentity(const Neuron::Sha256Digest& _content)
{
  return JoinIdentity{GAME_PROTOCOL_VERSION, Neuron::FilesStamp(Neuron::ExecutableFolder(), BUILD_PROGRAMS), _content};
}

std::string_view JoinRefusalName(JoinRefusal _refusal) noexcept
{
  switch (_refusal)
  {
  case JoinRefusal::None:
    return "joined";
  case JoinRefusal::Protocol:
    return "another protocol";
  case JoinRefusal::Build:
    return "another build";
  case JoinRefusal::Content:
    return "other content";
  }
  return "unknown";
}

JoinRefusal JoinRefusalOf(const JoinIdentity& _server, const JoinIdentity& _client) noexcept
{
  if (_client.protocol != _server.protocol)
  {
    return JoinRefusal::Protocol;
  }
  if (_client.build != _server.build)
  {
    return JoinRefusal::Build;
  }
  return _client.content != _server.content ? JoinRefusal::Content : JoinRefusal::None;
}

void JoinCommand::Encode(Neuron::BitWriter& _writer) const
{
  WriteIdentity(_writer, identity);
}

void JoinCommand::Decode(Neuron::BitReader& _reader)
{
  identity = ReadIdentity(_reader);
}

void JoinAnswer::Encode(Neuron::BitWriter& _writer) const
{
  _writer.WriteBounded(static_cast<std::uint64_t>(refusal), JOIN_REFUSAL_MAX);
  WriteIdentity(_writer, server);
}

void JoinAnswer::Decode(Neuron::BitReader& _reader)
{
  const std::uint64_t value = _reader.ReadBounded(JOIN_REFUSAL_MAX);
  server = ReadIdentity(_reader);
  // A refusal this build does not know fails the reading.
  if (value > static_cast<std::uint64_t>(JoinRefusal::Content))
  {
    _reader.Fail();
    return;
  }
  refusal = static_cast<JoinRefusal>(value);
}

} // namespace Species
