#pragma once

#include "ContinueFault.h"
#include "MessageLocale.h"
#include "SettingsStore.h"
#include "StringTable.h"
#include "TuningMvp.h"
#include "TuningPresentation.h"
#include "UiCore.h"
#include "WorldSaves.h"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace Species
{

/// What the steward chose on the front end (MVP-103).
struct FrontEndChoice
{
  enum class Kind : std::uint8_t
  {
    Continue, // the latest world (SetLatest)
    NewWorld, // a new world with its seed and site
    Quit,
    Epilogue // ADR-170: an ended world, opened on its epilogue
  };

  Kind kind = Kind::Quit;
  std::optional<std::uint64_t> seed;   // NewWorld: the seed typed; none: the server draws one (MVP-005)
  std::uint8_t site = 0;               // NewWorld: the steward's site (MVP-012)
  std::uint8_t aiSettlements = 0;      // NewWorld: its AI settlements, AI_SETTLEMENTS_FULL or none (MVP-013, O103)
  std::optional<SavedWorld> world;     // Epilogue: the ended world
  WorldPace pace = WorldPace::Session; // NewWorld: its pace (MVP-071, ADR-191)
};

// The front end's pages, by their UI page ids (GamePage's FrontEnd entries).
enum class FrontEndPage : std::uint8_t
{
  Main,
  NewWorld,
  Options,
  PastRuns // ADR-170
};

// The seed field's digits at most: a 64-bit seed's (MVP-005).
inline constexpr std::size_t SEED_DIGITS_MAX = 20;
// The master volume's step on the options page, in percent (O96).
inline constexpr std::int64_t VOLUME_STEP = 10;
// ADR-170: the ended worlds the past runs' page lists at most, the newest first.
inline constexpr std::size_t PAST_RUNS_MAX = 6;

/// MVP-103's front end at M2 (M2 task T18, ADR-155, O80, O96): the main page with Continue, New world, Options and Quit,
/// and from M3 task T14 (ADR-170) Past runs, whose page opens an ended world on its epilogue;
/// the new world's page with its seed field and its site; and the options page with the window mode, anti-aliasing,
/// vertical sync and the master volume, which it writes to the settings store as they change and saves when the page
/// is left. Its pages are the UI core's (ADR-149), built on the page grid of UI-163 with the strings of en-US. It holds
/// the steward's choice until the client takes it. Main thread only.
class FrontEnd
{
public:
  struct Desc
  {
    const TuningPresentation* presentation = nullptr; // each outliving the front end
    const TuningMvp* tuning = nullptr;                // the new world's default site (mvp.sandbox.steward_site)
    const Neuron::StringTable* strings = nullptr;
    const Neuron::MessageLocale* locale = nullptr;
    Neuron::SettingsStore* settings = nullptr;
    bool savesAvailable = true; // PLT-017: the saves folder exists, so worlds are kept
  };

  explicit FrontEnd(const Desc& _desc);

  // The world Continue opens; none: Continue is inactive (MVP-103). The main page reads it as it enters.
  void SetLatest(std::optional<SavedWorld> _latest);
  // ADR-155: why the last Continue failed, shown on the main page until the steward chooses again.
  void SetContinueFault(std::optional<ContinueFault> _fault);
  // MVP-063 (ADR-170): the worlds whose run has ended, the newest first, whose epilogues the past runs' page opens.
  void SetEnded(std::vector<SavedWorld> _ended);
  [[nodiscard]] const std::vector<SavedWorld>& Ended() const noexcept
  {
    return m_ended;
  }
  // The page of _page's id, for the UI core's factory; none for another id.
  [[nodiscard]] std::unique_ptr<Neuron::UiPage> MakePage(FrontEndPage _page);

  // The steward's choice since the last call.
  [[nodiscard]] std::optional<FrontEndChoice> TakeChoice() noexcept;
  // O96: the window mode or the vertical sync changed since the last call, which the client applies at once.
  [[nodiscard]] bool TakeDisplayChange() noexcept;

  [[nodiscard]] const Desc& Settings() const noexcept
  {
    return m_desc;
  }
  [[nodiscard]] const std::optional<SavedWorld>& Latest() const noexcept
  {
    return m_latest;
  }
  [[nodiscard]] const std::optional<ContinueFault>& LastFault() const noexcept
  {
    return m_fault;
  }
  // A string of the table, formatted (ADR-146).
  [[nodiscard]] std::string Text(std::string_view _id, std::span<const Neuron::FormatArgument> _arguments = {}) const;
  void Choose(const FrontEndChoice& _choice);
  void DisplayChanged() noexcept
  {
    m_displayChanged = true;
  }

private:
  Desc m_desc;
  std::optional<SavedWorld> m_latest;
  std::vector<SavedWorld> m_ended;
  std::optional<ContinueFault> m_fault;
  std::optional<FrontEndChoice> m_choice;
  bool m_displayChanged = false;
};

// MVP-005: the seed of the seed field's text: none for an empty field, which the server draws; an error for a number
// beyond 64 bits or a text that is not digits.
[[nodiscard]] std::expected<std::optional<std::uint64_t>, std::monostate> SeedOfText(std::string_view _text);

} // namespace Species
