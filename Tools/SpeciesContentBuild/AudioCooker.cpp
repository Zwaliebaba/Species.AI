#include "pch.h"

#include "AudioCooker.h"
#include "ContentKey.h"
#include "SoundAsset.h"
#include "WavFile.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <format>
#include <functional>
#include <map>
#include <numbers>
#include <string>

namespace ContentBuild
{

namespace
{

constexpr std::string_view STAND_IN_PRIORITY = "mvp";
constexpr std::string_view CONTENT_PREFIX = "GameData/";
constexpr std::string_view SYNTHESIZED = "synthesised tone"; // the manifest's placeholder column, as it is spelled there
constexpr std::array<std::string_view, 4> SOUND_CATEGORIES{"Audio by source and event", "UI sounds", "Ambience", "Music"};
constexpr std::uint32_t AUTHORED_INTEGER_BITS = 24; // AST-079: 24-bit integer or 32-bit float PCM

constexpr double RATE = static_cast<double>(Neuron::SOUND_SAMPLE_RATE);
constexpr double TAU = 2.0 * std::numbers::pi;
constexpr double PEAK = 0.891;             // a stand-in's peak, -1 dBFS
constexpr double END_FADE_SECONDS = 0.004; // the last moments of a stand-in fade to silence, so that it ends without a click
constexpr double INT16_SCALE = 32'767.0;

// --- The synthesizer's parts -----------------------------------------------------------------------------------------

/// SplitMix64 over the stand-in's seed: the noise of a recipe, the same on every build (AST-084).
class Noise
{
public:
  explicit Noise(std::uint64_t _seed) noexcept
    : m_state(_seed)
  {
  }

  // A value from -1 to 1.
  double Next() noexcept
  {
    m_state += 0x9E37'79B9'7F4A'7C15ULL;
    std::uint64_t mixed = m_state;
    mixed = (mixed ^ (mixed >> 30U)) * 0xBF58'476D'1CE4'E5B9ULL;
    mixed = (mixed ^ (mixed >> 27U)) * 0x94D0'49BB'1331'11EBULL;
    mixed ^= mixed >> 31U;
    return static_cast<double>(mixed >> 11U) * 0x1.0p-52 - 1.0;
  }

private:
  std::uint64_t m_state;
};

/// A one-pole low-pass filter at a cutoff frequency in hertz.
class LowPass
{
public:
  explicit LowPass(double _cutoffHertz) noexcept
    : m_coefficient(1.0 - std::exp(-TAU * _cutoffHertz / RATE))
  {
  }

