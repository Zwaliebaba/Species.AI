#include "pch.h"

#include <dwrite_2.h>
#include <wrl/client.h>

#include "Log.h"
#include "TextEncoding.h"
#include "TextSystem.h"

#include <algorithm>
#include <cmath>
#include <compare>
#include <map>
#include <optional>
#include <set>
#include <utility>

// ADR-147 (O92): DirectWrite, the operating system's, lays out, shapes and rasterizes every text.
#pragma comment(lib, "dwrite.lib")

namespace Neuron
{

namespace
{

template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

// The size a role's text format is made at; every layout sets its own.
constexpr float FORMAT_SIZE_PIXELS = 16.0f;
// The bound of a layout's box: a text is broken only at its line breaks (UI-201's wrapping arrives with T14's layout).
constexpr float LAYOUT_BOUND_PIXELS = 1.0e6f;
// The largest final size laid out; a glyph larger than a page is left out anyway.
constexpr float SIZE_MAX_PIXELS = 512.0f;
// The layouts kept: a text laid out again in its role at its size reuses its glyphs' slots, and one unused for this
// many frames is dropped, as are all of them past the bound.
constexpr std::uint64_t LAYOUT_KEEP_FRAMES = 120;
constexpr std::size_t LAYOUT_KEEP_MAX = 2048;
constexpr float SIXTEENTHS = 16.0f;

std::uint32_t SixteenthsOf(float _pixels) noexcept
{
  return static_cast<std::uint32_t>(std::lround(std::clamp(_pixels, 0.0f, SIZE_MAX_PIXELS) * SIXTEENTHS));
}

/// A glyph of a layout before it is placed: its face, its size, its index in the face and its pen position on the
/// baseline, in pixels from the layout's top-left corner.
struct RunGlyph
{
  IDWriteFontFace* face = nullptr; // held by the collector
  float emSize = 0.0f;
  std::uint16_t index = 0;
  float x = 0.0f;
  float y = 0.0f;
};

/// The renderer DirectWrite draws a layout into (ADR-147): it records each glyph's face and pen position and draws
/// nothing. It lives on the stack for one `Draw`, which holds no reference past its return, so its reference count is
/// fixed and owns nothing.
class GlyphRunCollector final : public IDWriteTextRenderer
{
public:
  GlyphRunCollector() = default;
  ~GlyphRunCollector() = default;

  GlyphRunCollector(const GlyphRunCollector&) = delete;
  GlyphRunCollector& operator=(const GlyphRunCollector&) = delete;

  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID _id, void** _object) noexcept override
  {
    if (_object == nullptr)
    {
      return E_POINTER;
    }
    if (_id == __uuidof(IUnknown) || _id == __uuidof(IDWritePixelSnapping) || _id == __uuidof(IDWriteTextRenderer))
    {
      *_object = static_cast<IDWriteTextRenderer*>(this);
      return S_OK;
    }
    *_object = nullptr;
    return E_NOINTERFACE;
  }
  ULONG STDMETHODCALLTYPE AddRef() noexcept override
  {
    return 1;
  }
  ULONG STDMETHODCALLTYPE Release() noexcept override
  {
    return 1;
  }

