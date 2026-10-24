#include "pch.h"

#include "AlwaysOnServer.h"
#include "Files.h"
#include "Log.h"
#include "SettingsFile.h"
#include "TextEncoding.h"

#include <charconv>
#include <cstddef>
#include <span>
#include <string>
#include <system_error>
#include <vector>

namespace Species
{

namespace
{

// The listing's keys (DAT-196).
constexpr std::string_view PORT_KEY = "server.port";
constexpr std::string_view FINGERPRINT_KEY = "server.fingerprint";
constexpr std::string_view BUILD_KEY = "server.build";
constexpr std::string_view PROCESS_KEY = "server.process";
constexpr std::string_view HEX_DIGITS = "0123456789abcdef";

std::string HexOf(const Neuron::Sha256Digest& _digest)
{
  std::string text;
  for (const std::byte value : _digest)
  {
    text += HEX_DIGITS[static_cast<std::size_t>(value) >> 4];
    text += HEX_DIGITS[static_cast<std::size_t>(value) & 0x0F];
  }
  return text;
}

std::optional<Neuron::Sha256Digest> DigestOf(std::string_view _text)
{
  Neuron::Sha256Digest digest{};
  if (_text.size() != 2 * digest.size())
  {
    return std::nullopt;
  }
  for (std::size_t index = 0; index < digest.size(); ++index)
  {
    const std::size_t high = HEX_DIGITS.find(_text[2 * index]);
    const std::size_t low = HEX_DIGITS.find(_text[(2 * index) + 1]);
    if (high == std::string_view::npos || low == std::string_view::npos)
    {
      return std::nullopt;
    }
    digest[index] = static_cast<std::byte>((high << 4) | low);
  }
  return digest;
}

template <class T> std::optional<T> NumberOf(std::string_view _text)
{
  T value{};
  const auto [end, error] = std::from_chars(_text.data(), _text.data() + _text.size(), value);
  return error == std::errc{} && end == _text.data() + _text.size() ? std::optional<T>(value) : std::nullopt;
}

} // namespace

bool WriteServerListing(const std::filesystem::path& _folder, const ServerListing& _listing)
{
  const std::vector<Neuron::SettingsEntryText> entries{{std::string(PORT_KEY), std::to_string(_listing.port)},
                                                       {std::string(FINGERPRINT_KEY), HexOf(_listing.fingerprint)},
                                                       {std::string(BUILD_KEY), HexOf(_listing.build)},
                                                       {std::string(PROCESS_KEY), std::to_string(_listing.processId)}};
  const std::string text = Neuron::WriteSettingsText({}, entries);
  if (!Neuron::ReplaceFileAtomically(_folder / SERVER_LISTING_FILE, std::as_bytes(std::span(text))))
  {
    Neuron::Log(Neuron::LogCategory::Network, Neuron::LogLevel::Error, "the server's listing in {} could not be written",
                Neuron::ToUtf8(_folder.wstring()));
    return false;
  }
  return true;
}

std::optional<ServerListing> ReadServerListing(const std::filesystem::path& _folder)
{
  const auto bytes = Neuron::ReadWholeFile(_folder / SERVER_LISTING_FILE);
  if (!bytes)
  {
    return std::nullopt;
  }
  std::optional<std::uint16_t> port;
  std::optional<Neuron::Sha256Digest> fingerprint;
  std::optional<Neuron::Sha256Digest> build;
  std::optional<std::uint32_t> process;
  for (const Neuron::SettingsLine& line : Neuron::ReadSettingsLines(*bytes))
  {
    if (line.kind != Neuron::SettingsLine::Kind::Entry)
    {
      continue;
    }
    if (line.key == PORT_KEY)
    {
      port = NumberOf<std::uint16_t>(line.value);
    }
    else if (line.key == FINGERPRINT_KEY)
    {
      fingerprint = DigestOf(line.value);
    }
    else if (line.key == BUILD_KEY)
    {
      build = DigestOf(line.value);
    }
    else if (line.key == PROCESS_KEY)
    {
      process = NumberOf<std::uint32_t>(line.value);
    }
  }
  if (!port || *port == 0 || !fingerprint || !build || !process)
  {
    return std::nullopt;
  }
  return ServerListing{*port, *fingerprint, *build, *process};
}

void RemoveServerListing(const std::filesystem::path& _folder)
{
  // A listing left behind names a process that has ended, which a client reads as no server.
  if (!Neuron::RemoveFile(_folder / SERVER_LISTING_FILE))
  {
    Neuron::Log(Neuron::LogCategory::Network, Neuron::LogLevel::Warning, "the server's listing in {} could not be removed",
                Neuron::ToUtf8(_folder.wstring()));
  }
}

} // namespace Species