  double Next(double _input) noexcept
  {
    m_output += m_coefficient * (_input - m_output);
    return m_output;
  }

private:
  double m_coefficient;
  double m_output = 0.0;
};

// A stand-in's member: its number from 0 and the group's count, from the row's `variants` ("file 3 of 10", "single file").
struct Member
{
  std::uint32_t index = 0;
  std::uint32_t count = 1;
};

Member MemberOf(std::string_view _variants)
{
  Member member;
  constexpr std::string_view FILE = "file ";
  constexpr std::string_view OF = " of ";
  const std::size_t of = _variants.find(OF);
  if (!_variants.starts_with(FILE) || of == std::string_view::npos)
  {
    return member;
  }
  std::uint32_t number = 0;
  std::uint32_t count = 0;
  const std::string_view first = _variants.substr(FILE.size(), of - FILE.size());
  const std::string_view second = _variants.substr(of + OF.size());
  if (std::from_chars(first.data(), first.data() + first.size(), number).ec == std::errc{} &&
      std::from_chars(second.data(), second.data() + second.size(), count).ec == std::errc{} && number >= 1 && number <= count)
  {
    member = {number - 1, count};
  }
  return member;
}

// FNV-1a over the asset id, mixed with the member's number: each member's own noise (AST-084).
std::uint64_t SeedOf(std::string_view _assetId, std::uint32_t _member) noexcept
{
  std::uint64_t hash = 0xCBF2'9CE4'8422'2325ULL;
  for (const char letter : _assetId)
  {
    hash = (hash ^ static_cast<std::uint8_t>(letter)) * 0x0000'0100'0000'01B3ULL;
  }
  return hash ^ (static_cast<std::uint64_t>(_member + 1) * 0x9E37'79B9'7F4A'7C15ULL);
}

std::vector<double> Silence(double _seconds)
{
  return std::vector<double>(static_cast<std::size_t>(std::lround(_seconds * RATE)), 0.0);
}

double TimeOf(std::size_t _sample) noexcept
{
  return static_cast<double>(_sample) / RATE;
}

// An attack that rises in about _rise seconds, never stepping.
double Rise(double _time, double _rise) noexcept
{
  return 1.0 - std::exp(-_time / _rise);
}

// A struck bell's partials (ratio, level, decay per second), started at _start.
struct Partial
{
  double ratio;
  double level;
  double decay;
};

void AddBell(std::vector<double>& _out, double _start, double _frequency, double _level, std::span<const Partial> _partials)
{
  for (std::size_t sample = static_cast<std::size_t>(std::lround(_start * RATE)); sample < _out.size(); ++sample)
  {
    const double time = TimeOf(sample) - _start;
    double value = 0.0;
    for (const Partial& partial : _partials)
    {
      value += partial.level * std::exp(-time * partial.decay) * std::sin(TAU * _frequency * partial.ratio * time);
    }
    _out[sample] += _level * Rise(time, 0.0015) * value;
  }
}

// A brass voice's sample at its phase: harmonics falling as 1/n^tilt, the upper ones opening as the note gets louder.
double Brass(double _phase, double _loudness, int _harmonics, double _tilt) noexcept
{
  double value = 0.0;
  for (int harmonic = 1; harmonic <= _harmonics; ++harmonic)
  {
    const auto order = static_cast<double>(harmonic);
    value += std::pow(order, -_tilt) * std::pow(std::max(_loudness, 1e-6), 0.12 * (order - 1.0)) * std::sin(order * _phase);
  }
  return value;
}

// A brass chord of _frequencies from _start to _end: its attack, its decay per second and its release before _end.
void AddBrassChord(std::vector<double>& _out, double _start, double _end, std::span<const double> _frequencies, double _attack,
                   double _decay, double _release, double _level)
{
  std::vector<double> phases(_frequencies.size(), 0.0);
  const auto first = static_cast<std::size_t>(std::lround(_start * RATE));
  const auto last = std::min(_out.size(), static_cast<std::size_t>(std::lround(_end * RATE)));
  for (std::size_t sample = first; sample < last; ++sample)
  {
    const double time = TimeOf(sample) - _start;
    const double left = _end - _start - time;
    const double loudness = Rise(time, _attack) * std::exp(-time * _decay) * std::clamp(left / _release, 0.0, 1.0);
    double value = 0.0;
    for (std::size_t note = 0; note < _frequencies.size(); ++note)
    {
      phases[note] += TAU * _frequencies[note] / RATE;
      value += Brass(phases[note], loudness, 10, 1.2);
    }
    _out[sample] += _level * loudness * value / static_cast<double>(_frequencies.size());
  }
}

// --- The recipes of MVP-087's groups (O98) ---------------------------------------------------------------------------

// sfx.weapon.laser_bolt.spawn: a bright frequency-modulated zap falling fast, over a short click.
std::vector<double> LaserBolt(Member _member, Noise& _noise)
{
  const double seconds = 0.24 + 0.035 * _member.index;
  const double from = 2'600.0 - 140.0 * (_member.index % 5);
  const double to = 420.0 + 55.0 * (_member.index % 4);
  std::vector<double> out = Silence(seconds);
  double phase = 0.0;
  double modulator = 0.0;
  double previous = 0.0;
  for (std::size_t sample = 0; sample < out.size(); ++sample)
  {
    const double time = TimeOf(sample);
    const double frequency = to + (from - to) * std::exp(-time * 10.0 / seconds);
    phase += TAU * frequency / RATE;
    modulator += TAU * frequency * 1.5 / RATE;
    const double tone = std::sin(phase + 1.4 * std::exp(-time * 12.0) * std::sin(modulator));
    const double noise = _noise.Next();
    const double click = (noise - previous) * std::exp(-time / 0.004); // high-passed by its difference
    previous = noise;
    out[sample] = Rise(time, 0.002) * std::exp(-time * 4.5 / seconds) * tone + 0.25 * click;
  }
  return out;
}

// sfx.unit.worker.die: a small creature's warbling squeal falling away, over a crackle.
std::vector<double> WorkerDie(Member _member, Noise& _noise)
{
  const double seconds = 0.45 + 0.1 * _member.index;
  const double from = 820.0 + 70.0 * (_member.index % 3);
  const double to = 95.0 + 15.0 * (_member.index % 4);
  const double warble = 22.0 + 3.0 * _member.index;
  std::vector<double> out = Silence(seconds);
  LowPass crackle(1'000.0);
  LowPass crackleAgain(1'000.0); // two poles, so that the crackle stays under the squeal
  double phase = 0.0;
  for (std::size_t sample = 0; sample < out.size(); ++sample)
  {
    const double time = TimeOf(sample);
    const double frequency = to + (from - to) * std::exp(-time * 3.2 / seconds);
    phase += TAU * frequency / RATE;
    const double tone = (std::sin(phase) + 0.33 * std::sin(3.0 * phase) + 0.2 * std::sin(5.0 * phase)) *
                        (1.0 - 0.175 * (1.0 + std::sin(TAU * warble * time)));
    const double burst = crackleAgain.Next(crackle.Next(_noise.Next())) * std::exp(-time / (0.18 * seconds));
    out[sample] = Rise(time, 0.004) * std::exp(-time * 3.2 / seconds) * (0.75 * tone + 3.0 * burst);
  }
  return out;
}

// sfx.unit.essence_orb.collected: a rising four-note arpeggio of glassy bells, with a shimmer.
std::vector<double> OrbCollected()
{
  constexpr std::array NOTES{1'046.5, 1'318.5, 1'568.0, 2'093.0};
  constexpr std::array PARTIALS{Partial{1.0, 1.0, 2.2}, Partial{2.76, 0.35, 3.5}, Partial{5.40, 0.15, 4.8}, Partial{8.93, 0.06, 6.1}};
  std::vector<double> out = Silence(2.0);
  for (std::size_t note = 0; note < NOTES.size(); ++note)
  {
    AddBell(out, 0.11 * static_cast<double>(note), NOTES[note], 1.0, PARTIALS);
  }
  for (std::size_t sample = 0; sample < out.size(); ++sample)
  {
    const double time = TimeOf(sample);
    out[sample] +=
      0.08 * std::sin(TAU * 4'186.0 * time) * (0.5 + 0.5 * std::sin(TAU * 6.0 * time)) * std::exp(-time * 1.5) * Rise(time, 0.05);
  }
  return out;
}

// sfx.building.dwelling.complete: a bright two-note chime a fifth apart, over a soft thump.
std::vector<double> StructureComplete()
{
  constexpr std::array PARTIALS{Partial{1.0, 1.0, 2.0}, Partial{2.0, 0.5, 3.0}, Partial{3.01, 0.25, 4.5}, Partial{4.2, 0.12, 6.0}};
  std::vector<double> out = Silence(2.0);
  AddBell(out, 0.0, 784.0, 1.0, PARTIALS);
  AddBell(out, 0.16, 1'174.7, 0.9, PARTIALS);
  double phase = 0.0;
  for (std::size_t sample = 0; sample < out.size(); ++sample)
  {
    const double time = TimeOf(sample);
    phase += TAU * (70.0 + 70.0 * std::exp(-time / 0.05)) / RATE;
    out[sample] += 0.8 * std::exp(-time / 0.07) * std::sin(phase);
  }
  return out;
}

// sfx.unit.worker.attack_horn: a war horn's call, a long note rising a fourth, with vibrato and breath.
std::vector<double> AttackHorn(Member _member, Noise& _noise)
{
  constexpr std::array LENGTHS{3.0, 3.7, 4.4};
  constexpr std::array ROOTS{98.0, 110.0, 87.3};
  const double seconds = LENGTHS.at(_member.index % LENGTHS.size());
  const double root = ROOTS.at(_member.index % ROOTS.size());
  const double change = 0.4 * seconds;
  std::vector<double> out = Silence(seconds);
  LowPass breath(900.0);
  double phase = 0.0;
  for (std::size_t sample = 0; sample < out.size(); ++sample)
  {
    const double time = TimeOf(sample);
    const double glide = std::clamp((time - change) / 0.12, 0.0, 1.0);
    const double vibrato = 0.006 * std::clamp((time - 0.6) / 0.4, 0.0, 1.0) * std::sin(TAU * 5.2 * time);
    phase += TAU * root * (1.0 + glide / 3.0) * (1.0 + vibrato) / RATE;
    const double dip = 1.0 - 0.15 * std::exp(-std::pow((time - change) / 0.05, 2.0));
    const double release = std::clamp((seconds - time) / 0.7, 0.0, 1.0);
    const double loudness = Rise(time, 0.07) * dip * release;
    out[sample] = loudness * (Brass(phase, loudness, 14, 1.1) + 0.03 * breath.Next(_noise.Next()));
  }
  return out;
}

// sfx.ui.battle.start: three drum hits, then a brass stab.
std::vector<double> BattleStart(Noise& _noise)
{
  std::vector<double> out = Silence(2.5);
  for (const double start : {0.0, 0.22, 0.44})
  {
    LowPass skin(600.0);
    double phase = 0.0;
    for (std::size_t sample = static_cast<std::size_t>(std::lround(start * RATE)); sample < out.size(); ++sample)
    {
      const double time = TimeOf(sample) - start;
      phase += TAU * (45.0 + 45.0 * std::exp(-time / 0.05)) / RATE;
      out[sample] += std::exp(-time / 0.18) * std::sin(phase) + 0.4 * std::exp(-time / 0.05) * skin.Next(_noise.Next());
    }
  }
  constexpr std::array CHORD{146.83, 220.0, 293.66};
  AddBrassChord(out, 0.62, 2.5, CHORD, 0.03, 1.2, 0.3, 1.1);
  return out;
}

// sfx.ui.battle.end: two brass chords resolving downwards, over a swelling cymbal wash.
std::vector<double> BattleEnd(Noise& _noise)
{
  std::vector<double> out = Silence(2.5);
  constexpr std::array FIRST{196.0, 246.94, 293.66};
  constexpr std::array SECOND{130.81, 164.81, 196.0, 261.63};
  AddBrassChord(out, 0.0, 1.05, FIRST, 0.04, 0.3, 0.25, 1.0);
  AddBrassChord(out, 0.9, 2.5, SECOND, 0.06, 0.9, 0.4, 1.1);
  LowPass body(3'000.0);
  for (std::size_t sample = 0; sample < out.size(); ++sample)
  {
    const double time = TimeOf(sample);
    const double noise = _noise.Next();
    const double swell = std::clamp(time / 0.9, 0.0, 1.0) * std::exp(-std::max(time - 0.9, 0.0) * 2.5);
    out[sample] += 0.03 * swell * (noise - body.Next(noise));
  }
  return out;
}

// Table 4.7-A's synthesised tone for a row with no recipe: a 440 Hz sine burst of the row's shortest target duration.
std::vector<double> SineBurst(const ManifestRow& _row)
{
  double seconds = 0.5;
  constexpr std::string_view ABOUT = "duration about ";
  if (const std::size_t at = _row.spec.find(ABOUT); at != std::string::npos)
  {
    const char* first = _row.spec.data() + at + ABOUT.size();
    double parsed = 0.0;
    if (std::from_chars(first, _row.spec.data() + _row.spec.size(), parsed).ec == std::errc{} && parsed > 0.0)
    {
      seconds = parsed;
    }
  }
  std::vector<double> out = Silence(seconds);
  for (std::size_t sample = 0; sample < out.size(); ++sample)
  {
    out[sample] = Rise(TimeOf(sample), 0.005) * std::sin(TAU * 440.0 * TimeOf(sample));
  }
  return out;
}

// --- Cooking ----------------------------------------------------------------------------------------------------------

bool Positioned(const ManifestRow& _row)
{
  return _row.spec.find("positional") != std::string::npos && _row.spec.find("not positional") == std::string::npos;
}

std::string KeyOf(const ManifestRow& _row)
{
  return Neuron::KeyOfSource(std::string_view(_row.speciesFilename).substr(CONTENT_PREFIX.size()));
}

// A member's checks of AST-079 and AST-080 on its samples, which an authored source and a stand-in meet alike.
bool CheckMember(const ManifestRow& _row, std::string_view _file, std::size_t _frames, std::uint16_t _channels, BuildReport& _report)
{
  bool good = true;
  if (_row.targetFormat.find("resident") == std::string::npos)
  {
    _report.Fail(_file, std::format("its row {} is streamed, which arrives after the MVP (AST-080, ADR-171)", _row.assetId));
    good = false;
  }
  if (_frames == 0)
  {
    _report.Fail(_file, "it has no sample frames (AST-079, AEN-179)");
    good = false;
  }
  if (static_cast<double>(_frames) / RATE > STREAM_THRESHOLD_SECONDS)
  {
    _report.Fail(_file, std::format("it lasts {:.2f} s, longer than the stream threshold of {} s (aen.residency.stream_threshold, AST-080)",
                                    static_cast<double>(_frames) / RATE, STREAM_THRESHOLD_SECONDS));
    good = false;
  }
  if (_channels == 0 || _channels > Neuron::SOUND_CHANNELS_MAX || (_channels != 1 && Positioned(_row)))
  {
    _report.Fail(_file, std::format("it has {} channels; its row {} plays {} (AST-079, AEN-072)", _channels, _row.assetId,
                                    Positioned(_row) ? "positioned, so mono" : "two-dimensional, so mono or stereo"));
    good = false;
  }
  return good;
}

Neuron::CookedEntry EntryOf(const ManifestRow& _row, std::span<const float> _samples, std::uint16_t _channels, bool _placeholder)
{
  Neuron::SoundAsset sound;
  sound.channels = _channels;
  sound.samples.reserve(_samples.size());
  for (const float sample : _samples)
  {
    // AST-080: the nearest 16-bit value, with no dither, so the output is reproducible.
    sound.samples.push_back(
      static_cast<std::int16_t>(std::clamp(std::lround(static_cast<double>(sample) * INT16_SCALE), -32'767L, 32'767L)));
  }
  Neuron::CookedEntry entry;
  entry.entry.key = KeyOf(_row);
  entry.entry.type = Neuron::ContentType::ResidentSound;
  entry.entry.placeholder = _placeholder;
  entry.bytes = Neuron::WriteSoundAsset(sound);
  return entry;
}

} // namespace

bool IsSoundCategory(std::string_view _category)
{
  return std::ranges::find(SOUND_CATEGORIES, _category) != SOUND_CATEGORIES.end();
}

std::optional<Neuron::CookedEntry> CookSound(const ManifestRow& _row, std::string_view _file, std::span<const std::byte> _bytes,
                                             BuildReport& _report)
{
  const std::expected<WavFile, std::string> wav = ReadWav(_bytes);
  if (!wav)
  {
    _report.Fail(_file, std::format("{} (AST-079)", wav.error()));
    return std::nullopt;
  }
  bool good = true;
  if (wav->sampleRate != Neuron::SOUND_SAMPLE_RATE)
  {
    _report.Fail(_file, std::format("it is at {} Hz; sources are at {} Hz, and the build resamples nothing (AST-079)", wav->sampleRate,
                                    Neuron::SOUND_SAMPLE_RATE));
    good = false;
  }
  if (wav->encoding == WavEncoding::IntegerPcm && wav->bitsPerSample != AUTHORED_INTEGER_BITS)
  {
    _report.Fail(_file,
                 std::format("it is {}-bit integer PCM; sources are 24-bit integer or 32-bit float PCM (AST-079)", wav->bitsPerSample));
    good = false;
  }
  good = CheckMember(_row, _file, wav->Frames(), wav->channels, _report) && good;
  if (!good)
  {
    return std::nullopt;
  }
  return EntryOf(_row, wav->samples, wav->channels, false);
}

namespace
{

// The samples of a stand-in from -1 to 1, mono at 48 kHz; empty for a row of another kind.
std::vector<float> SynthesizeSound(const ManifestRow& _row)
{
  if (_row.priority != STAND_IN_PRIORITY || !IsSoundCategory(_row.category) || _row.placeholder != SYNTHESIZED ||
      !_row.speciesFilename.starts_with(CONTENT_PREFIX) || !_row.speciesFilename.ends_with(".wav"))
  {
    return {};
  }
  const Member member = MemberOf(_row.variants);
  Noise noise(SeedOf(_row.assetId, member.index));
  using Recipe = std::function<std::vector<double>()>;
  const std::map<std::string_view, Recipe> recipes{
    {"sfx.weapon.laser_bolt.spawn", [&] { return LaserBolt(member, noise); }},
    {"sfx.unit.worker.die", [&] { return WorkerDie(member, noise); }},
    {"sfx.unit.essence_orb.collected", [] { return OrbCollected(); }},
    {"sfx.building.dwelling.complete", [] { return StructureComplete(); }},
    {"sfx.unit.worker.attack_horn", [&] { return AttackHorn(member, noise); }},
    {"sfx.ui.battle.start", [&] { return BattleStart(noise); }},
    {"sfx.ui.battle.end", [&] { return BattleEnd(noise); }},
  };
  const auto recipe = recipes.find(_row.assetId);
  std::vector<double> samples = recipe != recipes.end() ? recipe->second() : SineBurst(_row);
  // Each at the same peak, its mix level the event's (ADR-171), and faded at its end.
  double peak = 0.0;
  for (const double sample : samples)
  {
    peak = std::max(peak, std::abs(sample));
  }
  const double gain = peak > 0.0 ? PEAK / peak : 0.0;
  const auto fade = static_cast<std::size_t>(std::lround(END_FADE_SECONDS * RATE));
  std::vector<float> out(samples.size());
  for (std::size_t sample = 0; sample < samples.size(); ++sample)
  {
    const std::size_t left = samples.size() - sample;
    const double end = left < fade ? static_cast<double>(left) / static_cast<double>(fade) : 1.0;
    out[sample] = static_cast<float>(samples[sample] * gain * end);
  }
  return out;
}

} // namespace

std::optional<Neuron::CookedEntry> MakePlaceholderSound(const ManifestRow& _row, BuildReport& _report)
{
  const std::vector<float> samples = SynthesizeSound(_row);
  if (samples.empty())
  {
    return std::nullopt;
  }
  // AST-083: a stand-in meets its row's checks as an authored file must.
  if (!CheckMember(_row, _row.speciesFilename, samples.size(), 1, _report))
  {
    return std::nullopt;
  }
  return EntryOf(_row, samples, 1, true);
}

void CheckSoundBank(std::span<const Neuron::CookedEntry> _entries, BuildReport& _report)
{
  std::vector<const Neuron::CookedEntry*> members;
  std::size_t total = 0;
  for (const Neuron::CookedEntry& entry : _entries)
  {
    if (entry.entry.type == Neuron::ContentType::ResidentSound)
    {
      members.push_back(&entry);
      total += entry.bytes.size();
    }
  }
  if (total <= INTERFACE_BANK_BUDGET_BYTES)
  {
    return;
  }
  std::ranges::sort(members, std::ranges::greater{}, [](const Neuron::CookedEntry* _entry) { return _entry->bytes.size(); });
  std::string largest;
  for (std::size_t index = 0; index < std::min<std::size_t>(members.size(), 3); ++index)
  {
    largest += std::format("{}{} ({} bytes)", index == 0 ? "" : ", ", members[index]->entry.key, members[index]->bytes.size());
  }
  _report.Fail("interface bank",
               std::format("its resident sounds hold {} bytes, over aen.memory.interface_bank_budget's {}; the largest: {} "
                           "(AST-081, ADR-171)",
                           total, INTERFACE_BANK_BUDGET_BYTES, largest));
}

} // namespace ContentBuild