  // The baseline's origin is snapped to whole pixels, at one pixel a DIP, with no transform.
  HRESULT STDMETHODCALLTYPE IsPixelSnappingDisabled(void* /*_context*/, BOOL* _disabled) noexcept override
  {
    *_disabled = FALSE;
    return S_OK;
  }
  HRESULT STDMETHODCALLTYPE GetCurrentTransform(void* /*_context*/, DWRITE_MATRIX* _transform) noexcept override
  {
    *_transform = DWRITE_MATRIX{1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f};
    return S_OK;
  }
  HRESULT STDMETHODCALLTYPE GetPixelsPerDip(void* /*_context*/, FLOAT* _pixelsPerDip) noexcept override
  {
    *_pixelsPerDip = 1.0f;
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE DrawGlyphRun(void* /*_context*/, FLOAT _baselineX, FLOAT _baselineY, DWRITE_MEASURING_MODE /*_mode*/,
                                         const DWRITE_GLYPH_RUN* _run, const DWRITE_GLYPH_RUN_DESCRIPTION* /*_description*/,
                                         IUnknown* /*_effect*/) noexcept override
  {
    try
    {
      if (_run == nullptr || _run->fontFace == nullptr || _run->glyphIndices == nullptr)
      {
        return S_OK;
      }
      m_faces.emplace_back(_run->fontFace);
      // A right-to-left run's origin is its right end, and its glyphs advance to the left.
      const bool rightToLeft = (_run->bidiLevel & 1) != 0;
      float pen = _baselineX;
      for (UINT32 n = 0; n < _run->glyphCount; ++n)
      {
        const float advance = _run->glyphAdvances != nullptr ? _run->glyphAdvances[n] : 0.0f;
        if (rightToLeft)
        {
          pen -= advance;
        }
        float x = pen;
        float y = _baselineY;
        if (_run->glyphOffsets != nullptr)
        {
          x += rightToLeft ? -_run->glyphOffsets[n].advanceOffset : _run->glyphOffsets[n].advanceOffset;
          y -= _run->glyphOffsets[n].ascenderOffset;
        }
        m_glyphs.push_back(RunGlyph{_run->fontFace, _run->fontEmSize, _run->glyphIndices[n], x, y});
        if (!rightToLeft)
        {
          pen += advance;
        }
      }
      return S_OK;
    }
    catch (...)
    {
      return E_OUTOFMEMORY;
    }
  }
  // No text of the UI's is underlined or struck through, and none holds an inline object yet (UI-224 waits for T15).
  HRESULT STDMETHODCALLTYPE DrawUnderline(void* /*_context*/, FLOAT /*_x*/, FLOAT /*_y*/, const DWRITE_UNDERLINE* /*_underline*/,
                                          IUnknown* /*_effect*/) noexcept override
  {
    return S_OK;
  }
  HRESULT STDMETHODCALLTYPE DrawStrikethrough(void* /*_context*/, FLOAT /*_x*/, FLOAT /*_y*/, const DWRITE_STRIKETHROUGH* /*_strike*/,
                                              IUnknown* /*_effect*/) noexcept override
  {
    return S_OK;
  }
  HRESULT STDMETHODCALLTYPE DrawInlineObject(void* /*_context*/, FLOAT /*_x*/, FLOAT /*_y*/, IDWriteInlineObject* /*_object*/,
                                             BOOL /*_sideways*/, BOOL /*_rightToLeft*/, IUnknown* /*_effect*/) noexcept override
  {
    return S_OK;
  }

  [[nodiscard]] const std::vector<RunGlyph>& Glyphs() const noexcept
  {
    return m_glyphs;
  }

private:
  std::vector<RunGlyph> m_glyphs;
  std::vector<ComPtr<IDWriteFontFace>> m_faces; // each run's face, held while its glyphs are placed
};

} // namespace

struct TextSystem::Implementation
{
  /// A layout kept by its role, final size and text.
  struct LayoutKey
  {
    FontRole role = FontRole::Body;
    std::uint32_t sizeSixteenths = 0;
    std::string text;

    friend auto operator<=>(const LayoutKey&, const LayoutKey&) = default;
  };
  struct KeptLayout
  {
    TextLine line;
    std::uint64_t lastUseFrame = 0;
    std::uint64_t atlasClears = 0; // the atlas's clears when it was laid out: a later clear may have taken its slots
  };

  ComPtr<IDWriteFactory2> factory;
  std::array<ComPtr<IDWriteTextFormat>, FONT_ROLE_COUNT> formats;
  std::array<std::string, FONT_ROLE_COUNT> faceFamilies;
  std::vector<ComPtr<IDWriteFontFace>> faces; // a face's index is its id in the glyph keys
  std::map<LayoutKey, KeptLayout> layouts;
  std::set<GlyphKey> tooLarge; // logged once each
  std::uint32_t subpixelSteps = 4;
  std::uint64_t frame = 0;
  std::uint64_t rasterized = 0;

  std::uint32_t FaceId(IDWriteFontFace* _face)
  {
    const auto found = std::ranges::find_if(faces, [_face](const ComPtr<IDWriteFontFace>& _held) { return _held.Get() == _face; });
    if (found != faces.end())
    {
      return static_cast<std::uint32_t>(found - faces.begin());
    }
    faces.emplace_back(_face);
    return static_cast<std::uint32_t>(faces.size() - 1);
  }

