#pragma once

#include "Sha256.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string_view>

namespace Species
{

// ADR-187: the names that tie an always-on world's headless server to its client on one computer. One server runs at a
// time (O109), holding ALWAYS_ON_LOCK; ALWAYS_ON_STOP asks it to stop as closing its console does.
inline constexpr std::wstring_view ALWAYS_ON_LOCK = L"Species.AlwaysOnWorld";
inline constexpr std::wstring_view ALWAYS_ON_STOP = L"Species.AlwaysOnWorld.Stop";
// The option that starts the headless server on an always-on world's folder (StartupOptions::alwaysOnWorld).
inline constexpr std::wstring_view ALWAYS_ON_WORLD_OPTION = L"--always-on-world";
// The headless server's program, beside the client's.
inline constexpr std::wstring_view SERVER_PROGRAM = L"SpeciesServer.exe";
// The name of the headless server's certificate in the user's store (ADR-185).
inline constexpr std::wstring_view SERVER_CERTIFICATE_NAME = L"Species server";
// The listing a running server writes into its world's folder, and removes when it stops.
inline constexpr std::wstring_view SERVER_LISTING_FILE = L"server.txt";

/// ADR-187: how a client reaches the server of a world's folder: its port on the loopback interface, its certificate's
/// fingerprint, which the client pins (ADR-185), its build's stamp (ADR-186), and its process.
struct ServerListing
{
  std::uint16_t port = 0;
  Neuron::Sha256Digest fingerprint{};
  Neuron::Sha256Digest build{};
  std::uint32_t processId = 0;

  friend constexpr bool operator==(const ServerListing& _a, const ServerListing& _b) noexcept = default;
};

// Writes the listing into _folder through atomic replacement (PLT-089); false, logged, when it is not written.
bool WriteServerListing(const std::filesystem::path& _folder, const ServerListing& _listing);
// The listing of _folder; none when there is none or it does not read.
[[nodiscard]] std::optional<ServerListing> ReadServerListing(const std::filesystem::path& _folder);
// Removes the listing of _folder; one that is not there is removed already.
void RemoveServerListing(const std::filesystem::path& _folder);

} // namespace Species
