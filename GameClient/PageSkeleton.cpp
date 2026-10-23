#include "pch.h"

#include "PageSkeleton.h"
#include "UiLook.h"

#include <utility>

namespace Species
{

namespace
{

constexpr float ROW_TITLE_ROWS = 1.8f; // R-META-201: the first row half a button and a title below the box's top
constexpr float NOTICE_ROWS = 2.0f;    // a notice spans two rows of the body text

} // namespace

PageSkeleton::PageSkeleton(const TuningPresentation& _presentation, std::string_view _name) noexcept
  : m_presentation(_presentation),
    m_name(_name)
{
}

void PageSkeleton::Layout(Neuron::UiWindow& _window, const Neuron::UiMetrics& _metrics)
{
  const Neuron::PageGrid grid = Neuron::ComputePageGrid(_metrics.client, PageGridKeysOf(m_presentation));
  const auto& layout = m_presentation.ref.meta.layout;
  _window.SetRect(m_titleBar, grid.titleBar);
  _window.SetRect(m_leftBox, grid.leftBox);
  _window.SetRect(m_rightBox, grid.rightBox);
  const Neuron::UiRect& left = grid.leftBox;
  const auto inset = static_cast<float>(layout.titleInset);
  _window.SetRect(m_title, Neuron::UiRect::FromSize(left.left + inset, left.top + inset, left.Width() - 2.0f * inset,
                                                    static_cast<float>(layout.titleHeight) * grid.buttonHeight));
  _window.SetFontPixels(m_title, grid.fontMedium);
  float y = left.top + ROW_TITLE_ROWS * grid.buttonHeight + grid.ButtonStep();
  for (const Neuron::UiHandle row : m_rows)
  {
    _window.SetRect(row, Neuron::UiRect::FromSize(grid.ButtonColumnLeft(), y, grid.buttonWidth, grid.buttonHeight));
    _window.SetFontPixels(row, grid.fontMedium);
    y += grid.ButtonStep();
  }
  _window.SetRect(m_notice, Neuron::UiRect::FromSize(grid.ButtonColumnLeft(), y, grid.buttonWidth, NOTICE_ROWS * grid.buttonHeight));
  _window.SetFontPixels(m_notice, grid.fontSmall);
  if (m_back)
  {
    _window.SetRect(m_back, Neuron::UiRect::FromSize(grid.ButtonColumnLeft(),
                                                     left.bottom - static_cast<float>(layout.backOffset) * grid.buttonHeight,
                                                     grid.buttonWidth, grid.buttonHeight));
    _window.SetFontPixels(m_back, grid.fontMedium);
  }
  LayoutMore(_window, grid);
}

void PageSkeleton::Begin(Neuron::UiWindow& _window, std::string _title)
{
  m_titleBar = Add(_window, Neuron::UiWidgetKind::FillBox, "title_bar", {});
  m_leftBox = Add(_window, Neuron::UiWidgetKind::FillBox, "left_box", {});
  m_rightBox = Add(_window, Neuron::UiWidgetKind::FillBox, "right_box", {});
  m_title = Add(_window, Neuron::UiWidgetKind::PageTitle, "title", std::move(_title));
}

Neuron::UiHandle PageSkeleton::Row(Neuron::UiWindow& _window, Neuron::UiWidgetDesc _desc)
{
  const Neuron::UiHandle row = _window.Add(std::move(_desc));
  m_rows.push_back(row);
  return row;
}

Neuron::UiHandle PageSkeleton::Row(Neuron::UiWindow& _window, Neuron::UiWidgetKind _kind, std::string _id, std::string _text)
{
  Neuron::UiWidgetDesc desc;
  desc.kind = _kind;
  desc.id = std::move(_id);
  desc.text = std::move(_text);
  return Row(_window, std::move(desc));
}

void PageSkeleton::Finish(Neuron::UiWindow& _window, std::optional<std::string> _back, Neuron::UiHandle _initialFocus)
{
  Neuron::UiWidgetDesc notice;
  notice.id = "notice";
  notice.role = Neuron::FontRole::Body;
  m_notice = _window.Add(std::move(notice));
  std::vector<Neuron::UiHandle> order = m_rows;
  if (_back.has_value())
  {
    m_back = Add(_window, Neuron::UiWidgetKind::PageButton, "back", std::move(*_back));
    _window.SetBackAction(m_back);
    order.push_back(m_back);
  }
  _window.SetNavigation(std::move(order), _initialFocus);
}

Neuron::UiHandle PageSkeleton::Add(Neuron::UiWindow& _window, Neuron::UiWidgetKind _kind, std::string _id, std::string _text)
{
  Neuron::UiWidgetDesc desc;
  desc.kind = _kind;
  desc.id = std::move(_id);
  desc.text = std::move(_text);
  return _window.Add(std::move(desc));
}

} // namespace Species
