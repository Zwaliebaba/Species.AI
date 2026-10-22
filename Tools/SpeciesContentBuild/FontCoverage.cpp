#include "pch.h"

#include "WindowsHeaders.h"

#include <dwrite.h>
#include <wrl/client.h>

#include "FontCoverage.h"
#include "TextEncoding.h"

#include <format>

// ADR-147: DirectWrite, the operating system's, which the client draws text with.
#pragma comment(lib, "dwrite.lib")

namespace ContentBuild
{

struct FontCoverage::Fonts
{
  std::vector<Microsoft::WRL::ComPtr<IDWriteFont>> fonts; // each family's regular face, in the chain's order
};

std::expected<FontCoverage, std::string> FontCoverage::Open(std::span<const std::string_view> _families)
{
  Microsoft::WRL::ComPtr<IDWriteFactory> factory;
  Microsoft::WRL::ComPtr<IDWriteFontCollection> system;
  const HRESULT created =
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(factory.GetAddressOf()));
  if (FAILED(created) || FAILED(factory->GetSystemFontCollection(&system, FALSE)))
  {
    return std::unexpected(std::format("DirectWrite could not be opened (0x{:08X})", static_cast<std::uint32_t>(created)));
  }
  auto fonts = std::make_shared<Fonts>();
  FontCoverage coverage;
  for (const std::string_view family : _families)
  {
    UINT32 index = 0;
    BOOL exists = FALSE;
    Microsoft::WRL::ComPtr<IDWriteFontFamily> found;
    Microsoft::WRL::ComPtr<IDWriteFont> font;
    if (FAILED(system->FindFamilyName(Neuron::ToUtf16(family).c_str(), &index, &exists)) || exists == FALSE ||
        FAILED(system->GetFontFamily(index, &found)) ||
        FAILED(found->GetFirstMatchingFont(DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STRETCH_NORMAL, DWRITE_FONT_STYLE_NORMAL, &font)))
    {
      coverage.m_missing.emplace_back(family);
      continue;
    }
    fonts->fonts.push_back(std::move(font));
  }
  if (fonts->fonts.empty())
  {
    return std::unexpected(std::format("this machine holds no family of the chain, {} first", _families.empty() ? "" : _families.front()));
  }
  coverage.m_fonts = std::move(fonts);
  return coverage;
}

bool FontCoverage::Covers(std::uint32_t _codePoint) const
{
  for (const Microsoft::WRL::ComPtr<IDWriteFont>& font : m_fonts->fonts)
  {
    BOOL has = FALSE;
    if (SUCCEEDED(font->HasCharacter(_codePoint, &has)) && has != FALSE)
    {
      return true;
    }
  }
  return false;
}

} // namespace ContentBuild
