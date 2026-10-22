#include "pch.h"

#include "ClientSettings.h"

#include <algorithm>
#include <string>
#include <utility>
#include <variant>

namespace Species
{

namespace
{

// RND-442's defaults, named after their keys (ADR-116 keeps `rnd.option.*.default` as constants, ADR-150).
constexpr std::string_view WINDOW_MODE_DEFAULT = WINDOW_MODE_BORDERLESS;      // rnd.option.window_mode.default
constexpr std::string_view PRESENT_MODE_DEFAULT = PRESENT_MODE_VERTICAL_SYNC; // rnd.option.present_mode.default
constexpr std::int64_t FRAME_RATE_CAP_DEFAULT = 0;                            // rnd.option.frame_rate_cap.default: none
constexpr bool LOW_LATENCY_DEFAULT = false;                                   // rnd.option.low_latency.default
constexpr std::string_view ANTI_ALIASING_DEFAULT = ANTI_ALIASING_4X;          // rnd.option.anti_aliasing.default
constexpr double BRIGHTNESS_DEFAULT = 1.0;                                    // rnd.option.brightness.default
constexpr std::int64_t SEA_DETAIL_DEFAULT = 1;                                // rnd.option.sea_detail.default
constexpr std::int64_t ENTITY_DETAIL_DEFAULT = 1;                             // rnd.option.entity_detail.default
constexpr double WORKER_SCALE_FACTOR_DEFAULT = 2.0;                           // rnd.option.worker_scale_factor.default
constexpr std::int64_t VOLUME_DEFAULT = 255;                                  // aen.settings.volume_default

// The ranges that no requirement states yet, chosen by ADR-150: a frame-rate cap of at most 1,000 frames per second (0
// is none), a render size of at most 16,384 pixels a side (D3D12's largest texture), a brightness from 0.5 to 2, a
// Worker scale factor of at most 10, and UI and text scales from 0.5 to 3.
constexpr double FRAME_RATE_CAP_MAX = 1000.0;
constexpr double RENDER_SIDE_MAX = 16384.0;
constexpr double BRIGHTNESS_MIN = 0.5;
constexpr double BRIGHTNESS_MAX = 2.0;
constexpr double WORKER_SCALE_FACTOR_MAX = 10.0;
constexpr double UI_SCALE_MIN = 0.5;
constexpr double UI_SCALE_MAX = 3.0;

Neuron::SettingSpec Spec(std::string_view _key, Neuron::SettingType _type, std::optional<Neuron::SettingValue> _default,
                         double _minimum = 0.0, double _maximum = 0.0, std::vector<std::string> _enumerators = {})
{
  return Neuron::SettingSpec{std::string(_key), _type, std::move(_default), _minimum, _maximum, std::move(_enumerators)};
}

std::optional<double> OptionalDecimal(const Neuron::SettingsStore& _store, std::string_view _key)
{
  const std::optional<Neuron::SettingValue> value = _store.Get(_key);
  const auto* decimal = value.has_value() ? std::get_if<double>(&*value) : nullptr;
  return decimal != nullptr ? std::optional<double>(*decimal) : std::nullopt;
}

} // namespace

std::vector<Neuron::SettingSpec> ClientSettingSpecs()
{
  using Neuron::SettingType;
  std::vector<Neuron::SettingSpec> specs;
  specs.push_back(Spec(SETTING_WINDOW_MODE, SettingType::Enumeration, std::string(WINDOW_MODE_DEFAULT), 0.0, 0.0,
                       {std::string(WINDOW_MODE_WINDOWED), std::string(WINDOW_MODE_BORDERLESS)}));
  specs.push_back(Spec(SETTING_RENDER_WIDTH, SettingType::Integer, std::nullopt, 1.0, RENDER_SIDE_MAX));
  specs.push_back(Spec(SETTING_RENDER_HEIGHT, SettingType::Integer, std::nullopt, 1.0, RENDER_SIDE_MAX));
  specs.push_back(Spec(SETTING_PRESENT_MODE, SettingType::Enumeration, std::string(PRESENT_MODE_DEFAULT), 0.0, 0.0,
                       {std::string(PRESENT_MODE_VERTICAL_SYNC), std::string(PRESENT_MODE_UNCAPPED)}));
  specs.push_back(Spec(SETTING_FRAME_RATE_CAP, SettingType::Integer, FRAME_RATE_CAP_DEFAULT, 0.0, FRAME_RATE_CAP_MAX));
  specs.push_back(Spec(SETTING_LOW_LATENCY, SettingType::Boolean, LOW_LATENCY_DEFAULT));
  specs.push_back(Spec(SETTING_ANTI_ALIASING, SettingType::Enumeration, std::string(ANTI_ALIASING_DEFAULT), 0.0, 0.0,
                       {std::string(ANTI_ALIASING_OFF), std::string(ANTI_ALIASING_2X), std::string(ANTI_ALIASING_4X)}));
  specs.push_back(Spec(SETTING_BRIGHTNESS, SettingType::Decimal, BRIGHTNESS_DEFAULT, BRIGHTNESS_MIN, BRIGHTNESS_MAX));
  specs.push_back(Spec(SETTING_SEA_DETAIL, SettingType::Integer, SEA_DETAIL_DEFAULT, 0.0, 3.0));
  specs.push_back(Spec(SETTING_ENTITY_DETAIL, SettingType::Integer, ENTITY_DETAIL_DEFAULT, 1.0, 3.0));
  specs.push_back(Spec(SETTING_WORKER_SCALE_FACTOR, SettingType::Decimal, WORKER_SCALE_FACTOR_DEFAULT, 0.0, WORKER_SCALE_FACTOR_MAX));
  specs.push_back(Spec(SETTING_UI_SCALE, SettingType::Decimal, std::nullopt, UI_SCALE_MIN, UI_SCALE_MAX));
  specs.push_back(Spec(SETTING_TEXT_SCALE, SettingType::Decimal, std::nullopt, UI_SCALE_MIN, UI_SCALE_MAX));
  specs.push_back(Spec(SETTING_REDUCED_FLASH, SettingType::Boolean, false));
  specs.push_back(Spec(SETTING_REDUCED_MOTION, SettingType::Boolean, false));
  specs.push_back(Spec(SETTING_MASTER_VOLUME, SettingType::Integer, VOLUME_DEFAULT, 0.0, static_cast<double>(MASTER_VOLUME_MAX)));
  specs.push_back(Spec(SETTING_KEYBOARD_LAYOUT, SettingType::Text, std::nullopt));
  specs.push_back(Spec(SETTING_BINDING_FAMILY, SettingType::Binding, std::nullopt));
  return specs;
}

DisplayOptions DisplayOptionsOf(const Neuron::SettingsStore& _store)
{
  DisplayOptions options;
  options.seaDetail = _store.GetInteger(SETTING_SEA_DETAIL);
  options.entityDetail = _store.GetInteger(SETTING_ENTITY_DETAIL);
  options.workerScaleFactor = _store.GetDecimal(SETTING_WORKER_SCALE_FACTOR);
  options.brightness = _store.GetDecimal(SETTING_BRIGHTNESS);
  options.reducedFlash = _store.GetBool(SETTING_REDUCED_FLASH);
  options.reducedMotion = _store.GetBool(SETTING_REDUCED_MOTION);
  options.uiScale = OptionalDecimal(_store, SETTING_UI_SCALE);
  options.textScale = OptionalDecimal(_store, SETTING_TEXT_SCALE);
  return options;
}

RenderSettings RenderSettingsOf(const Neuron::SettingsStore& _store)
{
  RenderSettings settings;
  settings.windowed = _store.GetText(SETTING_WINDOW_MODE) == WINDOW_MODE_WINDOWED;
  const std::optional<Neuron::SettingValue> width = _store.Get(SETTING_RENDER_WIDTH);
  const std::optional<Neuron::SettingValue> height = _store.Get(SETTING_RENDER_HEIGHT);
  const auto* widthPixels = width.has_value() ? std::get_if<std::int64_t>(&*width) : nullptr;
  const auto* heightPixels = height.has_value() ? std::get_if<std::int64_t>(&*height) : nullptr;
  if (widthPixels != nullptr && heightPixels != nullptr)
  {
    settings.renderSize = {static_cast<std::uint32_t>(*widthPixels), static_cast<std::uint32_t>(*heightPixels)};
  }
  settings.uncapped = _store.GetText(SETTING_PRESENT_MODE) == PRESENT_MODE_UNCAPPED;
  const std::int64_t cap = _store.GetInteger(SETTING_FRAME_RATE_CAP);
  settings.frameRateCap = cap > 0 ? std::optional<float>(static_cast<float>(cap)) : std::nullopt;
  settings.lowLatency = _store.GetBool(SETTING_LOW_LATENCY);
  const std::string antiAliasing = _store.GetText(SETTING_ANTI_ALIASING);
  settings.sceneSamplesPreferred = antiAliasing == ANTI_ALIASING_OFF ? 1u : antiAliasing == ANTI_ALIASING_2X ? 2u : 4u;
  return settings;
}

void WriteBackDisplay(Neuron::SettingsStore& _store, bool _windowed, Neuron::SizePx _renderSize)
{
  _store.Set(SETTING_WINDOW_MODE, std::string(_windowed ? WINDOW_MODE_WINDOWED : WINDOW_MODE_BORDERLESS));
  if (_renderSize.width > 0 && _renderSize.height > 0)
  {
    _store.Set(SETTING_RENDER_WIDTH, static_cast<std::int64_t>(_renderSize.width));
    _store.Set(SETTING_RENDER_HEIGHT, static_cast<std::int64_t>(_renderSize.height));
  }
}

} // namespace Species
