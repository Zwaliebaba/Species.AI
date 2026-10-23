#pragma once

#include "TuningPresentation.h"
#include "UiCore.h"
#include "UiLayout.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Species
{

/// The page skeleton of UI-373 at M2 (ADR-155; shared from M3 task T14, ADR-170): the page grid's title bar and boxes, the
/// page title in the left box, a column of rows in the medium font, a notice in the body role below them, and Back at
/// its offset above the box's bottom. A page builds it in its Enter with Begin, its rows, then Finish.
class PageSkeleton : public Neuron::UiPage
{
public:
  // _presentation outlives the page; _name is a literal.
  PageSkeleton(const TuningPresentation& _presentation, std::string_view _name) noexcept;

  [[nodiscard]] std::string_view Name() const override
  {
    return m_name;
  }
  void Layout(Neuron::UiWindow& _window, const Neuron::UiMetrics& _metrics) override;

protected:
  // The skeleton with its title; the page adds its rows, then Finish.
  void Begin(Neuron::UiWindow& _window, std::string _title);
  Neuron::UiHandle Row(Neuron::UiWindow& _window, Neuron::UiWidgetDesc _desc);
  Neuron::UiHandle Row(Neuron::UiWindow& _window, Neuron::UiWidgetKind _kind, std::string _id, std::string _text);
  // The notice and, with _back, the Back button of that caption as the back action; the navigation order is the rows,
  // then Back.
  void Finish(Neuron::UiWindow& _window, std::optional<std::string> _back, Neuron::UiHandle _initialFocus = {});
  static Neuron::UiHandle Add(Neuron::UiWindow& _window, Neuron::UiWidgetKind _kind, std::string _id, std::string _text);
  // ADR-170: called at the end of each layout with its page grid, for a page that places more.
  virtual void LayoutMore(Neuron::UiWindow& /*_window*/, const Neuron::PageGrid& /*_grid*/) {}

  const TuningPresentation& m_presentation;
  Neuron::UiHandle m_notice;
  Neuron::UiHandle m_back;

private:
  std::string_view m_name;
  Neuron::UiHandle m_titleBar;
  Neuron::UiHandle m_leftBox;
  Neuron::UiHandle m_rightBox;
  Neuron::UiHandle m_title;
  std::vector<Neuron::UiHandle> m_rows;
};

} // namespace Species
