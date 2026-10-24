#include "pch.h"

#include "AlwaysOnServer.h"
#include "Files.h"
#include "TestSupport.h"

#include <cstddef>
#include <optional>
#include <span>
#include <string_view>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameSharedTests
{

TEST_CLASS(AlwaysOnServerTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp071AServerListingRoundTripsAndIsRemoved)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-071")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp071AServerListingRoundTripsAndIsRemoved)
  {
    // ADR-187: the listing names the port, the certificate's fingerprint, the build's stamp and the process, and reads
    // back whole; a folder without one, or with one that does not read, has none.
    const TemporaryFolder folder;
    Assert::IsFalse(Species::ReadServerListing(folder.Path()).has_value(), L"no listing yet");
    Species::ServerListing listing{51'234, {}, {}, 4'321};
    listing.fingerprint.fill(std::byte{0xA7});
    listing.build.fill(std::byte{0x0C});
    listing.build.back() = std::byte{0xF0};
    Assert::IsTrue(Species::WriteServerListing(folder.Path(), listing), L"written");
    const std::optional<Species::ServerListing> read = Species::ReadServerListing(folder.Path());
    Assert::IsTrue(read.has_value() && *read == listing, L"read back whole");
    const std::string_view damaged = "server.port = 51234\nserver.fingerprint = a7\nserver.build = 00\nserver.process = 1\n";
    Assert::IsTrue(
      Neuron::ReplaceFileAtomically(folder.Path() / Species::SERVER_LISTING_FILE, std::as_bytes(std::span(damaged))).has_value(),
      L"a damaged listing written");
    Assert::IsFalse(Species::ReadServerListing(folder.Path()).has_value(), L"a damaged listing does not read");
    Species::RemoveServerListing(folder.Path());
    Assert::IsFalse(Species::ReadServerListing(folder.Path()).has_value(), L"removed");
  }
};

} // namespace GameSharedTests
