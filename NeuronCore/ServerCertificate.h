#pragma once

#include "Sha256.h"

#include <expected>
#include <string>
#include <string_view>

namespace Neuron
{

/// A server's TLS certificate (ADR-088 item 4, ADR-189): a self-signed ECDSA P-256 certificate whose subject is CN=_name,
/// for the server's own use only, its private key a persisted key of the user's software key storage provider named
/// _name, and the certificate kept in the user's "My" store, so that a later start uses it again. A client pins it by its
/// fingerprint, the SHA-256 of its DER encoding, which the server publishes beside the world it runs. Moved, never
/// copied; it releases its certificate context when it ends.
class ServerCertificate
{
public:
  // The certificate of _name that is valid for at least another day, or a new one made and stored; or why neither.
  [[nodiscard]] static std::expected<ServerCertificate, std::string> OpenOrCreate(std::wstring_view _name);

  ServerCertificate(ServerCertificate&& _other) noexcept;
  ServerCertificate& operator=(ServerCertificate&& _other) noexcept;
  ServerCertificate(const ServerCertificate&) = delete;
  ServerCertificate& operator=(const ServerCertificate&) = delete;
  ~ServerCertificate();

  // The system's certificate context (PCCERT_CONTEXT), which the transport hands to MsQuic's credential.
  [[nodiscard]] const void* Context() const noexcept
  {
    return m_context;
  }
  [[nodiscard]] const Sha256Digest& Fingerprint() const noexcept
  {
    return m_fingerprint;
  }

private:
  ServerCertificate(const void* _context, const Sha256Digest& _fingerprint) noexcept;

  const void* m_context = nullptr;
  Sha256Digest m_fingerprint{};
};

// The fingerprint of a system certificate context (PCCERT_CONTEXT): the SHA-256 of its DER encoding.
[[nodiscard]] Sha256Digest CertificateFingerprint(const void* _context);

} // namespace Neuron
