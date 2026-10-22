#include "pch.h"

#include "AssetError.h"
#include "Log.h"
#include "MessageFormat.h"
#include "MessagePattern.h"
#include "TextLayout.h"
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
constexpr std::uint32_t STYLE_SCALE = 2;
constexpr std::uint32_t SCALE_LINES = 4;
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

std::uint32_t UiTextScale(const TuningPresentation& _presentation, const Neuron::FontAsset& _face, double _sizePixels,
                          std::uint32_t _clientHeight) noexcept
{
  const TuningPresentation::Ui::Access& access = _presentation.ui.access;
  return Neuron::TextScaleForSize(_sizePixels, _face.designSizePixels,
                                  Neuron::MinimumTextPixels(_clientHeight, access.minText1080p, access.minText2160p));
}

void AppendBadgeCount(const BadgeLabel& _label, const Neuron::FontAsset& _face, const TuningPresentation& _presentation,
                      std::uint32_t _clientHeight, std::vector<Neuron::UiBatch>& _out)
{
  Neuron::TextStyle style;
  style.scale = UiTextScale(_presentation, _face, _presentation.ui.text.defaultSize, _clientHeight);
  const float cellHeight = static_cast<float>(_face.cellHeightPixels) * static_cast<float>(style.scale);
  Neuron::AppendText(_out, _face, _label.text, _label.left, _label.middle - cellHeight * 0.5f, style);
}

void AppendUiTestPage(const Neuron::FontAsset& _face, const TuningPresentation& _presentation, const Neuron::StringTable& _strings,
                      const Neuron::MessageLocale& _locale, Neuron::SizePx _size, std::vector<Neuron::UiBatch>& _out)
{
  const TuningPresentation::Ui& ui = _presentation.ui;
  const std::array<float, 4> bodyTint =
    Rgba(ui.palette.windowBodyTint.r, ui.palette.windowBodyTint.g, ui.palette.windowBodyTint.b, ui.palette.windowBodyTint.a);
  const std::array<float, 4> outline =
    Rgba(ui.palette.textOutline.r, ui.palette.textOutline.g, ui.palette.textOutline.b, ui.palette.textOutline.a);
  const auto repeat = static_cast<float>(ui.chrome.bodyRepeat);
  const float cell = static_cast<float>(_face.cellHeightPixels);

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

  // GameFont at the scales 1 to 4, outlined and filled as the framework's text is (UI-192).
  Neuron::TextStyle outlined;
  outlined.outline = outline;
  outlined.outlineOffsetPixels = static_cast<std::int32_t>(std::max<std::int64_t>(ui.text.outlineOffset, 1));
  constexpr std::array<std::string_view, SCALE_LINES> SCALE_TEXTS{"k=1 The quick brown fox jumps over the lazy dog 0123456789",
                                                                  "k=2 Àéîõü ¿×÷ 0123456789", "k=3 Species", "k=4 Essence"};
  float y = top + PAGE_PADDING;
  const float x = left + PAGE_PADDING;
  for (std::uint32_t k = 1; k <= SCALE_LINES; ++k)
  {
    outlined.scale = k;
    Neuron::AppendText(_out, _face, SCALE_TEXTS[k - 1], x, y, outlined);
    y += cell * static_cast<float>(k) + PAGE_GAP;
  }

  // The four ways a line is drawn, at one scale: filled additive (UI-190), outlined and filled, inked alone, and laid
  // over; then a line of characters GameFont does not hold, each drawn as its missing glyph.
  Neuron::TextStyle additive;
  additive.scale = STYLE_SCALE;
  Neuron::AppendText(_out, _face, "Additive fill", x, y, additive);
  y += cell * STYLE_SCALE + PAGE_GAP;
  outlined.scale = STYLE_SCALE;
  Neuron::AppendText(_out, _face, "Outline and fill", x, y, outlined);
  y += cell * STYLE_SCALE + PAGE_GAP;
  Neuron::TextStyle ink;
  ink.scale = STYLE_SCALE;
  ink.blend = Neuron::UiBlend::Ink;
  ink.color = outline;
  Neuron::AppendText(_out, _face, "Ink alone", x, y, ink);
  y += cell * STYLE_SCALE + PAGE_GAP;
  Neuron::TextStyle over;
  over.scale = STYLE_SCALE;
  over.blend = Neuron::UiBlend::Alpha;
  over.color = {1.0f, 0.85f, 0.4f, 1.0f};
  Neuron::AppendText(_out, _face, "Alpha fill", x, y, over);
  y += cell * STYLE_SCALE + PAGE_GAP;
  Neuron::AppendText(_out, _face, "Missing: € Ł ж 漢", x, y, outlined);
  y += cell * STYLE_SCALE + PAGE_GAP;

  // ADR-146: a string the table holds, one it does not, which a Debug build shows as its id, and a plural formatted in
  // the language's forms through ICU.
  Neuron::TextStyle plain;
  Neuron::AppendText(_out, _face, _strings.Lookup(TEST_PAGE_TITLE_ID), x, y, plain);
  y += cell + PAGE_GAP;
  Neuron::AppendText(_out, _face, _strings.Lookup(TEST_PAGE_UNWRITTEN_ID), x, y, plain);
  y += cell + PAGE_GAP;
  if (const auto workers = Neuron::ParseMessage(TEST_PAGE_PLURAL))
  {
    std::string counts;
    for (const std::int64_t count : TEST_PAGE_COUNTS)
    {
      const std::array arguments{Neuron::FormatArgument{"count", count}};
      counts += (counts.empty() ? "" : ", ") + Neuron::FormatPattern(*workers, arguments, _locale);
    }
    Neuron::AppendText(_out, _face, counts, x, y, plain);
  }
  y += cell + PAGE_GAP;

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