  // UI-198: the glyph's coverage in grayscale at its size, its pen at _offsetX of a pixel past a whole pixel, cached in
  // _atlas. A glyph that draws nothing takes an empty slot.
  std::optional<GlyphSlot> Rasterize(GlyphAtlas& _atlas, const GlyphKey& _key, IDWriteFontFace* _face, float _emSize, std::uint16_t _index,
                                     float _offsetX)
  {
    const FLOAT advance = 0.0f;
    const DWRITE_GLYPH_OFFSET offset{};
    DWRITE_GLYPH_RUN run{};
    run.fontFace = _face;
    run.fontEmSize = _emSize;
    run.glyphCount = 1;
    run.glyphIndices = &_index;
    run.glyphAdvances = &advance;
    run.glyphOffsets = &offset;
    run.isSideways = FALSE;
    run.bidiLevel = 0;
    ComPtr<IDWriteGlyphRunAnalysis> analysis;
    if (FAILED(factory->CreateGlyphRunAnalysis(&run, nullptr, DWRITE_RENDERING_MODE_NATURAL_SYMMETRIC, DWRITE_MEASURING_MODE_NATURAL,
                                               DWRITE_GRID_FIT_MODE_ENABLED, DWRITE_TEXT_ANTIALIAS_MODE_GRAYSCALE, _offsetX, 0.0f,
                                               &analysis)))
    {
      return std::nullopt;
    }
    // O92: grayscale coverage, one byte a pixel, which the grayscale mode gives as the 1×1 texture; a ClearType
    // texture, should the analysis give that instead, is averaged across its three subpixels.
    DWRITE_TEXTURE_TYPE type = DWRITE_TEXTURE_ALIASED_1x1;
    RECT bounds{};
    if (FAILED(analysis->GetAlphaTextureBounds(type, &bounds)))
    {
      return std::nullopt;
    }
    if (bounds.right <= bounds.left || bounds.bottom <= bounds.top)
    {
      type = DWRITE_TEXTURE_CLEARTYPE_3x1;
      if (FAILED(analysis->GetAlphaTextureBounds(type, &bounds)))
      {
        return std::nullopt;
      }
    }
    if (bounds.right <= bounds.left || bounds.bottom <= bounds.top)
    {
      return _atlas.Insert(_key, 0, 0, 0, 0, {});
    }
    const auto width = static_cast<std::uint32_t>(bounds.right - bounds.left);
    const auto height = static_cast<std::uint32_t>(bounds.bottom - bounds.top);
    const std::size_t channels = type == DWRITE_TEXTURE_CLEARTYPE_3x1 ? 3 : 1;
    std::vector<std::uint8_t> texels(std::size_t{width} * height * channels);
    if (FAILED(analysis->CreateAlphaTexture(type, &bounds, texels.data(), static_cast<UINT32>(texels.size()))))
    {
      return std::nullopt;
    }
    if (channels == 3)
    {
      for (std::size_t n = 0; n < std::size_t{width} * height; ++n)
      {
        texels[n] = static_cast<std::uint8_t>((unsigned{texels[3 * n]} + texels[3 * n + 1] + texels[3 * n + 2] + 1) / 3);
      }
      texels.resize(std::size_t{width} * height);
    }
    ++rasterized;
    return _atlas.Insert(_key, width, height, bounds.left, bounds.top, texels);
  }
};

TextSystem::TextSystem(const Desc& _desc)
  : m_atlas(std::make_shared<GlyphAtlas>(_desc.atlas)),
    m_implementation(std::make_unique<Implementation>())
{
  Implementation& text = *m_implementation;
  text.subpixelSteps = std::clamp(_desc.subpixelSteps, 1u, 16u);
  ComPtr<IDWriteFactory2> factory;
  const HRESULT created =
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory2), reinterpret_cast<IUnknown**>(factory.GetAddressOf()));
  ComPtr<IDWriteFontCollection> system;
  if (FAILED(created) || FAILED(factory->GetSystemFontCollection(&system, FALSE)))
  {
    Log(LogCategory::Ui, LogLevel::Error, "DirectWrite could not be opened (0x{:08X}): no text is drawn",
        static_cast<std::uint32_t>(created));
    return;
  }
  const std::wstring language = ToUtf16(_desc.languageTag);
  for (std::size_t role = 0; role < FONT_ROLE_COUNT; ++role)
  {
    const std::vector<std::string>& chain = _desc.families[role];
    if (chain.empty())
    {
      Log(LogCategory::Ui, LogLevel::Error, "font role {} names no family: its text is not drawn", role);
      continue;
    }
    // UI-234: the role's face is the first family of its chain that the system holds.
    std::size_t face = chain.size();
    for (std::size_t n = 0; n < chain.size() && face == chain.size(); ++n)
    {
      UINT32 index = 0;
      BOOL exists = FALSE;
      if (SUCCEEDED(system->FindFamilyName(ToUtf16(chain[n]).c_str(), &index, &exists)) && exists != FALSE)
      {
        face = n;
      }
    }
    if (face == chain.size())
    {
      Log(LogCategory::Ui, LogLevel::Error, "no family of font role {} is installed, {} first: DirectWrite's default draws it", role,
          chain.front());
      face = 0;
    }
    else if (face > 0)
    {
      Log(LogCategory::Ui, LogLevel::Warning, "font role {}: {} is not installed, so {} draws it", role, chain.front(), chain[face]);
    }
    text.faceFamilies[role] = chain[face];
    ComPtr<IDWriteTextFormat> format;
    if (FAILED(factory->CreateTextFormat(ToUtf16(chain[face]).c_str(), nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                                         DWRITE_FONT_STRETCH_NORMAL, FORMAT_SIZE_PIXELS, language.c_str(), &format)))
    {
      Log(LogCategory::Ui, LogLevel::Error, "font role {}: the text format of {} could not be made", role, chain[face]);
      continue;
    }
    format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    // UI-199: a character the face lacks is drawn from the rest of the chain and from no other system font, so that
    // what the content build's coverage check passes is what draws.
    std::vector<std::wstring> rest;
    for (std::size_t n = 0; n < chain.size(); ++n)
    {
      if (n != face)
      {
        rest.push_back(ToUtf16(chain[n]));
      }
    }
    std::vector<const WCHAR*> names;
    names.reserve(rest.size());
    for (const std::wstring& name : rest)
    {
      names.push_back(name.c_str());
    }
    ComPtr<IDWriteFontFallbackBuilder> builder;
    ComPtr<IDWriteFontFallback> fallback;
    ComPtr<IDWriteTextFormat1> format1;
    const DWRITE_UNICODE_RANGE everything{0x0000, 0x10FFFF};
    if (FAILED(factory->CreateFontFallbackBuilder(&builder)) ||
        (!names.empty() && FAILED(builder->AddMapping(&everything, 1, names.data(), static_cast<UINT32>(names.size())))) ||
        FAILED(builder->CreateFontFallback(&fallback)) || FAILED(format.As(&format1)) || FAILED(format1->SetFontFallback(fallback.Get())))
    {
      Log(LogCategory::Ui, LogLevel::Warning, "font role {}: its fallback could not be set, so the system's applies", role);
    }
    text.formats[role] = std::move(format);
  }
  text.factory = std::move(factory);
}

