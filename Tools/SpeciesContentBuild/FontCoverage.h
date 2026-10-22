#pragma once

#include <cstdint>
#include <expected>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ContentBuild
{

/// The characters that a chain of system font families draws (UI-199 as O92 amends it, ADR-147): a character is covered
/// when a family of the chain that this machine holds has a glyph for it, as the client's fallback draws it. The
/// families are read through DirectWrite, the operating system's.
class FontCoverage
{
public:
  // The chain _families in order; it fails when DirectWrite cannot be opened or the machine holds none of them.
  [[nodiscard]] static std::expected<FontCoverage, std::string> Open(std::span<const std::string_view> _families);

  [[nodiscard]] bool Covers(std::uint32_t _codePoint) const;
  // The families of the chain this machine does not hold, which the check passes over.
  [[nodiscard]] const std::vector<std::string>& Missing() const noexcept
  {
    return m_missing;
  }

private:
  struct Fonts; // DirectWrite's font of each family held

  std::shared_ptr<const Fonts> m_fonts;
  std::vector<std::string> m_missing;
};

} // namespace ContentBuild
