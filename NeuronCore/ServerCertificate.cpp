#include "pch.h"

#include <wincrypt.h>
#include <ncrypt.h>

#include "ServerCertificate.h"
#include "Sha256.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <format>
#include <span>
#include <utility>
#include <vector>

#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "ncrypt.lib")

namespace Neuron
{

namespace
{

// A certificate within this many days of its end is made again.
constexpr std::uint64_t RENEW_DAYS = 1;
// A new certificate is valid for this many days.
constexpr std::uint64_t VALID_DAYS = 365;
constexpr std::uint64_t FILETIME_TICKS_PER_DAY = std::uint64_t{864'000'000'000};

std::uint64_t TicksOf(const FILETIME& _time) noexcept
{
  return (std::uint64_t{_time.dwHighDateTime} << 32) | _time.dwLowDateTime;
}

FILETIME FileTimeOf(std::uint64_t _ticks) noexcept
{
  return FILETIME{static_cast<DWORD>(_ticks & 0xFFFF'FFFF), static_cast<DWORD>(_ticks >> 32)};
}

std::uint64_t NowTicks() noexcept
{
  FILETIME now{};
  GetSystemTimeAsFileTime(&now);
  return TicksOf(now);
}

std::string Failure(std::string_view _step, std::uint32_t _status)
{
  return std::format("{} failed with {:#010x}", _step, _status);
}

// The user's "My" store, closed when the guard ends.
class UserStore
{
public:
  UserStore() noexcept
    : m_store(CertOpenStore(CERT_STORE_PROV_SYSTEM_W, 0, 0, CERT_SYSTEM_STORE_CURRENT_USER, L"MY"))
  {
  }
  UserStore(const UserStore&) = delete;
  UserStore& operator=(const UserStore&) = delete;
  ~UserStore()
  {
    if (m_store != nullptr)
    {
      static_cast<void>(CertCloseStore(m_store, 0));
    }
  }
  [[nodiscard]] HCERTSTORE Get() const noexcept
  {
    return m_store;
  }

private:
  HCERTSTORE m_store;
};

// A CNG object, freed when the guard ends.
class NcryptObject
{
public:
  NcryptObject() noexcept = default;
  NcryptObject(const NcryptObject&) = delete;
  NcryptObject& operator=(const NcryptObject&) = delete;
  ~NcryptObject()
  {
    if (m_handle != 0)
    {
      static_cast<void>(NCryptFreeObject(m_handle));
    }
  }
  [[nodiscard]] NCRYPT_HANDLE* Put() noexcept
  {
    return &m_handle;
  }
  [[nodiscard]] NCRYPT_HANDLE Get() const noexcept
  {
    return m_handle;
  }

private:
  NCRYPT_HANDLE m_handle = 0;
};

// The stored certificate of subject CN=_name with a private key that is valid for longer than RENEW_DAYS, or null.
PCCERT_CONTEXT FindValid(HCERTSTORE _store, const std::wstring& _subject)
{
  const std::uint64_t renewAfter = NowTicks() + RENEW_DAYS * FILETIME_TICKS_PER_DAY;
  PCCERT_CONTEXT found = nullptr;
  while ((found = CertFindCertificateInStore(_store, X509_ASN_ENCODING, 0, CERT_FIND_SUBJECT_STR_W, _subject.c_str(), found)) != nullptr)
  {
    DWORD size = 0;
    const bool hasKey = CertGetCertificateContextProperty(found, CERT_KEY_PROV_INFO_PROP_ID, nullptr, &size) != FALSE;
    if (hasKey && TicksOf(found->pCertInfo->NotAfter) > renewAfter)
    {
      return found; // the caller's to free, which ends the search
    }
  }
  return nullptr;
}

// A new certificate of CN=_name signed by the persisted key _name, made when missing, added to _store; or why not.
std::expected<PCCERT_CONTEXT, std::string> Create(HCERTSTORE _store, const std::wstring& _name)
{
  NcryptObject provider;
  // NCRYPT_HANDLE, NCRYPT_PROV_HANDLE and NCRYPT_KEY_HANDLE are one type, so one guard frees each.
  if (const SECURITY_STATUS status = NCryptOpenStorageProvider(provider.Put(), MS_KEY_STORAGE_PROVIDER, 0); status != ERROR_SUCCESS)
  {
    return std::unexpected(Failure("NCryptOpenStorageProvider", static_cast<std::uint32_t>(status)));
  }
  NcryptObject key;
  if (NCryptOpenKey(provider.Get(), key.Put(), _name.c_str(), 0, 0) != ERROR_SUCCESS)
  {
    if (const SECURITY_STATUS status =
          NCryptCreatePersistedKey(provider.Get(), key.Put(), BCRYPT_ECDSA_P256_ALGORITHM, _name.c_str(), 0, NCRYPT_OVERWRITE_KEY_FLAG);
        status != ERROR_SUCCESS)
    {
      return std::unexpected(Failure("NCryptCreatePersistedKey", static_cast<std::uint32_t>(status)));
    }
    if (const SECURITY_STATUS status = NCryptFinalizeKey(key.Get(), 0); status != ERROR_SUCCESS)
    {
      return std::unexpected(Failure("NCryptFinalizeKey", static_cast<std::uint32_t>(status)));
    }
  }

  const std::wstring subject = L"CN=" + _name;
  DWORD subjectSize = 0;
  if (CertStrToNameW(X509_ASN_ENCODING, subject.c_str(), CERT_X500_NAME_STR, nullptr, nullptr, &subjectSize, nullptr) == FALSE)
  {
    return std::unexpected(Failure("CertStrToNameW", GetLastError()));
  }
  std::vector<BYTE> subjectBytes(subjectSize);
  if (CertStrToNameW(X509_ASN_ENCODING, subject.c_str(), CERT_X500_NAME_STR, nullptr, subjectBytes.data(), &subjectSize, nullptr) == FALSE)
  {
    return std::unexpected(Failure("CertStrToNameW", GetLastError()));
  }
  CERT_NAME_BLOB subjectBlob{subjectSize, subjectBytes.data()};

  // The key's provider, so that the certificate names its private key (CERT_KEY_PROV_INFO_PROP_ID).
  std::wstring container = _name;
  std::wstring providerName = MS_KEY_STORAGE_PROVIDER;
  CRYPT_KEY_PROV_INFO providerInfo{};
  providerInfo.pwszContainerName = container.data();
  providerInfo.pwszProvName = providerName.data();

  std::array<char, sizeof(szOID_ECDSA_SHA256)> signatureOid{};
  std::ranges::copy(std::string_view(szOID_ECDSA_SHA256), signatureOid.begin());
  CRYPT_ALGORITHM_IDENTIFIER signature{};
  signature.pszObjId = signatureOid.data();

  // Server authentication, the one use of the certificate.
  std::array<char, sizeof(szOID_PKIX_KP_SERVER_AUTH)> serverAuth{};
  std::ranges::copy(std::string_view(szOID_PKIX_KP_SERVER_AUTH), serverAuth.begin());
  std::array<LPSTR, 1> usages{serverAuth.data()};
  CERT_ENHKEY_USAGE usage{static_cast<DWORD>(usages.size()), usages.data()};
  DWORD usageSize = 0;
  if (CryptEncodeObjectEx(X509_ASN_ENCODING, X509_ENHANCED_KEY_USAGE, &usage, 0, nullptr, nullptr, &usageSize) == FALSE)
  {
    return std::unexpected(Failure("CryptEncodeObjectEx", GetLastError()));
  }
  std::vector<BYTE> usageBytes(usageSize);
  if (CryptEncodeObjectEx(X509_ASN_ENCODING, X509_ENHANCED_KEY_USAGE, &usage, 0, nullptr, usageBytes.data(), &usageSize) == FALSE)
  {
    return std::unexpected(Failure("CryptEncodeObjectEx", GetLastError()));
  }
  std::array<char, sizeof(szOID_ENHANCED_KEY_USAGE)> usageOid{};
  std::ranges::copy(std::string_view(szOID_ENHANCED_KEY_USAGE), usageOid.begin());
  CERT_EXTENSION extension{usageOid.data(), FALSE, {usageSize, usageBytes.data()}};
  CERT_EXTENSIONS extensions{1, &extension};

  // From now for VALID_DAYS, computed in file time, so that no leap day makes an invalid end date.
  const std::uint64_t now = NowTicks();
  const FILETIME startFile = FileTimeOf(now);
  const FILETIME endFile = FileTimeOf(now + VALID_DAYS * FILETIME_TICKS_PER_DAY);
  SYSTEMTIME start{};
  SYSTEMTIME end{};
  if (FileTimeToSystemTime(&startFile, &start) == FALSE || FileTimeToSystemTime(&endFile, &end) == FALSE)
  {
    return std::unexpected(Failure("FileTimeToSystemTime", GetLastError()));
  }

  PCCERT_CONTEXT made = CertCreateSelfSignCertificate(key.Get(), &subjectBlob, 0, &providerInfo, &signature, &start, &end, &extensions);
  if (made == nullptr)
  {
    return std::unexpected(Failure("CertCreateSelfSignCertificate", GetLastError()));
  }
  PCCERT_CONTEXT stored = nullptr;
  const BOOL added = CertAddCertificateContextToStore(_store, made, CERT_STORE_ADD_REPLACE_EXISTING, &stored);
  const DWORD addError = GetLastError();
  static_cast<void>(CertFreeCertificateContext(made));
  if (added == FALSE)
  {
    return std::unexpected(Failure("CertAddCertificateContextToStore", addError));
  }
  return stored;
}

} // namespace

std::expected<ServerCertificate, std::string> ServerCertificate::OpenOrCreate(std::wstring_view _name)
{
  const UserStore store;
  if (store.Get() == nullptr)
  {
    return std::unexpected(Failure("CertOpenStore", GetLastError()));
  }
  const std::wstring name(_name);
  PCCERT_CONTEXT context = FindValid(store.Get(), name);
  if (context == nullptr)
  {
    std::expected<PCCERT_CONTEXT, std::string> created = Create(store.Get(), name);
    if (!created)
    {
      return std::unexpected(std::move(created.error()));
    }
    context = *created;
  }
  return ServerCertificate(context, CertificateFingerprint(context));
}

ServerCertificate::ServerCertificate(const void* _context, const Sha256Digest& _fingerprint) noexcept
  : m_context(_context),
    m_fingerprint(_fingerprint)
{
}

ServerCertificate::ServerCertificate(ServerCertificate&& _other) noexcept
  : m_context(std::exchange(_other.m_context, nullptr)),
    m_fingerprint(_other.m_fingerprint)
{
}

ServerCertificate& ServerCertificate::operator=(ServerCertificate&& _other) noexcept
{
  if (this != &_other)
  {
    if (m_context != nullptr)
    {
      static_cast<void>(CertFreeCertificateContext(static_cast<PCCERT_CONTEXT>(m_context)));
    }
    m_context = std::exchange(_other.m_context, nullptr);
    m_fingerprint = _other.m_fingerprint;
  }
  return *this;
}

ServerCertificate::~ServerCertificate()
{
  if (m_context != nullptr)
  {
    static_cast<void>(CertFreeCertificateContext(static_cast<PCCERT_CONTEXT>(m_context)));
  }
}

Sha256Digest CertificateFingerprint(const void* _context)
{
  const auto* context = static_cast<PCCERT_CONTEXT>(_context);
  return Sha256Of(std::as_bytes(std::span(context->pbCertEncoded, context->cbCertEncoded)));
}

} // namespace Neuron
