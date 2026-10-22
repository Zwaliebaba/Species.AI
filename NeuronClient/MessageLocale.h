#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace Neuron
{

/// The number formats and plural rules of one language, from the in-box ICU (ADR-063, ADR-146): what a message's
/// numbers, plurals and select-ordinals need. When ICU cannot open the language, which the log reports, numbers are
/// written in plain digits and every number takes the category other. Every call is made from the frame thread.
class MessageLocale
{
public:
  // _languageTag is a BCP 47 tag (ADR-063), en-GB for the MVP.
  explicit MessageLocale(std::string_view _languageTag);
  ~MessageLocale();

  MessageLocale(const MessageLocale&) = delete;
  MessageLocale& operator=(const MessageLocale&) = delete;

  [[nodiscard]] const std::string& LanguageTag() const noexcept
  {
    return m_languageTag;
  }
  // The language's form of a whole number, with its grouping (1,500 in en-GB).
  [[nodiscard]] std::string FormatInteger(std::int64_t _value) const;
  // The language's form of a number, with its grouping and at most three fractional digits.
  [[nodiscard]] std::string FormatDecimal(double _value) const;
  // CLDR's plural category of a number, cardinal or ordinal: zero, one, two, few, many or other.
  [[nodiscard]] std::string PluralCategory(double _value, bool _ordinal) const;

private:
  struct Handles; // ICU's, opened once

  std::string m_languageTag;
  std::unique_ptr<Handles> m_handles;
};

} // namespace Neuron
