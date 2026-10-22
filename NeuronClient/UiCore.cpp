#include "pch.h"

#include "Log.h"
#include "UiCore.h"
#include "UiText.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <ranges>
#include <set>
#include <utility>

namespace Neuron
{

namespace
{

using Color = UiStyle::Color;

[[nodiscard]] bool Focusable(UiWidgetKind _kind) noexcept
{
  return _kind == UiWidgetKind::PageButton || _kind == UiWidgetKind::CheckBox || _kind == UiWidgetKind::Cycler ||
         _kind == UiWidgetKind::TextField;
}

// UI-066's editing characters, which arrive as characters (INP-113).
constexpr char32_t CHARACTER_BACKSPACE = 0x08;
constexpr char32_t CHARACTER_ENTER = 0x0D;
constexpr char32_t CHARACTER_ESCAPE = 0x1B;
constexpr char32_t CHARACTER_FIRST_PRINTABLE = 0x20;
constexpr char32_t CHARACTER_DELETE = 0x7F;
constexpr char32_t CHARACTER_LAST = 0x10FFFF;
constexpr char32_t SURROGATE_FIRST = 0xD800;
constexpr char32_t SURROGATE_LAST = 0xDFFF;

// A Unicode scalar value as UTF-8, appended.
void AppendUtf8(std::string& _out, char32_t _codePoint)
{
  if (_codePoint < 0x80)
  {
    _out.push_back(static_cast<char>(_codePoint));
  }
  else if (_codePoint < 0x800)
  {
    _out.push_back(static_cast<char>(0xC0 | (_codePoint >> 6)));
    _out.push_back(static_cast<char>(0x80 | (_codePoint & 0x3F)));
  }
  else if (_codePoint < 0x10000)
  {
    _out.push_back(static_cast<char>(0xE0 | (_codePoint >> 12)));
    _out.push_back(static_cast<char>(0x80 | ((_codePoint >> 6) & 0x3F)));
    _out.push_back(static_cast<char>(0x80 | (_codePoint & 0x3F)));
  }
  else
  {
    _out.push_back(static_cast<char>(0xF0 | (_codePoint >> 18)));
    _out.push_back(static_cast<char>(0x80 | ((_codePoint >> 12) & 0x3F)));
    _out.push_back(static_cast<char>(0x80 | ((_codePoint >> 6) & 0x3F)));
    _out.push_back(static_cast<char>(0x80 | (_codePoint & 0x3F)));
  }
}

// The characters of UTF-8 text: its bytes that do not continue a character.
[[nodiscard]] std::size_t CharacterCount(std::string_view _utf8) noexcept
{
  return static_cast<std::size_t>(
    std::ranges::count_if(_utf8, [](char _byte) { return (static_cast<std::uint8_t>(_byte) & 0xC0u) != 0x80u; }));
}

// The last character of UTF-8 text removed, with its continuation bytes.
void RemoveLastCharacter(std::string& _utf8)
{
  while (!_utf8.empty() && (static_cast<std::uint8_t>(_utf8.back()) & 0xC0u) == 0x80u)
  {
    _utf8.pop_back();
  }
  if (!_utf8.empty())
  {
    _utf8.pop_back();
  }
}

[[nodiscard]] Color WithAlpha(const Color& _color, float _alpha) noexcept
{
  return {_color[0], _color[1], _color[2], _color[3] * _alpha};
}

// UI-113, UI-049: Latin-1 text upper-cased as the content build's coverage check upper-cases it (ADR-147); a
// character outside Latin-1, or bytes that are not UTF-8, are kept as they are.
[[nodiscard]] std::string UpperCased(std::string_view _utf8)
{
  std::string upper;
  upper.reserve(_utf8.size());
  for (std::size_t n = 0; n < _utf8.size(); ++n)
  {
    const auto byte = static_cast<std::uint8_t>(_utf8[n]);
    if (byte >= 'a' && byte <= 'z')
    {
      upper.push_back(static_cast<char>(byte - 0x20));
    }
    else if ((byte == 0xC3) && n + 1 < _utf8.size())
    {
      // U+00C0 to U+00FF are C3 80 to C3 BF; à to þ but ÷ lie 0x20 above their capitals.
      const auto next = static_cast<std::uint8_t>(_utf8[n + 1]);
      const std::uint32_t point = 0xC0u + (next & 0x3Fu);
      upper.push_back(static_cast<char>(byte));
      upper.push_back(static_cast<char>(point >= 0xE0 && point <= 0xFE && point != 0xF7 ? next - 0x20 : next));
      ++n;
    }
    else
    {
      upper.push_back(static_cast<char>(byte));
    }
  }
  return upper;
}

UiQuad Quad(const UiRect& _rect, const Color& _color)
{
  UiQuad quad;
  quad.rectangle = {_rect.left, _rect.top, _rect.right, _rect.bottom};
  quad.color = _color;
  return quad;
}

// A line loop _width pixels wide just inside _rect, as four solid quads.
void AppendOutline(std::vector<UiBatch>& _out, const UiRect& _rect, float _width, UiBlend _blend, const Color& _color)
{
  AppendQuad(_out, UiSource::Solid, _blend, 0, Quad({_rect.left, _rect.top, _rect.right, _rect.top + _width}, _color));
  AppendQuad(_out, UiSource::Solid, _blend, 0, Quad({_rect.left, _rect.bottom - _width, _rect.right, _rect.bottom}, _color));
  AppendQuad(_out, UiSource::Solid, _blend, 0, Quad({_rect.left, _rect.top + _width, _rect.left + _width, _rect.bottom - _width}, _color));
  AppendQuad(_out, UiSource::Solid, _blend, 0,
             Quad({_rect.right - _width, _rect.top + _width, _rect.right, _rect.bottom - _width}, _color));
}

// A line of text in _role at _pixels, its row centered vertically in _rect (UI-165), at _left or aligned in _rect.
void AppendRowText(std::vector<UiBatch>& _out, TextSystem& _text, std::string_view _utf8, FontRole _role, float _pixels,
                   const UiRect& _rect, UiAlign _align, float _left, const TextStyle& _style)
{
  if (_utf8.empty())
  {
    return;
  }
  const TextLine line = _text.Layout(_utf8, _role, _pixels);
  float x = _left;
  if (_align == UiAlign::Center)
  {
    x = _rect.left + (_rect.Width() - line.widthPixels) * 0.5f;
  }
  else if (_align == UiAlign::Right)
  {
    x = _left - line.widthPixels;
  }
  AppendText(_out, line, x, RowTextTop(_rect.top, _rect.Height(), line.heightPixels), _style);
}

TextStyle CaptionStyle(const UiStyle& _style, const Color& _fill, float _outlinePixels)
{
  TextStyle style;
  style.color = _fill;
  style.outline = _style.textOutline;
  style.outlineOffsetPixels = static_cast<std::int32_t>(_outlinePixels);
  return style;
}

// UI-108, UI-117, UI-125: a caption's fill color by its widget's state.
const Color& CaptionColor(const UiStyle& _style, const UiWidgetDesc& _desc, bool _highlighted)
{
  if (_desc.inactive)
  {
    return _style.captionInactive;
  }
  if (_highlighted)
  {
    return _style.captionFocus;
  }
  return _desc.alternativeColor ? _style.captionAlternative : _style.caption;
}

// UI-115: a fill box's halo, body, drifting glow and outlines.
void AppendFillBox(std::vector<UiBatch>& _out, const UiRect& _rect, const UiStyle& _style, const UiMetrics& _metrics, double _seconds)
{
  const auto width = static_cast<float>(_metrics.client.width);
  const auto height = static_cast<float>(_metrics.client.height);
  // 1. The darkening halo, from the edge color at the box to nothing outside, in the ink state with alpha 0.
  const float shadow = std::round(_style.boxShadowWidth * width);
  if (shadow >= 1.0f)
  {
    const Color edge = WithAlpha(_style.boxShadowEdge, 0.0f);
    const Color none{0.0f, 0.0f, 0.0f, 0.0f};
    UiQuad top = Quad({_rect.left - shadow, _rect.top - shadow, _rect.right + shadow, _rect.top}, none);
    top.colorTo = edge;
    UiQuad bottom = Quad({_rect.left - shadow, _rect.bottom, _rect.right + shadow, _rect.bottom + shadow}, edge);
    bottom.colorTo = none;
    UiQuad left = Quad({_rect.left - shadow, _rect.top, _rect.left, _rect.bottom}, none);
    left.colorTo = edge;
    left.across = true;
    UiQuad right = Quad({_rect.right, _rect.top, _rect.right + shadow, _rect.bottom}, edge);
    right.colorTo = none;
    right.across = true;
    for (const UiQuad& quad : {top, bottom, left, right})
    {
      AppendQuad(_out, UiSource::Solid, UiBlend::Ink, 0, quad);
    }
  }
  // 2. The blurred body, tiled across.
  if (_style.fillBlur.has_value())
  {
    UiQuad body = Quad(_rect, _style.boxTint);
    body.uv = {0.0f, 0.0f, _rect.Width() / std::max(_style.boxBlurTilePixels, 1.0f), 1.0f};
    AppendQuad(_out, UiSource::Image, UiBlend::Alpha, *_style.fillBlur, body);
  }
  // 3. The cloudy glow, drifting slowly.
  if (_style.glowCloudy.has_value() && width > 0.0f && height > 0.0f)
  {
    const auto t = static_cast<float>(_seconds);
    const float u = -_style.glowMotion[0] * t;
    const float v = -_style.glowMotion[1] * t;
    UiQuad glow = Quad(_rect, _style.boxGlow);
    glow.uv = {u, v, u + _rect.Width() / width + _style.glowMotion[0] * std::sin(t),
               v + _rect.Height() / height + _style.glowMotion[1] * std::sin(t)};
    AppendQuad(_out, UiSource::Image, UiBlend::Additive, *_style.glowCloudy, glow);
  }
  // 4. The outline and its inner line.
  AppendOutline(_out, _rect, _metrics.LinePixels(_style.boxOutlinePixels), UiBlend::Additive, _style.boxOutline);
  const float inset = _metrics.LinePixels(_style.boxInnerInsetPixels);
  AppendOutline(_out, {_rect.left + inset, _rect.top + inset, _rect.right - inset, _rect.bottom - inset}, _metrics.LinePixels(1.0),
                UiBlend::Additive, WithAlpha(_style.boxOutline, _style.boxInnerAlpha));
}

} // namespace

// --- UiWindow -------------------------------------------------------------------------------------------------------

UiWindow::UiWindow(std::string _name, bool _layer, std::function<UiHandle()> _nextHandle)
  : m_name(std::move(_name)),
    m_layer(_layer),
    m_nextHandle(std::move(_nextHandle))
{
}

UiHandle UiWindow::Add(UiWidgetDesc _desc)
{
  Widget widget;
  widget.handle = m_nextHandle();
  if (_desc.kind == UiWidgetKind::Cycler && !_desc.options.empty())
  {
    _desc.selected = std::min(_desc.selected, _desc.options.size() - 1);
  }
  widget.desc = std::move(_desc);
  m_widgets.push_back(std::move(widget));
  return m_widgets.back().handle;
}

void UiWindow::SetNavigation(std::vector<UiHandle> _order, UiHandle _initialFocus)
{
  m_navigation = std::move(_order);
  m_cursor = 0;
  if (_initialFocus)
  {
    SetCursor(_initialFocus);
  }
}

void UiWindow::SetNeighbors(UiHandle _widget, const UiNeighbors& _neighbors)
{
  if (Widget* widget = Get(_widget))
  {
    widget->neighbors = _neighbors;
  }
}

void UiWindow::SetBackAction(UiHandle _widget)
{
  m_backAction = _widget;
}

void UiWindow::SetRect(UiHandle _widget, const UiRect& _rect)
{
  if (Widget* widget = Get(_widget))
  {
    widget->rect = SnapRect(_rect); // UI-169
  }
}

void UiWindow::SetText(UiHandle _widget, std::string _text)
{
  if (Widget* widget = Get(_widget))
  {
    widget->desc.text = std::move(_text);
  }
}

void UiWindow::SetFontPixels(UiHandle _widget, float _pixels)
{
  if (Widget* widget = Get(_widget))
  {
    widget->desc.fontPixels = _pixels;
  }
}

void UiWindow::SetChecked(UiHandle _widget, bool _checked)
{
  if (Widget* widget = Get(_widget))
  {
    widget->desc.checked = _checked;
  }
}

void UiWindow::SetSelected(UiHandle _widget, std::size_t _option)
{
  if (Widget* widget = Get(_widget); widget != nullptr && !widget->desc.options.empty())
  {
    widget->desc.selected = std::min(_option, widget->desc.options.size() - 1);
  }
}

void UiWindow::SetValue(UiHandle _widget, std::string _value)
{
  if (Widget* widget = Get(_widget))
  {
    widget->desc.value = std::move(_value);
  }
}

void UiWindow::SetInactive(UiHandle _widget, bool _inactive)
{
  if (Widget* widget = Get(_widget))
  {
    widget->desc.inactive = _inactive;
  }
}

void UiWindow::SetCursor(UiHandle _widget)
{
  if (const auto found = std::ranges::find(m_navigation, _widget); found != m_navigation.end())
  {
    m_cursor = static_cast<std::size_t>(found - m_navigation.begin());
  }
}

UiHandle UiWindow::Find(std::string_view _id) const noexcept
{
  const auto found = std::ranges::find(m_widgets, _id, [](const Widget& _widget) { return std::string_view(_widget.desc.id); });
  return found != m_widgets.end() ? found->handle : UiHandle{};
}

bool UiWindow::Contains(UiHandle _widget) const noexcept
{
  return Get(_widget) != nullptr;
}

bool UiWindow::Checked(UiHandle _widget) const noexcept
{
  const Widget* widget = Get(_widget);
  return widget != nullptr && widget->desc.checked;
}

std::size_t UiWindow::Selected(UiHandle _widget) const noexcept
{
  const Widget* widget = Get(_widget);
  return widget != nullptr ? widget->desc.selected : 0;
}

std::string_view UiWindow::Value(UiHandle _widget) const noexcept
{
  const Widget* widget = Get(_widget);
  return widget != nullptr ? std::string_view(widget->desc.value) : std::string_view{};
}

UiRect UiWindow::RectOf(UiHandle _widget) const noexcept
{
  const Widget* widget = Get(_widget);
  return widget != nullptr ? widget->rect : UiRect{};
}

UiHandle UiWindow::Cursor() const noexcept
{
  return m_cursor < m_navigation.size() ? m_navigation[m_cursor] : UiHandle{};
}

bool UiWindow::Highlighted(UiHandle _widget) const noexcept
{
  return _widget && (_widget == Cursor() || (m_hoverMode && _widget == m_hover));
}

UiWindow::Widget* UiWindow::Get(UiHandle _widget) noexcept
{
  const auto found = std::ranges::find(m_widgets, _widget, &Widget::handle);
  return _widget && found != m_widgets.end() ? &*found : nullptr;
}

const UiWindow::Widget* UiWindow::Get(UiHandle _widget) const noexcept
{
  const auto found = std::ranges::find(m_widgets, _widget, &Widget::handle);
  return _widget && found != m_widgets.end() ? &*found : nullptr;
}

bool UiWindow::Acts(UiHandle _widget) const noexcept
{
  const Widget* widget = Get(_widget);
  return widget != nullptr && Focusable(widget->desc.kind) && !widget->desc.inactive;
}

// --- UiPage ---------------------------------------------------------------------------------------------------------

void UiPage::Update(UiWindow& /*_window*/, std::span<const UiEvent> /*_events*/, UiNavigator& /*_navigator*/) {}

void UiPage::Back(UiWindow& _window, UiNavigator& _navigator)
{
  if (_window.IsLayer())
  {
    _navigator.CloseLayer();
  }
  else
  {
    _navigator.Request({UiPageRequest::Kind::Pop, 0});
  }
}

// --- UiCore ---------------------------------------------------------------------------------------------------------

UiCore::UiCore(PageFactory _pages)
  : m_pages(std::move(_pages))
{
}

UiCore::~UiCore() = default;

void UiCore::OpenScreen(UiPageId _root)
{
  m_pendingScreen = _root;
  m_pendingClose = false;
  m_pendingRequest.reset();
}

void UiCore::CloseScreen()
{
  m_pendingClose = true;
  m_pendingScreen.reset();
  m_pendingRequest.reset();
}

void UiCore::Request(const UiPageRequest& _request)
{
  m_pendingRequest = _request; // UI-350: the last request of a frame wins
}

void UiCore::OpenLayer(std::unique_ptr<UiPage> _layer)
{
  if (_layer != nullptr)
  {
    m_pendingLayers.push_back(std::move(_layer));
  }
}

void UiCore::CloseLayer()
{
  ++m_pendingLayerCloses;
}

const UiWindow* UiCore::Focused() const noexcept
{
  const Entry* entry = FocusedEntry();
  return entry != nullptr ? entry->window.get() : nullptr;
}

UiWindow* UiCore::Focused() noexcept
{
  Entry* entry = FocusedEntry();
  return entry != nullptr ? entry->window.get() : nullptr;
}

bool UiCore::TakesPointer() const noexcept
{
  return FocusedEntry() != nullptr;
}

UiCore::Entry* UiCore::FocusedEntry() noexcept
{
  if (!m_layers.empty())
  {
    return &m_layers.back();
  }
  return m_page.has_value() ? &*m_page : nullptr;
}

const UiCore::Entry* UiCore::FocusedEntry() const noexcept
{
  if (!m_layers.empty())
  {
    return &m_layers.back();
  }
  return m_page.has_value() ? &*m_page : nullptr;
}

UiCore::Entry UiCore::Make(std::unique_ptr<UiPage> _page, bool _layer)
{
  Entry entry;
  entry.window.reset(new UiWindow(std::string(_page->Name()), _layer, [this] { return UiHandle{m_nextSerial++}; }));
  entry.page = std::move(_page);
  entry.page->Enter(*entry.window);
  if (m_metrics.has_value())
  {
    entry.page->Layout(*entry.window, *m_metrics);
  }
  return entry;
}

void UiCore::EnterTop()
{
  m_captured = {};
  m_layers.clear();
  m_page.reset();
  if (m_stack.empty())
  {
    return;
  }
  std::unique_ptr<UiPage> page = m_pages(m_stack.back());
  if (page == nullptr)
  {
    Log(LogCategory::Ui, LogLevel::Error, "the UI has no page {}", m_stack.back());
    return;
  }
  m_page = Make(std::move(page), false);
}

void UiCore::ApplyPageRequest(const UiPageRequest& _request)
{
  if (m_stack.empty())
  {
    return;
  }
  const auto onStack = std::ranges::find(m_stack, _request.page);
  const bool present = onStack != m_stack.end();
  const bool current = m_stack.back() == _request.page;
  switch (_request.kind)
  {
  case UiPageRequest::Kind::Pop:
    if (m_stack.size() == 1)
    {
      Log(LogCategory::Ui, LogLevel::Warning, "a pop of the page stack's only page was refused (UI-331)");
      return;
    }
    m_stack.pop_back();
    break;
  case UiPageRequest::Kind::Push:
  case UiPageRequest::Kind::Replace:
  case UiPageRequest::Kind::UnwindTo:
    if (current)
    {
      return; // UI-355: a request for the current page does nothing
    }
    if (present)
    {
      m_stack.erase(onStack + 1, m_stack.end()); // UI-331: a page on the stack is unwound to
    }
    else if (_request.kind == UiPageRequest::Kind::Push)
    {
      m_stack.push_back(_request.page);
    }
    else if (_request.kind == UiPageRequest::Kind::Replace)
    {
      m_stack.back() = _request.page;
    }
    else
    {
      m_stack.resize(1); // UI-331: unwinding to a page not on the stack gives the root and the page
      m_stack.push_back(_request.page);
    }
    break;
  }
  EnterTop();
}

void UiCore::ApplyPending()
{
  if (m_pendingClose)
  {
    m_pendingClose = false;
    m_stack.clear();
    EnterTop();
  }
  if (m_pendingScreen.has_value())
  {
    m_stack.assign(1, *m_pendingScreen); // UI-332
    m_pendingScreen.reset();
    EnterTop();
  }
  for (; m_pendingLayerCloses > 0 && !m_layers.empty(); --m_pendingLayerCloses)
  {
    m_layers.pop_back();
    m_captured = {};
  }
  m_pendingLayerCloses = 0;
  if (m_pendingRequest.has_value())
  {
    const UiPageRequest request = *m_pendingRequest;
    m_pendingRequest.reset();
    ApplyPageRequest(request);
  }
  std::vector<std::unique_ptr<UiPage>> layers = std::move(m_pendingLayers);
  m_pendingLayers.clear();
  for (std::unique_ptr<UiPage>& layer : layers)
  {
    m_layers.push_back(Make(std::move(layer), true));
    m_captured = {};
  }
}

void UiCore::Update(const UiInput& _input, const UiMetrics& _metrics)
{
  // UI-172: a change of the size, the DPI or a scale lays every window out again; no state is touched (UI-174).
  if (!m_metrics.has_value() || *m_metrics != _metrics)
  {
    m_metrics = _metrics;
    if (m_page.has_value())
    {
      m_page->page->Layout(*m_page->window, _metrics);
    }
    for (Entry& layer : m_layers)
    {
      layer.page->Layout(*layer.window, _metrics);
    }
  }
  ApplyPending(); // UI-351: the requests of the last frame; each new window is laid out at this frame's metrics

  m_events.clear();
  Entry* focused = FocusedEntry();
  if (focused == nullptr)
  {
    return;
  }
  // UI-038: an edit whose field is no longer in the focused window is abandoned (UI-064).
  if (m_editing && !focused->window->Contains(m_editing))
  {
    m_editing = {};
    m_editText.clear();
  }
  if (_input.windowFocused)
  {
    // UI-123, INP-156: while a field is edited, and in the frame its edit ends by Enter or Escape, the keys type and do
    // not navigate, so one key press never does both.
    const bool editing = static_cast<bool>(m_editing);
    Point(*focused->window, _input);
    if (m_editing)
    {
      Type(*focused->window, _input.text);
    }
    if (!editing && !m_editing)
    {
      Navigate(*focused->window, *focused->page, _input);
    }
  }
  else
  {
    m_captured = {};
  }
  // UI-352 step 4: every window's hook, the focused one with its events.
  if (m_page.has_value())
  {
    m_page->page->Update(*m_page->window, focused == &*m_page ? std::span<const UiEvent>(m_events) : std::span<const UiEvent>{}, *this);
  }
  for (Entry& layer : m_layers)
  {
    layer.page->Update(*layer.window, focused == &layer ? std::span<const UiEvent>(m_events) : std::span<const UiEvent>{}, *this);
  }
}

void UiCore::Point(UiWindow& _window, const UiInput& _input)
{
  // UI-026, UI-028: the newest focusable widget whose rectangle holds the pointer.
  UiHandle under;
  for (const UiWindow::Widget& widget : std::views::reverse(_window.m_widgets))
  {
    if (Focusable(widget.desc.kind) && widget.rect.Contains(_input.pointerX, _input.pointerY))
    {
      under = widget.handle;
      break;
    }
  }
  // UI-032, UI-056: a widget the pointer moves onto becomes the hover widget and the cursor, in hover-highlight mode.
  if (_input.pointerMoved && under != _window.m_hover)
  {
    _window.m_hover = under;
    if (under)
    {
      _window.SetCursor(under);
      _window.m_hoverMode = true;
    }
  }
  // UI-031, UI-035: a press captures the widget under the pointer, and a release on it clicks it (UI-006: in order).
  if (_input.pointerDown)
  {
    // UI-064: a press on another text field abandons the edit; ADR-154: a press anywhere else applies it first, so that
    // a value typed before a button is pressed is kept.
    if (m_editing && under != m_editing)
    {
      const UiWindow::Widget* target = _window.Get(under);
      EndEdit(_window, target == nullptr || target->desc.kind != UiWidgetKind::TextField);
    }
    m_captured = under;
    if (under)
    {
      _window.m_hover = under;
      _window.SetCursor(under);
      _window.m_hoverMode = true;
    }
  }
  if (_input.pointerUp && m_captured)
  {
    const UiHandle captured = std::exchange(m_captured, UiHandle{});
    if (_window.Contains(captured) && _window.RectOf(captured).Contains(_input.pointerX, _input.pointerY) && _window.Acts(captured))
    {
      Click(_window, captured);
    }
  }
}

void UiCore::Navigate(UiWindow& _window, UiPage& _page, const UiInput& _input)
{
  const std::size_t count = _window.m_navigation.size();
  const auto step = [&](int _direction, UiHandle UiNeighbors::*_neighbor)
  {
    if (count == 0)
    {
      return;
    }
    _window.m_hoverMode = false; // UI-056: navigation ends hover-highlight mode
    // UI-381: an authored neighbor wins over the order.
    if (const UiWindow::Widget* cursor = _window.Get(_window.Cursor()); cursor != nullptr && cursor->neighbors.*_neighbor)
    {
      if (_window.Acts(cursor->neighbors.*_neighbor))
      {
        _window.SetCursor(cursor->neighbors.*_neighbor);
      }
      return;
    }
    if (_direction == 0)
    {
      return; // left and right move only to authored neighbors (UI-210)
    }
    // UI-051: the next active entry in the direction, wrapping, at most once round the order.
    std::size_t candidate = _window.m_cursor;
    for (std::size_t tried = 0; tried < count; ++tried)
    {
      candidate = (candidate + count + static_cast<std::size_t>(_direction > 0 ? 1 : count - 1)) % count;
      if (_window.Acts(_window.m_navigation[candidate]))
      {
        _window.m_cursor = candidate;
        break;
      }
    }
  };
  if (_input.down)
  {
    step(+1, &UiNeighbors::down);
  }
  if (_input.up)
  {
    step(-1, &UiNeighbors::up);
  }
  if (_input.left)
  {
    step(0, &UiNeighbors::left);
  }
  if (_input.right)
  {
    step(0, &UiNeighbors::right);
  }
  if (_input.activate && _window.Acts(_window.Cursor()))
  {
    Click(_window, _window.Cursor());
  }
  // UI-051, UI-383: the close action clicks the back action, or runs the page's back handler.
  if (_input.close)
  {
    if (_window.m_backAction && _window.Acts(_window.m_backAction))
    {
      Click(_window, _window.m_backAction);
    }
    else if (!_window.m_backAction)
    {
      _page.Back(_window, *this);
    }
  }
}

void UiCore::Click(UiWindow& _window, UiHandle _widget)
{
  UiWindow::Widget* widget = _window.Get(_widget);
  if (widget == nullptr)
  {
    return;
  }
  if (widget->desc.kind == UiWidgetKind::CheckBox)
  {
    widget->desc.checked = !widget->desc.checked; // UI-080
    m_events.push_back({UiEventKind::Changed, _widget});
  }
  else if (widget->desc.kind == UiWidgetKind::Cycler && !widget->desc.options.empty())
  {
    widget->desc.selected = (widget->desc.selected + 1) % widget->desc.options.size(); // UI-116's cycle mode
    m_events.push_back({UiEventKind::Changed, _widget});
  }
  else if (widget->desc.kind == UiWidgetKind::TextField && m_editing != _widget)
  {
    BeginEdit(_window, _widget); // UI-064, UI-123
  }
  m_events.push_back({UiEventKind::Click, _widget});
}

void UiCore::BeginEdit(UiWindow& _window, UiHandle _widget)
{
  if (const UiWindow::Widget* widget = _window.Get(_widget); widget != nullptr)
  {
    m_editing = _widget;
    m_editText = widget->desc.value;
  }
}

void UiCore::EndEdit(UiWindow& _window, bool _apply)
{
  const UiHandle edited = std::exchange(m_editing, UiHandle{});
  if (UiWindow::Widget* widget = _window.Get(edited); widget != nullptr && _apply)
  {
    widget->desc.value = std::move(m_editText);
    m_events.push_back({UiEventKind::Changed, edited});
  }
  m_editText.clear();
}

void UiCore::Type(UiWindow& _window, std::span<const TextCharacter> _text)
{
  const UiWindow::Widget* widget = _window.Get(m_editing);
  if (widget == nullptr)
  {
    return;
  }
  const UiWidgetDesc& desc = widget->desc;
  for (const TextCharacter& character : _text)
  {
    const char32_t point = character.codePoint;
    // UI-066 in the order of its tests: Backspace, Enter, Escape, then the characters a field takes.
    if (point == CHARACTER_BACKSPACE)
    {
      RemoveLastCharacter(m_editText);
    }
    else if (point == CHARACTER_ENTER || point == CHARACTER_ESCAPE)
    {
      EndEdit(_window, point == CHARACTER_ENTER);
      return; // the rest of the frame's characters reach no field
    }
    else if (point < CHARACTER_FIRST_PRINTABLE || point == CHARACTER_DELETE || point > CHARACTER_LAST ||
             (point >= SURROGATE_FIRST && point <= SURROGATE_LAST) || character.control)
    {
      continue; // control characters and Ctrl's commands insert nothing (INP-113)
    }
    else if ((!desc.digitsOnly || (point >= U'0' && point <= U'9')) && (desc.maxLength == 0 || CharacterCount(m_editText) < desc.maxLength))
    {
      AppendUtf8(m_editText, point); // UI-068: a character beyond the maximum length is not inserted
    }
  }
}

void UiCore::Draw(TextSystem& _text, const UiStyle& _style, double _timeSeconds, std::vector<UiBatch>& _out) const
{
  if (!m_metrics.has_value())
  {
    return;
  }
  if (m_page.has_value())
  {
    DrawWindow(*m_page->window, *m_metrics, _text, _style, _timeSeconds, _out);
  }
  for (const Entry& layer : m_layers)
  {
    DrawWindow(*layer.window, *m_metrics, _text, _style, _timeSeconds, _out);
  }
}

void UiCore::DrawWindow(const UiWindow& _window, const UiMetrics& _metrics, TextSystem& _text, const UiStyle& _style, double _timeSeconds,
                        std::vector<UiBatch>& _out) const
{
  const UiMetrics& metrics = _metrics;
  const float outline = metrics.LinePixels(_style.textOutlinePixels);
  // UI-337, UI-474: fill boxes behind everything, then each widget's backgrounds, pictures and text in the order added.
  for (const UiWindow::Widget& widget : _window.m_widgets)
  {
    if (widget.desc.kind == UiWidgetKind::FillBox)
    {
      AppendFillBox(_out, widget.rect, _style, metrics, _timeSeconds);
    }
  }
  for (const UiWindow::Widget& widget : _window.m_widgets)
  {
    const UiWidgetDesc& desc = widget.desc;
    const UiRect& rect = widget.rect;
    const bool highlighted = _window.Highlighted(widget.handle);
    const float pixels = metrics.TextPixels(desc.fontPixels);
    const float captionLeft = rect.left + std::round(_style.captionInset * rect.Width());
    switch (desc.kind)
    {
    case UiWidgetKind::FillBox:
      break;
    case UiWidgetKind::Label:
    {
      TextStyle style = CaptionStyle(_style, _style.labelText, outline);
      const float left = desc.align == UiAlign::Right ? rect.right : rect.left;
      AppendRowText(_out, _text, desc.text, desc.role, pixels, rect, desc.align, left, style);
      break;
    }
    case UiWidgetKind::PageTitle:
    {
      // UI-114: the highlight beam over the title's rectangle, from the top color to the bottom one, outlined.
      if (_style.menuHighlight.has_value())
      {
        UiQuad beam = Quad(rect, _style.beamTop);
        beam.colorTo = _style.beamBottom;
        AppendQuad(_out, UiSource::Image, UiBlend::Alpha, *_style.menuHighlight, beam);
      }
      AppendOutline(_out, rect, metrics.LinePixels(1.0), UiBlend::Alpha, _style.beamOutline);
      // UI-113: upper-cased at the size factor, shrunk to fit the width with its margin, outlined then inked, centered.
      const std::string caption = UpperCased(desc.text);
      float size = pixels * _style.titleSizeFactor;
      TextLine line = _text.Layout(caption, desc.role, size);
      if (line.widthPixels > rect.Width() && line.widthPixels > 0.0f)
      {
        size *= rect.Width() / (line.widthPixels * _style.titleFitMargin);
        line = _text.Layout(caption, desc.role, size);
      }
      TextStyle style;
      style.blend = UiBlend::Ink;
      style.color = _style.titleShadow;
      style.outline = _style.titleOutline;
      style.outlineOffsetPixels = static_cast<std::int32_t>(outline);
      AppendText(_out, line, rect.left + (rect.Width() - line.widthPixels) * 0.5f, RowTextTop(rect.top, rect.Height(), line.heightPixels),
                 style);
      break;
    }
    case UiWidgetKind::PageButton:
    {
      // UI-108: the focus band while highlighted, and the caption outlined, then filled in its state's color.
      if (highlighted)
      {
        AppendQuad(_out, UiSource::Solid, UiBlend::Alpha, 0, Quad(rect, _style.focusBand));
      }
      AppendRowText(_out, _text, desc.text, desc.role, pixels, rect, desc.align, captionLeft,
                    CaptionStyle(_style, CaptionColor(_style, desc, highlighted), outline));
      break;
    }
    case UiWidgetKind::CheckBox:
    {
      // UI-125: the row box, the caption, and UI-081's box and tick, the tick in the caption's color.
      AppendQuad(_out, UiSource::Solid, UiBlend::Alpha, 0, Quad(rect, highlighted ? _style.focusBand : _style.rowBand));
      const float height = rect.Height();
      const float side = std::round(_style.checkBoxSize * height);
      const float boxLeft = std::round(rect.right - _style.checkBoxRight * height);
      const float boxTop = std::round(rect.top + (height - side) * 0.5f);
      const UiRect box{boxLeft, boxTop, boxLeft + side, boxTop + side};
      AppendQuad(_out, UiSource::Solid, UiBlend::Alpha, 0, Quad(box, _style.checkBox));
      AppendOutline(_out, box, metrics.LinePixels(1.0), UiBlend::Alpha, {1.0f, 1.0f, 1.0f, 1.0f});
      if (desc.checked && _style.checkTick.has_value())
      {
        const float inset = std::round(_style.checkTickInset * height);
        const float tick = std::round(_style.checkTickSize * height);
        AppendQuad(_out, UiSource::Image, UiBlend::Alpha, *_style.checkTick,
                   Quad({box.left + inset, box.top + inset, box.left + inset + tick, box.top + inset + tick}, _style.caption));
      }
      AppendRowText(_out, _text, desc.text, desc.role, pixels, rect, UiAlign::Left, captionLeft,
                    CaptionStyle(_style, CaptionColor(_style, desc, highlighted), outline));
      break;
    }
    case UiWidgetKind::TextField:
    {
      // UI-121's first style: the row box, the caption at the caption inset, the value right-aligned one font size in from
      // the right, and while it is edited the typed text and the caret line at its fixed place, blinking (UI-070).
      const bool edited = widget.handle == m_editing;
      AppendQuad(_out, UiSource::Solid, UiBlend::Alpha, 0, Quad(rect, highlighted || edited ? _style.focusBand : _style.rowBand));
      const TextStyle style = CaptionStyle(_style, CaptionColor(_style, desc, highlighted), outline);
      AppendRowText(_out, _text, desc.text, desc.role, pixels, rect, UiAlign::Left, captionLeft, style);
      const std::string& shown = edited ? m_editText : desc.value;
      if (!shown.empty())
      {
        AppendRowText(_out, _text, shown, desc.role, pixels, rect, UiAlign::Right, std::round(rect.right - pixels), style);
      }
      const double period = std::max(static_cast<double>(_style.caretBlinkPeriod), 1e-3);
      if (edited && std::fmod(std::max(_timeSeconds, 0.0), period) < period * static_cast<double>(_style.caretDuty))
      {
        const float caretLeft = std::round(rect.right - _style.fieldCaretOffset * pixels);
        const float margin = metrics.LinePixels(_style.caretMarginPixels);
        AppendQuad(_out, UiSource::Solid, UiBlend::Alpha, 0,
                   Quad({caretLeft, rect.top + margin, caretLeft + metrics.LinePixels(_style.fieldCaretWidthPixels), rect.bottom - margin},
                        CaptionColor(_style, desc, true)));
      }
      break;
    }
    case UiWidgetKind::Cycler:
    {
      // UI-117: the row box, the label at the caption inset, and the value right-aligned one font size in from the right.
      AppendQuad(_out, UiSource::Solid, UiBlend::Alpha, 0, Quad(rect, highlighted ? _style.focusBand : _style.rowBand));
      const TextStyle style = CaptionStyle(_style, CaptionColor(_style, desc, highlighted), outline);
      AppendRowText(_out, _text, desc.text, desc.role, pixels, rect, UiAlign::Left, captionLeft, style);
      if (desc.selected < desc.options.size())
      {
        AppendRowText(_out, _text, desc.options[desc.selected], desc.role, pixels, rect, UiAlign::Right, std::round(rect.right - pixels),
                      style);
      }
      break;
    }
    }
  }
}

std::vector<std::string> ValidateWindow(const UiWindow& _window)
{
  std::vector<std::string> problems;
  std::set<std::string_view> ids;
  for (const UiWindow::Widget& widget : _window.m_widgets)
  {
    if (widget.desc.id.empty())
    {
      problems.push_back(std::format("{}: a widget has no id", _window.Name()));
    }
    else if (!ids.insert(widget.desc.id).second)
    {
      problems.push_back(std::format("{}: the id {} is used twice", _window.Name(), widget.desc.id));
    }
    const bool inOrder = std::ranges::find(_window.m_navigation, widget.handle) != _window.m_navigation.end();
    if (Focusable(widget.desc.kind) && !inOrder)
    {
      problems.push_back(std::format("{}: {} can take the focus but is in no navigation order", _window.Name(), widget.desc.id));
    }
    for (const UiHandle neighbor : {widget.neighbors.up, widget.neighbors.down, widget.neighbors.left, widget.neighbors.right})
    {
      if (neighbor && std::ranges::find(_window.m_navigation, neighbor) == _window.m_navigation.end())
      {
        problems.push_back(std::format("{}: a neighbor of {} is not in the navigation order", _window.Name(), widget.desc.id));
      }
    }
  }
  for (const UiHandle entry : _window.m_navigation)
  {
    const UiWindow::Widget* widget = _window.Get(entry);
    if (widget == nullptr || !Focusable(widget->desc.kind))
    {
      problems.push_back(std::format("{}: a navigation entry is not a focusable widget of the window", _window.Name()));
    }
  }
  if (_window.m_backAction && !_window.Contains(_window.m_backAction))
  {
    problems.push_back(std::format("{}: the back action is not a widget of the window", _window.Name()));
  }
  return problems;
}

} // namespace Neuron