TextSystem::~TextSystem() = default;

bool TextSystem::Available() const noexcept
{
  return m_implementation->factory != nullptr;
}

void TextSystem::BeginFrame(std::uint64_t _frame)
{
  Implementation& text = *m_implementation;
  text.frame = _frame;
  m_atlas->BeginFrame(_frame);
  if (text.layouts.size() > LAYOUT_KEEP_MAX)
  {
    text.layouts.clear();
  }
  std::erase_if(text.layouts, [&](const auto& _entry) { return _entry.second.lastUseFrame + LAYOUT_KEEP_FRAMES < _frame; });
}

TextLine TextSystem::Layout(std::string_view _utf8, FontRole _role, float _sizePixels)
{
  Implementation& text = *m_implementation;
  const auto role = static_cast<std::size_t>(_role);
  if (text.factory == nullptr || role >= FONT_ROLE_COUNT || text.formats[role] == nullptr || !(_sizePixels > 0.0f))
  {
    return {};
  }
  if (_utf8.empty())
  {
    return Metrics(_role, _sizePixels);
  }
  const std::uint32_t sizeSixteenths = SixteenthsOf(_sizePixels);
  Implementation::LayoutKey key{_role, sizeSixteenths, std::string(_utf8)};
  if (const auto kept = text.layouts.find(key); kept != text.layouts.end() && kept->second.atlasClears == m_atlas->Clears())
  {
    kept->second.lastUseFrame = text.frame;
    for (const PlacedGlyph& glyph : kept->second.line.glyphs)
    {
      m_atlas->MarkUsed(glyph.page);
    }
    return kept->second.line;
  }

  const std::wstring wide = ToUtf16(_utf8);
  ComPtr<IDWriteTextLayout> layout;
  if (FAILED(text.factory->CreateTextLayout(wide.data(), static_cast<UINT32>(wide.size()), text.formats[role].Get(), LAYOUT_BOUND_PIXELS,
                                            LAYOUT_BOUND_PIXELS, &layout)))
  {
    return {};
  }
  const float size = static_cast<float>(sizeSixteenths) / SIXTEENTHS;
  layout->SetFontSize(size, DWRITE_TEXT_RANGE{0, static_cast<UINT32>(wide.size())});
  DWRITE_TEXT_METRICS metrics{};
  UINT32 lineCount = 0;
  layout->GetLineMetrics(nullptr, 0, &lineCount);
  std::vector<DWRITE_LINE_METRICS> lines(std::max<UINT32>(lineCount, 1));
  GlyphRunCollector collector;
  if (FAILED(layout->GetMetrics(&metrics)) || FAILED(layout->GetLineMetrics(lines.data(), static_cast<UINT32>(lines.size()), &lineCount)) ||
      FAILED(layout->Draw(nullptr, &collector, 0.0f, 0.0f)))
  {
    return {};
  }

  TextLine line;
  line.widthPixels = metrics.widthIncludingTrailingWhitespace;
  line.heightPixels = metrics.height;
  line.baselinePixels = std::round(lines.front().baseline);
  const auto steps = static_cast<float>(text.subpixelSteps);
  const auto side = static_cast<float>(m_atlas->PageSide());
  for (const RunGlyph& glyph : collector.Glyphs())
  {
    // UI-198: the pen's whole pixel, and its fraction in steps of 1 ÷ ui.glyph.subpixel_steps; the baseline is whole.
    float whole = std::floor(glyph.x);
    auto step = static_cast<std::uint32_t>(std::lround((glyph.x - whole) * steps));
    if (step >= text.subpixelSteps)
    {
      whole += 1.0f;
      step = 0;
    }
    const float baseline = std::round(glyph.y);
    const GlyphKey glyphKey{text.FaceId(glyph.face), glyph.index, SixteenthsOf(glyph.emSize), step};
    std::optional<GlyphSlot> slot = m_atlas->Find(glyphKey);
    if (!slot)
    {
      slot = text.Rasterize(*m_atlas, glyphKey, glyph.face, glyph.emSize, glyph.index, static_cast<float>(step) / steps);
    }
    if (!slot)
    {
      if (text.tooLarge.insert(glyphKey).second)
      {
        Log(LogCategory::Ui, LogLevel::Warning, "glyph {} at {} px could not be cached, so it is not drawn", glyph.index, glyph.emSize);
      }
      continue;
    }
    if (slot->width == 0 || slot->height == 0)
    {
      continue;
    }
    PlacedGlyph placed;
    placed.page = slot->page;
    const float left = whole + static_cast<float>(slot->left);
    const float top = baseline + static_cast<float>(slot->top);
    placed.rectangle = {left, top, left + static_cast<float>(slot->width), top + static_cast<float>(slot->height)};
    placed.uv = {static_cast<float>(slot->x) / side, static_cast<float>(slot->y) / side, static_cast<float>(slot->x + slot->width) / side,
                 static_cast<float>(slot->y + slot->height) / side};
    line.glyphs.push_back(placed);
  }
  text.layouts.insert_or_assign(std::move(key), Implementation::KeptLayout{line, text.frame, m_atlas->Clears()});
  return line;
}

TextLine TextSystem::Metrics(FontRole _role, float _sizePixels)
{
  // A space draws nothing and takes the line's height and baseline at its size.
  TextLine line = Layout(" ", _role, _sizePixels);
  line.glyphs.clear();
  line.widthPixels = 0.0f;
  return line;
}

std::uint64_t TextSystem::GlyphsRasterized() const noexcept
{
  return m_implementation->rasterized;
}

std::string TextSystem::FaceFamily(FontRole _role) const
{
  const auto role = static_cast<std::size_t>(_role);
  return role < FONT_ROLE_COUNT ? m_implementation->faceFamilies[role] : std::string();
}

} // namespace Neuron
