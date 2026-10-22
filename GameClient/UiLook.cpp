#include "pch.h"

#include "AssetError.h"
#include "GameFonts.h"
#include "Log.h"
#include "MessageFormat.h"
#include "MessagePattern.h"
#include "UiLook.h"
#include "UiText.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <string_view>

namespace Species
{

namespace
{

// The content keys of the UI's images (AST-014; Design/implementation/mvp_assets.md), in UiImage's order.
constexpr std::array<std::string_view, static_cast<std::size_t>(UiImage::Count)> UI_IMAGE_KEYS{"textures/uitex/panelaccent",
                                                                                               "textures/uitex/panelneutral"};

// The test page's measures, in pixels; its own, not the game's.
constexpr float PAGE_MARGIN = 24.0f;
constexpr float PAGE_PADDING = 16.0f;
constexpr float PAGE_GAP = 8.0f;
constexpr float NEUTRAL_WIDTH = 160.0f;
constexpr float PANEL_WIDTH_MAX = 960.0f;
constexpr float SWATCH_SIZE = 48.0f;
constexpr float STYLE_SIZE = 24.0f;
constexpr float BODY_SIZE = 16.0f;
// The test page's strings (ADR-146): the game's name, which the table holds, an id no table holds, and a plural.
constexpr std::string_view TEST_PAGE_TITLE_ID = "ui.title.game_name";
constexpr std::string_view TEST_PAGE_UNWRITTEN_ID = "ui.test_page.unwritten";
constexpr std::string_view TEST_PAGE_PLURAL = "{count, plural, one {# Worker} other {# Workers}}";
constexpr std::array<std::int64_t, 3> TEST_PAGE_COUNTS{1, 2, 1500};

std::array<float, 4> Rgba(double _r, double _g, double _b, double _a) noexcept
{
  return {static_cast<float>(_r), static_cast<float>(_g), static_cast<float>(_b), static_cast<float>(_a)};
}

Neuron::UiQuad Quad(float _left, float _top, float _right, float _bottom, const std::array<float, 4>& _uv,
                    const std::array<float, 4>& _color)
{
  Neuron::UiQuad quad;
  quad.rectangle = {std::round(_left), std::round(_top), std::round(_right), std::round(_bottom)};
  quad.uv = _uv;
  quad.color = _color;
  return quad;
}

} // namespace

std::shared_ptr<const Neuron::UiImages> LoadUiImages(const Neuron::LooseContent& _content)
{
  auto images = std::make_shared<Neuron::UiImages>();
  for (const std::string_view key : UI_IMAGE_KEYS)
  {
    auto bytes = _content.Read(key);
    if (!bytes)
    {
      Neuron::Log(Neuron::LogCategory::Content, Neuron::LogLevel::Error, "the UI's image {}: {}", key,
                  Neuron::FormatAssetError(bytes.error()));
      images->files.emplace_back();
      continue;
    }
    images->files.push_back(std::move(*bytes));
  }
  return images;
}

Neuron::TextSystem::Desc TextSystemDesc(const TuningPresentation& _presentation)
{
  Neuron::TextSystem::Desc desc;
  const std::vector<std::string> game(GAME_FONT_FAMILIES.begin(), GAME_FONT_FAMILIES.end());
  for (const Neuron::FontRole role : {Neuron::FontRole::Title, Neuron::FontRole::Body, Neuron::FontRole::Caption})
  {
    desc.families[static_cast<std::size_t>(role)] = game;
  }
  desc.families[static_cast<std::size_t>(Neuron::FontRole::Development)].assign(DEVELOPMENT_FONT_FAMILIES.begin(),
                                                                                DEVELOPMENT_FONT_FAMILIES.end());
  const TuningPresentation::Ui::Glyph& glyph = _presentation.ui.glyph;
  desc.atlas.pageSide = static_cast<std::uint32_t>(std::clamp<std::int64_t>(glyph.pageSize, 64, 4096));
  desc.atlas.pageCount = static_cast<std::uint32_t>(std::clamp<std::int64_t>(glyph.pageCount, 1, 64));
  desc.subpixelSteps = static_cast<std::uint32_t>(std::clamp<std::int64_t>(glyph.subpixelSteps, 1, 16));
  desc.languageTag = "en-US";
  return desc;
}

float UiTextPixels(const TuningPresentation& _presentation, double _sizePixels, std::uint32_t _clientHeight, float _dpiScale) noexcept
{
  const TuningPresentation::Ui::Access& access = _presentation.ui.access;
  return Neuron::FinalTextPixels(_sizePixels, 1.0, _dpiScale,
                                 Neuron::MinimumTextPixels(_clientHeight, access.minText1080p, access.minText2160p));
}

void AppendBadgeCount(const BadgeLabel& _label, Neuron::TextSystem& _text, const TuningPresentation& _presentation,
                      std::uint32_t _clientHeight, float _dpiScale, std::vector<Neuron::UiBatch>& _out)
{
  const float size = UiTextPixels(_presentation, _presentation.ui.text.defaultSize, _clientHeight, _dpiScale);
  const Neuron::TextLine line = _text.Layout(_label.text, Neuron::FontRole::Body, size);
  Neuron::AppendText(_out, line, _label.left, _label.middle - line.heightPixels * 0.5f, Neuron::TextStyle{});
}

void AppendUiTestPage(Neuron::TextSystem& _text, const TuningPresentation& _presentation, const Neuron::StringTable& _strings,
                      const Neuron::MessageLocale& _locale, Neuron::SizePx _size, float _dpiScale, std::vector<Neuron::UiBatch>& _out)
{
  const TuningPresentation::Ui& ui = _presentation.ui;
  const std::array<float, 4> bodyTint =
    Rgba(ui.palette.windowBodyTint.r, ui.palette.windowBodyTint.g, ui.palette.windowBodyTint.b, ui.palette.windowBodyTint.a);
  const std::array<float, 4> outline =
    Rgba(ui.palette.textOutline.r, ui.palette.textOutline.g, ui.palette.textOutline.b, ui.palette.textOutline.a);
  const auto repeat = static_cast<float>(ui.chrome.bodyRepeat);

  // UI-054: the window body, the accent strip with u from 0 to the repeat across and v from 0 to 1 down, tinted and laid
  // over; the neutral strip beside it in the same form.
  const float panelWidth = std::clamp(static_cast<float>(_size.width) - 3.0f * PAGE_MARGIN - NEUTRAL_WIDTH, 0.0f, PANEL_WIDTH_MAX);
  const float panelHeight = std::max(static_cast<float>(_size.height) - 2.0f * PAGE_MARGIN, 0.0f);
  const float left = PAGE_MARGIN;
  const float top = PAGE_MARGIN;
  Neuron::AppendQuad(_out, Neuron::UiSource::Image, Neuron::UiBlend::Alpha, static_cast<std::uint16_t>(UiImage::PanelAccent),
                     Quad(left, top, left + panelWidth, top + panelHeight, {0.0f, 0.0f, repeat, 1.0f}, bodyTint));
  const float neutralLeft = left + panelWidth + PAGE_MARGIN;
  Neuron::AppendQuad(_out, Neuron::UiSource::Image, Neuron::UiBlend::Alpha, static_cast<std::uint16_t>(UiImage::PanelNeutral),
                     Quad(neutralLeft, top, neutralLeft + NEUTRAL_WIDTH, top + panelHeight, {0.0f, 0.0f, repeat, 1.0f}, bodyTint));

  // The body role at four sizes and the development role at one, outlined and filled as the framework's text is
  // (UI-192), each line below the last by its height.
  Neuron::TextStyle outlined;
  outlined.outline = outline;
  outlined.outlineOffsetPixels =
    static_cast<std::int32_t>(std::max<std::int64_t>(std::lround(static_cast<double>(ui.text.outlineOffset) * _dpiScale), 1));
  float y = top + PAGE_PADDING;
  const float x = left + PAGE_PADDING;
  const auto line = [&](std::string_view _utf8, Neuron::FontRole _role, float _sizePixels, const Neuron::TextStyle& _style)
  {
    const Neuron::TextLine laid = _text.Layout(_utf8, _role, _sizePixels * _dpiScale);
    Neuron::AppendText(_out, laid, x, y, _style);
    y += std::ceil(laid.heightPixels) + PAGE_GAP;
  };
  line("12 px The quick brown fox jumps over the lazy dog 0123456789", Neuron::FontRole::Body, 12.0f, outlined);
  line("16 px Àéîõü ¿×÷ £€° “quotes” – and — dashes…", Neuron::FontRole::Body, BODY_SIZE, outlined);
  line("24 px Species", Neuron::FontRole::Title, STYLE_SIZE, outlined);
  line("32 px Essence", Neuron::FontRole::Title, 32.0f, outlined);
  line("Development 14 px: frame 16.67 ms {0x1F} [ok]", Neuron::FontRole::Development, 14.0f, outlined);

  // The four ways a line is drawn: filled additive (UI-190), outlined and filled, inked alone, and laid over; then
  // characters the face lacks, which its fallback draws, and one no family of the chain draws.
  line("Additive fill", Neuron::FontRole::Body, STYLE_SIZE, Neuron::TextStyle{});
  line("Outline and fill", Neuron::FontRole::Body, STYLE_SIZE, outlined);
  Neuron::TextStyle ink;
  ink.blend = Neuron::UiBlend::Ink;
  ink.color = outline;
  line("Ink alone", Neuron::FontRole::Body, STYLE_SIZE, ink);
  Neuron::TextStyle over;
  over.blend = Neuron::UiBlend::Alpha;
  over.color = {1.0f, 0.85f, 0.4f, 1.0f};
  line("Alpha fill", Neuron::FontRole::Body, STYLE_SIZE, over);
  line("Fallback: Ł ж Ω €, none: 漢", Neuron::FontRole::Body, STYLE_SIZE, outlined);

  // ADR-146: a string the table holds, one it does not, which a Debug build shows as its id, and a plural formatted in
  // the language's forms through ICU.
  const Neuron::TextStyle plain;
  line(_strings.Lookup(TEST_PAGE_TITLE_ID), Neuron::FontRole::Body, BODY_SIZE, plain);
  line(_strings.Lookup(TEST_PAGE_UNWRITTEN_ID), Neuron::FontRole::Body, BODY_SIZE, plain);
  if (const auto workers = Neuron::ParseMessage(TEST_PAGE_PLURAL))
  {
    std::string counts;
    for (const std::int64_t count : TEST_PAGE_COUNTS)
    {
      const std::array arguments{Neuron::FormatArgument{"count", count}};
      counts += (counts.empty() ? "" : ", ") + Neuron::FormatPattern(*workers, arguments, _locale);
    }
    line(counts, Neuron::FontRole::Body, BODY_SIZE, plain);
  }

  // A swatch in each blend the UI draws solid and image quads in.
  const std::array<float, 4> whole{0.0f, 0.0f, 1.0f, 1.0f};
  float swatch = x;
  Neuron::AppendQuad(_out, Neuron::UiSource::Solid, Neuron::UiBlend::Alpha, 0,
                     Quad(swatch, y, swatch + SWATCH_SIZE, y + SWATCH_SIZE, whole, {0.2f, 0.6f, 1.0f, 0.6f}));
  swatch += SWATCH_SIZE + PAGE_GAP;
  Neuron::AppendQuad(_out, Neuron::UiSource::Solid, Neuron::UiBlend::Additive, 0,
                     Quad(swatch, y, swatch + SWATCH_SIZE, y + SWATCH_SIZE, whole, {0.3f, 0.3f, 0.3f, 1.0f}));
  swatch += SWATCH_SIZE + PAGE_GAP;
  Neuron::AppendQuad(_out, Neuron::UiSource::Image, Neuron::UiBlend::Additive, static_cast<std::uint16_t>(UiImage::PanelNeutral),
                     Quad(swatch, y, swatch + SWATCH_SIZE, y + SWATCH_SIZE, whole, {1.0f, 1.0f, 1.0f, 0.5f}));
}

} // namespace Species
