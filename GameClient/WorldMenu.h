#pragma once

#include "AwayMessages.h"
#include "ClientSession.h"
#include "MessageLocale.h"
#include "RelationMessages.h"
#include "StringTable.h"
#include "TuningMvp.h"
#include "TuningPresentation.h"
#include "UiCore.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Species
{

/// What the steward chose on the world's menu (ADR-170; ADR-083 item 3): back to the world, leaving to the front end, or
/// quitting.
struct WorldMenuChoice
{
  enum class Kind : std::uint8_t
  {
    Resume,
    Leave,
    Quit
  };

  Kind kind = Kind::Resume;
};

// The world's pages, by their UI page ids (GamePage's World entries).
enum class WorldMenuPage : std::uint8_t
{
  Menu,
  Epilogue,
  AwayReport // MVP-074 (ADR-187)
};

// ADR-187: the most lines of each section of the away report's page; the rest are counted on its last line.
inline constexpr std::size_t AWAY_PAGE_LINES_MAX = 8;

/// A part of the epilogue (ADR-170): its title, which names its row on the page, and its lines, each short enough for the
/// page's right box in the small font.
struct EpilogueSection
{
  std::string title;
  std::vector<std::string> lines;
};

/// The epilogue's text (MVP-063, ADR-170): the final standing, two lines a settlement, then each settlement's history in
/// the standing's order (its battles, claims, structures and grievances, Workers and strength over the run), the steward's
/// own led by its statistics (WLD-224).
struct EpilogueText
{
  std::vector<EpilogueSection> sections;
};

// MVP-074 (ADR-187): _report's text in the epilogue's form: its summary, then the deputy's actions with the standing order
// of each (PIL-026), at most AWAY_PAGE_LINES_MAX lines a section; its ticks _tickSeconds long.
[[nodiscard]] EpilogueText AwayReportTextOf(const AwayReport& _report, double _tickSeconds, const Neuron::StringTable& _strings,
                                            const Neuron::MessageLocale& _locale);

// MVP-063 (ADR-170): _epilogue's text in _strings, its ticks _tickSeconds long.
[[nodiscard]] EpilogueText EpilogueTextOf(const Epilogue& _epilogue, double _tickSeconds, const Neuron::StringTable& _strings,
                                          const Neuron::MessageLocale& _locale);

/// The world's menu and its epilogue (M3 task T14, ADR-170; MVP-063, ADR-083 item 3): the menu page with Return to the
/// world, the epilogue, Leave to the front end and Quit, which the close action opens over the world; and the epilogue's
/// page, which opens by itself once the run has ended and its epilogue has come. It asks the server for the epilogue once
/// the valley status says the run has ended, and holds the steward's choice until the client takes it. Main thread only.
class WorldMenu
{
public:
  struct Desc
  {
    const TuningPresentation* presentation = nullptr; // each outliving the menu
    const TuningMvp* tuning = nullptr;
    const Neuron::StringTable* strings = nullptr;
    const Neuron::MessageLocale* locale = nullptr;
  };

  explicit WorldMenu(const Desc& _desc);

  // Once a frame in a world, after the session's update: the run's end from the valley status, the request for the
  // epilogue, and the epilogue when it comes.
  void Update(ClientSession& _session);
  // A new world: nothing ended, asked or kept.
  void Reset() noexcept;

  // The run has ended, by the valley status or the epilogue.
  [[nodiscard]] bool RunEnded() const noexcept
  {
    return m_ended;
  }
  // The epilogue of the ended run; none before it came.
  [[nodiscard]] const std::optional<Epilogue>& Ended() const noexcept
  {
    return m_epilogue;
  }
  // The epilogue of the ended run came since the last call, so that its page opens once.
  [[nodiscard]] bool TakeEpilogueArrived() noexcept;
  // MVP-074 (ADR-187): the latest away report of an absence; none in a world the steward was never away from.
  [[nodiscard]] const std::optional<AwayReport>& Away() const noexcept
  {
    return m_away;
  }
  // O109: a return's away report with news came since the last call, so that its page opens first.
  [[nodiscard]] bool TakeAwayReportArrived() noexcept;
  // The page of _page's id, for the UI core's factory.
  [[nodiscard]] std::unique_ptr<Neuron::UiPage> MakePage(WorldMenuPage _page);
  // The steward's choice since the last call.
  [[nodiscard]] std::optional<WorldMenuChoice> TakeChoice() noexcept;
  void Choose(const WorldMenuChoice& _choice) noexcept
  {
    m_choice = _choice;
  }
  // A string of the table, formatted (ADR-146).
  [[nodiscard]] std::string Text(std::string_view _id, std::span<const Neuron::FormatArgument> _arguments = {}) const;
  [[nodiscard]] const Desc& Settings() const noexcept
  {
    return m_desc;
  }
  // The steward's site, from the session's settlement; none before it.
  [[nodiscard]] std::optional<std::uint8_t> Site() const noexcept
  {
    return m_site;
  }

private:
  Desc m_desc;
  std::optional<std::uint8_t> m_site;
  bool m_ended = false;
  bool m_requested = false;
  bool m_arrived = false;
  std::optional<Epilogue> m_epilogue;
  std::optional<AwayReport> m_away;
  bool m_awayAsked = false;
  bool m_awayArrived = false;
  std::optional<WorldMenuChoice> m_choice;
};

} // namespace Species
