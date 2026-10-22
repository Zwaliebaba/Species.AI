#include "pch.h"

#include <icu.h>

#include "Log.h"
#include "MessageLocale.h"

#include <array>
#include <format>

// ADR-063: the in-box ICU of every supported Windows (icu.dll), its C API from the Windows SDK.
#pragma comment(lib, "icu.lib")

namespace Neuron
{

namespace
{

// Longer than any number or keyword ICU writes for a binary64 value, and its UTF-8 at four bytes a unit at most.
constexpr std::int32_t ICU_TEXT_UNITS = 128;
constexpr std::size_t ICU_TEXT_BYTES = 4 * static_cast<std::size_t>(ICU_TEXT_UNITS);

std::string Utf8Of(const UChar* _text, std::int32_t _units)
{
  std::array<char, ICU_TEXT_BYTES> buffer{};
  std::int32_t length = 0;
  UErrorCode status = U_ZERO_ERROR;
  u_strToUTF8(buffer.data(), static_cast<std::int32_t>(buffer.size()), &length, _text, _units, &status);
  return U_FAILURE(status) ? std::string() : std::string(buffer.data(), static_cast<std::size_t>(length));
}

} // namespace

struct MessageLocale::Handles
{
  UNumberFormat* integer = nullptr;
  UNumberFormat* decimal = nullptr;
  UPluralRules* cardinal = nullptr;
  UPluralRules* ordinal = nullptr;

  Handles() = default;
  Handles(const Handles&) = delete;
  Handles& operator=(const Handles&) = delete;

  ~Handles()
  {
    if (integer != nullptr)
    {
      unum_close(integer);
    }
    if (decimal != nullptr)
    {
      unum_close(decimal);
    }
    if (cardinal != nullptr)
    {
      uplrules_close(cardinal);
    }
    if (ordinal != nullptr)
    {
      uplrules_close(ordinal);
    }
  }
};

MessageLocale::MessageLocale(std::string_view _languageTag)
  : m_languageTag(_languageTag)
{
  // ICU names a locale by its own id, which the BCP 47 tag gives (en-US is en_US).
  std::array<char, ULOC_FULLNAME_CAPACITY> locale{};
  std::int32_t parsed = 0;
  UErrorCode status = U_ZERO_ERROR;
  uloc_forLanguageTag(m_languageTag.c_str(), locale.data(), static_cast<std::int32_t>(locale.size()), &parsed, &status);
  auto handles = std::make_unique<Handles>();
  if (U_SUCCESS(status))
  {
    handles->integer = unum_open(UNUM_DECIMAL, nullptr, 0, locale.data(), nullptr, &status);
  }
  if (U_SUCCESS(status))
  {
    unum_setAttribute(handles->integer, UNUM_MAX_FRACTION_DIGITS, 0);
    handles->decimal = unum_open(UNUM_DECIMAL, nullptr, 0, locale.data(), nullptr, &status);
  }
  if (U_SUCCESS(status))
  {
    handles->cardinal = uplrules_openForType(locale.data(), UPLURAL_TYPE_CARDINAL, &status);
  }
  if (U_SUCCESS(status))
  {
    handles->ordinal = uplrules_openForType(locale.data(), UPLURAL_TYPE_ORDINAL, &status);
  }
  if (U_FAILURE(status))
  {
    Log(LogCategory::Content, LogLevel::Error, "ICU cannot open the language {}: {}; numbers are plain and every plural is other",
        m_languageTag, u_errorName(status));
    return;
  }
  m_handles = std::move(handles);
}

MessageLocale::~MessageLocale() = default;

std::string MessageLocale::FormatInteger(std::int64_t _value) const
{
  if (m_handles == nullptr)
  {
    return std::format("{}", _value);
  }
  std::array<UChar, ICU_TEXT_UNITS> text{};
  UErrorCode status = U_ZERO_ERROR;
  const std::int32_t units =
    unum_formatInt64(m_handles->integer, _value, text.data(), static_cast<std::int32_t>(text.size()), nullptr, &status);
  return U_FAILURE(status) ? std::format("{}", _value) : Utf8Of(text.data(), units);
}

std::string MessageLocale::FormatDecimal(double _value) const
{
  if (m_handles == nullptr)
  {
    return std::format("{}", _value);
  }
  std::array<UChar, ICU_TEXT_UNITS> text{};
  UErrorCode status = U_ZERO_ERROR;
  const std::int32_t units =
    unum_formatDouble(m_handles->decimal, _value, text.data(), static_cast<std::int32_t>(text.size()), nullptr, &status);
  return U_FAILURE(status) ? std::format("{}", _value) : Utf8Of(text.data(), units);
}

std::string MessageLocale::PluralCategory(double _value, bool _ordinal) const
{
  if (m_handles == nullptr)
  {
    return "other";
  }
  std::array<UChar, ICU_TEXT_UNITS> keyword{};
  UErrorCode status = U_ZERO_ERROR;
  const std::int32_t units = uplrules_select(_ordinal ? m_handles->ordinal : m_handles->cardinal, _value, keyword.data(),
                                             static_cast<std::int32_t>(keyword.size()), &status);
  return U_FAILURE(status) ? std::string("other") : Utf8Of(keyword.data(), units);
}

} // namespace Neuron
