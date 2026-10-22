#pragma once

#include "TextSystem.h"
#include "UiLayout.h"
#include "WorldScene.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Neuron
{

/// A widget's typed handle (ADR-030): a serial number that the UI core never gives twice in its life, so that the handle
/// of a widget whose window has gone finds nothing (UI-036). The default handle is no widget.
struct UiHandle
{
  std::uint32_t serial = 0;

  [[nodiscard]] explicit operator bool() const noexcept
  {
    return serial != 0;
  }
  bool operator==(const UiHandle& _other) const = default;
};

// The widgets of the page family that M2 builds (ADR-149); the HUD's and the front end's own arrive with T16 and T18.
enum class UiWidgetKind : std::uint8_t
{
  Label,      // UI-426: a line of text, never focusable
  PageTitle,  // UI-113, UI-114: the page's title over the highlight beam, never in the navigation order
  FillBox,    // UI-115: a box behind the widgets, never focusable
  PageButton, // UI-105, UI-108: a caption that clicks
  CheckBox,   // UI-080, UI-125: a caption and a box that a click toggles
  Cycler      // UI-116 in cycle mode, UI-117: a label and a value that a click steps to the next option, wrapping
};

enum class UiAlign : std::uint8_t
{
  Left,
  Center,
  Right
};

/// What a page gives of a widget when it adds it (UI-370's groups as M2 has them: its kind, its id, its flags and its
/// text; its place is set by the page's layout, ADR-149).
struct UiWidgetDesc
{
  UiWidgetKind kind = UiWidgetKind::Label;
  std::string id;                   // unique within its window: the log's, the validator's and the tests' name (UI-372)
  std::string text;                 // the caption, label or title, looked up and formatted (ADR-146)
  std::string accessibleName;       // the caption when empty (ADR-064)
  std::vector<std::string> options; // a cycler's option texts
  std::size_t selected = 0;         // a cycler's option
  bool checked = false;             // a check box's state
  bool inactive = false;            // UI-105: drawn inactive, never acts
  bool alternativeColor = false;    // UI-105: a page button's alternative caption color
  UiAlign align = UiAlign::Left;
  FontRole role = FontRole::Title; // UI-390: the title role for buttons and titles, the body role for rows
  float fontPixels = 20.0f;        // the text's layout size, before the scales and the minimum (UI-171)
};

/// The authored neighbors of a navigation entry (UI-381), which win over the navigation order.
struct UiNeighbors
{
  UiHandle up;
  UiHandle down;
  UiHandle left;
  UiHandle right;
};

/// A page or a layer (UI-304, UI-336): its widgets in the order they were added, its navigation order, its cursor and its
/// back action (UI-050). At M2 every window covers the client area (UI-330).
class UiWindow
{
public:
  [[nodiscard]] std::string_view Name() const noexcept
  {
    return m_name;
  }
  [[nodiscard]] bool IsLayer() const noexcept
  {
    return m_layer;
  }

  UiHandle Add(UiWidgetDesc _desc);
  // UI-050, UI-381: the navigation order; the cursor moves to the initial focus, or to the first entry (UI-385).
  void SetNavigation(std::vector<UiHandle> _order, UiHandle _initialFocus = {});
  void SetNeighbors(UiHandle _widget, const UiNeighbors& _neighbors);
  // UI-051, UI-209: the widget whose click the close action runs.
  void SetBackAction(UiHandle _widget);
  void SetRect(UiHandle _widget, const UiRect& _rect);
  void SetText(UiHandle _widget, std::string _text);
  void SetFontPixels(UiHandle _widget, float _pixels);
  void SetChecked(UiHandle _widget, bool _checked);
  void SetSelected(UiHandle _widget, std::size_t _option);
  void SetInactive(UiHandle _widget, bool _inactive);
  // UI-053: the cursor moves to _widget when it is in the navigation order, and stays otherwise.
  void SetCursor(UiHandle _widget);

  [[nodiscard]] UiHandle Find(std::string_view _id) const noexcept;
  [[nodiscard]] bool Contains(UiHandle _widget) const noexcept;
  [[nodiscard]] bool Checked(UiHandle _widget) const noexcept;
  [[nodiscard]] std::size_t Selected(UiHandle _widget) const noexcept;
  [[nodiscard]] UiRect RectOf(UiHandle _widget) const noexcept;
  [[nodiscard]] UiHandle Cursor() const noexcept;
  [[nodiscard]] UiHandle BackAction() const noexcept
  {
    return m_backAction;
  }
  [[nodiscard]] std::span<const UiHandle> Navigation() const noexcept
  {
    return m_navigation;
  }
  // UI-056: drawn highlighted, as its window's cursor or under the pointer in hover-highlight mode.
  [[nodiscard]] bool Highlighted(UiHandle _widget) const noexcept;

private:
  friend class UiCore;
  friend std::vector<std::string> ValidateWindow(const UiWindow& _window);

  struct Widget
  {
    UiHandle handle;
    UiWidgetDesc desc;
    UiRect rect;
    UiNeighbors neighbors;
  };

  UiWindow(std::string _name, bool _layer, std::function<UiHandle()> _nextHandle);
  [[nodiscard]] Widget* Get(UiHandle _widget) noexcept;
  [[nodiscard]] const Widget* Get(UiHandle _widget) const noexcept;
  [[nodiscard]] bool Acts(UiHandle _widget) const noexcept;

  std::string m_name;
  bool m_layer = false;
  std::function<UiHandle()> m_nextHandle;
  std::vector<Widget> m_widgets;
  std::vector<UiHandle> m_navigation;
  std::size_t m_cursor = 0;
  UiHandle m_backAction;
  UiHandle m_hover;         // the widget under the pointer (UI-032)
  bool m_hoverMode = false; // UI-056: hover-highlight mode, which navigation ends
};

// What happened to a widget in a frame's update, in the order it happened (UI-035, UI-080, UI-116).
enum class UiEventKind : std::uint8_t
{
  Click,  // a click or an activation: a page button acts, a check box has toggled, a cycler has stepped
  Changed // a check box's or a cycler's value changed
};

struct UiEvent
{
  UiEventKind kind = UiEventKind::Click;
  UiHandle widget;
};

using UiPageId = std::uint16_t;

/// A change of the page stack (UI-331), applied at the start of the next frame (UI-351).
struct UiPageRequest
{
  enum class Kind : std::uint8_t
  {
    Push,    // onto the stack; a page already on it is unwound to
    Pop,     // the top page; refused on a stack of one
    Replace, // the top page; a page already on the stack is unwound to
    UnwindTo // pop to the page; a page not on the stack gives the root and the page
  };

  Kind kind = Kind::Push;
  UiPageId page = 0;
};

class UiPage;

/// What a page may ask of the UI core in its update (UI-310): a page change, or a layer opened or closed (UI-336).
class UiNavigator
{
public:
  virtual ~UiNavigator() = default;

  virtual void Request(const UiPageRequest& _request) = 0;
  virtual void OpenLayer(std::unique_ptr<UiPage> _layer) = 0;
  // The top layer closes at the start of the next frame.
  virtual void CloseLayer() = 0;
};

/// A page or a layer of the game's (UI-450's recipe in its M2 form, ADR-149): it builds its widgets when it enters,
/// places them whenever the layout changes, and handles its events and the close action in its update.
class UiPage
{
public:
  virtual ~UiPage() = default;

  [[nodiscard]] virtual std::string_view Name() const = 0;
  // UI-360: the widgets, their navigation order and the back action.
  virtual void Enter(UiWindow& _window) = 0;
  // UI-172, UI-174: every widget's rectangle for _metrics; nothing is created or removed.
  virtual void Layout(UiWindow& _window, const UiMetrics& _metrics) = 0;
  // UI-352 step 4: the page's update hook, after navigation, with its window's events of the frame.
  virtual void Update(UiWindow& _window, std::span<const UiEvent> _events, UiNavigator& _navigator);
  // UI-383: the close action when the window has no back action: a layer closes, a page pops. The root page overrides
  // it to ignore the action.
  virtual void Back(UiWindow& _window, UiNavigator& _navigator);
};

/// The UI's actions of a frame (UI-203, UI-204, UI-006), which the client reads from its input router.
struct UiInput
{
  bool up = false; // action.menu.up, its repeats included
  bool down = false;
  bool left = false;
  bool right = false;
  bool activate = false; // action.menu.activate
  bool close = false;    // action.menu.close
  float pointerX = 0.0f; // client pixels
  float pointerY = 0.0f;
  bool pointerMoved = false;
  bool pointerDown = false; // action.ui.pointer_down
  bool pointerHeld = false; // action.ui.pointer_held
  bool pointerUp = false;   // the primary button's release
  bool windowFocused = true;
};

/// How the page family draws (UI-187, UI-188, UI-108 to UI-125, ADR-149): the palette's colors, the images' indices in
/// the UI's set, which the game takes from its records and its content, and the measures of the widgets.
struct UiStyle
{
  using Color = std::array<float, 4>;

  Color textOutline{1.0f, 1.0f, 1.0f, 0.0f};
  Color caption{0.6f, 1.0f, 0.4f, 1.0f};         // ui.palette.fe_caption
  Color captionFocus{1.0f, 0.3f, 0.3f, 1.0f};    // ui.palette.fe_caption_focus
  Color captionInactive{0.5f, 0.5f, 0.5f, 1.0f}; // ui.palette.fe_caption_inactive
  Color captionAlternative{1.0f, 1.0f, 0.3f, 1.0f};
  Color focusBand{0.75f, 0.75f, 1.0f, 0.5f};
  Color rowBand{1.0f, 1.0f, 1.0f, 0.1f};
  Color titleOutline{0.75f, 1.0f, 0.45f, 1.0f};
  Color titleShadow{1.0f, 0.9f, 0.9f, 0.0f};
  Color beamTop{0.6f, 1.0f, 0.4f, 0.8f};
  Color beamBottom{0.3f, 0.5f, 0.2f, 0.7f};
  Color beamOutline{0.0f, 0.0f, 0.0f, 0.5f};
  Color boxTint{0.8f, 0.8f, 1.0f, 0.9f};
  Color boxGlow{0.3f, 0.6f, 0.9f, 0.4f};
  Color boxOutline{0.8f, 0.7f, 1.0f, 0.8f};
  float boxInnerAlpha = 0.4f;
  Color boxShadowEdge{1.0f, 1.0f, 1.0f, 0.0f};
  Color checkBox{0.3f, 0.3f, 0.3f, 1.0f};
  Color labelText{1.0f, 1.0f, 1.0f, 1.0f}; // ui.palette.text_normal: a label's text, additive

  std::optional<std::uint16_t> menuHighlight; // uitex.menu_highlight: the highlight beam (UI-114)
  std::optional<std::uint16_t> fillBlur;      // uitex.fill_blur: a fill box's body (UI-115)
  std::optional<std::uint16_t> glowCloudy;    // fx.glow_cloudy: a fill box's drifting glow (UI-115)
  std::optional<std::uint16_t> checkTick;     // uitex.icon_check: a check box's tick (UI-081)

  float captionInset = 0.05f;                    // ui.page_button.caption_inset, × the widget's width
  float titleSizeFactor = 1.5f;                  // ui.page_title.size_factor
  float titleFitMargin = 1.05f;                  // ui.page_title.fit_margin
  float boxShadowWidth = 0.01f;                  // ui.fill_box.shadow_width, × W
  float boxBlurTilePixels = 64.0f;               // ui.fill_box.blur_tile
  std::array<float, 2> glowMotion{0.01f, 0.02f}; // ui.fill_box.glow_motion
  float boxOutlinePixels = 2.0f;                 // ui.fill_box.outline_width
  float boxInnerInsetPixels = 3.0f;              // ui.fill_box.inner_inset
  float textOutlinePixels = 1.0f;                // ui.text.outline_offset (UI-169, UI-192)
  float checkBoxSize = 0.75f;                    // ui.check.box_size, × the row's height (UI-081)
  float checkBoxRight = 1.5f;                    // ui.check.box_right, × the row's height from its right edge
  float checkTickInset = 0.05f;                  // ui.check.tick_inset, × the row's height
  float checkTickSize = 0.65f;                   // ui.check.tick_size, × the row's height
};

/// The UI core (ADR-030 to ADR-033 at M2, ADR-149): the screen's page stack and the layers above it, the focus, the
/// navigation and the pointer of the focused window, and the drawing of every window. Main thread only.
class UiCore final : private UiNavigator
{
public:
  using PageFactory = std::function<std::unique_ptr<UiPage>(UiPageId)>;

  explicit UiCore(PageFactory _pages);
  ~UiCore() override;

  UiCore(const UiCore&) = delete;
  UiCore& operator=(const UiCore&) = delete;

  // UI-332: a screen whose stack is its root page, from the next frame; CloseScreen removes every page and layer then.
  void OpenScreen(UiPageId _root);
  void CloseScreen();
  // UI-310, UI-350: the one pending page change; the last request of a frame wins.
  void Request(const UiPageRequest& _request) override;

  // UI-352: the frame's update. The pending changes first (UI-351), then a relayout when the metrics changed (UI-172),
  // then the pointer and the navigation of the focused window (UI-031 to UI-035, UI-051), then each window's hook.
  void Update(const UiInput& _input, const UiMetrics& _metrics);
  // Every window, bottom to top (UI-337), into _out. Drawing never changes state (UI-021).
  void Draw(TextSystem& _text, const UiStyle& _style, double _timeSeconds, std::vector<UiBatch>& _out) const;

  [[nodiscard]] std::span<const UiPageId> Stack() const noexcept
  {
    return m_stack;
  }
  // The window that takes input: the top layer, else the top page; none without either.
  [[nodiscard]] const UiWindow* Focused() const noexcept;
  [[nodiscard]] UiWindow* Focused() noexcept;
  // UI-006: whether a window lies under the pointer, so that a press there is withheld from the world's actions. Every
  // M2 window covers the client area, so it is whether a window exists.
  [[nodiscard]] bool TakesPointer() const noexcept;
  // The events of the last update, of the focused window.
  [[nodiscard]] std::span<const UiEvent> Events() const noexcept
  {
    return m_events;
  }

private:
  struct Entry
  {
    std::unique_ptr<UiPage> page;
    std::unique_ptr<UiWindow> window;
  };

  void OpenLayer(std::unique_ptr<UiPage> _layer) override;
  void CloseLayer() override;

  void ApplyPending();
  void ApplyPageRequest(const UiPageRequest& _request);
  void EnterTop();
  [[nodiscard]] Entry Make(std::unique_ptr<UiPage> _page, bool _layer);
  void Navigate(UiWindow& _window, UiPage& _page, const UiInput& _input);
  void Point(UiWindow& _window, const UiInput& _input);
  void Click(UiWindow& _window, UiHandle _widget);
  [[nodiscard]] Entry* FocusedEntry() noexcept;
  [[nodiscard]] const Entry* FocusedEntry() const noexcept;
  void DrawWindow(const UiWindow& _window, const UiMetrics& _metrics, TextSystem& _text, const UiStyle& _style, double _timeSeconds,
                  std::vector<UiBatch>& _out) const;

  PageFactory m_pages;
  std::vector<UiPageId> m_stack;
  std::optional<Entry> m_page; // the top page's
  std::vector<Entry> m_layers; // bottom to top
  std::optional<UiPageRequest> m_pendingRequest;
  std::optional<UiPageId> m_pendingScreen;
  bool m_pendingClose = false;
  std::vector<std::unique_ptr<UiPage>> m_pendingLayers;
  std::size_t m_pendingLayerCloses = 0;
  std::optional<UiMetrics> m_metrics;
  std::uint32_t m_nextSerial = 1;
  UiHandle m_captured; // UI-031: the widget the press began on, in the focused window
  std::vector<UiEvent> m_events;
};

// UI-372's checks as M2 runs them in its tests (ADR-149): every id unique and present, every navigation entry a
// focusable widget of the window, every focusable widget in the order, and every authored neighbor in the order. The
// problems found, one line each; none when the window passes.
[[nodiscard]] std::vector<std::string> ValidateWindow(const UiWindow& _window);

} // namespace Neuron
