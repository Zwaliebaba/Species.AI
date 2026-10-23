#include "pch.h"

#include <xaudio2.h>
#include <xapofx.h>
#include <wrl/client.h>

#include "Log.h"
#include "XAudio2Device.h"

#include <array>
#include <atomic>
#include <cmath>
#include <vector>

#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "ole32.lib")

namespace Neuron
{

namespace
{

constexpr std::uint32_t SAMPLE_BITS = 16;
constexpr UINT32 SUBMIX_STAGE_BUSES = 0; // the player buses send to the pre-master, so they are processed first
constexpr UINT32 SUBMIX_STAGE_PREMASTER = 1;

// AEN-025, ADR-026: a callback only sets a flag; it never allocates, locks, logs or calls the engine.
class VoiceCallback final : public IXAudio2VoiceCallback
{
public:
  void STDMETHODCALLTYPE OnStreamEnd() noexcept override
  {
    m_ended.store(true, std::memory_order_release);
  }
  void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() noexcept override {}
  void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32 /*_bytesRequired*/) noexcept override {}
  void STDMETHODCALLTYPE OnBufferEnd(void* /*_context*/) noexcept override {}
  void STDMETHODCALLTYPE OnBufferStart(void* /*_context*/) noexcept override {}
  void STDMETHODCALLTYPE OnLoopEnd(void* /*_context*/) noexcept override {}
  void STDMETHODCALLTYPE OnVoiceError(void* /*_context*/, HRESULT /*_error*/) noexcept override {}

  void Reset() noexcept
  {
    m_ended.store(false, std::memory_order_relaxed);
  }
  [[nodiscard]] bool Ended() const noexcept
  {
    return m_ended.load(std::memory_order_acquire);
  }

private:
  std::atomic<bool> m_ended{false};
};

class EngineCallback final : public IXAudio2EngineCallback
{
public:
  void STDMETHODCALLTYPE OnProcessingPassEnd() noexcept override {}
  void STDMETHODCALLTYPE OnProcessingPassStart() noexcept override {}
  void STDMETHODCALLTYPE OnCriticalError(HRESULT /*_error*/) noexcept override
  {
    m_lost.store(true, std::memory_order_release);
  }

  void Reset() noexcept
  {
    m_lost.store(false, std::memory_order_relaxed);
  }
  [[nodiscard]] bool Lost() const noexcept
  {
    return m_lost.load(std::memory_order_acquire);
  }

private:
  std::atomic<bool> m_lost{false};
};

// A voice is no COM object: it is destroyed, never released (R12's RAII through its own deleter).
struct VoiceDestroyer
{
  void operator()(IXAudio2Voice* _voice) const noexcept
  {
    _voice->DestroyVoice();
  }
};

template <class T> using VoicePointer = std::unique_ptr<T, VoiceDestroyer>;

// XAudio2 needs COM on the thread that creates it; each thread that builds a device initializes it once.
struct ComScope
{
  ComScope() noexcept
    : initialized(SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED)))
  {
  }
  ~ComScope()
  {
    if (initialized)
    {
      CoUninitialize();
    }
  }
  ComScope(const ComScope&) = delete;
  ComScope& operator=(const ComScope&) = delete;

  bool initialized;
};

struct PooledVoice
{
  VoiceCallback callback; // declared first, so that the voice that calls it is destroyed before it
  VoicePointer<IXAudio2SourceVoice> source;
  std::uint16_t channels = 1;
  AudioBus bus = AudioBus::World;
  bool busy = false;
};

class XAudio2Device final : public AudioDevice
{
public:
  explicit XAudio2Device(const XAudio2DeviceDesc& _desc)
    : m_desc(_desc)
  {
  }
  ~XAudio2Device() override
  {
    Teardown();
  }

  XAudio2Device(const XAudio2Device&) = delete;
  XAudio2Device& operator=(const XAudio2Device&) = delete;

  // The engine, its mastering voice on the default device, the pre-master with its limiter, the six buses and the
  // pools; false, logged, with nothing left built, when any of it cannot be made.
  bool Build()
  {
    thread_local ComScope g_com;
    m_engineCallback.Reset();
    if (FAILED(XAudio2Create(&m_xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR)) || FAILED(m_xaudio->RegisterForCallbacks(&m_engineCallback)))
    {
      return Fail("XAudio2 could not be created");
    }
    IXAudio2MasteringVoice* master = nullptr;
    if (FAILED(m_xaudio->CreateMasteringVoice(&master)))
    {
      return Fail("there is no audio output device");
    }
    m_master.reset(master);
    XAUDIO2_VOICE_DETAILS details{};
    m_master->GetVoiceDetails(&details);
    m_channels = details.InputChannels;
    // AEN-220: the pre-master carries the master volume and the limiter, which keeps the output from clipping.
    Microsoft::WRL::ComPtr<IUnknown> limiter;
    FXMASTERINGLIMITER_PARAMETERS limits{m_desc.limiterRelease, m_desc.limiterLoudness};
    if (FAILED(CreateFX(__uuidof(FXMasteringLimiter), &limiter, &limits, sizeof(limits))))
    {
      return Fail("the mastering limiter could not be created");
    }
    XAUDIO2_EFFECT_DESCRIPTOR effect{limiter.Get(), TRUE, m_channels};
    XAUDIO2_EFFECT_CHAIN chain{1, &effect};
    IXAudio2SubmixVoice* premaster = nullptr;
    if (FAILED(m_xaudio->CreateSubmixVoice(&premaster, m_channels, details.InputSampleRate, 0, SUBMIX_STAGE_PREMASTER, nullptr, &chain)))
    {
      return Fail("the pre-master bus could not be created");
    }
    m_premaster.reset(premaster);
    XAUDIO2_SEND_DESCRIPTOR toPremaster{0, m_premaster.get()};
    XAUDIO2_VOICE_SENDS premasterSends{1, &toPremaster};
    for (VoicePointer<IXAudio2SubmixVoice>& bus : m_buses)
    {
      IXAudio2SubmixVoice* submix = nullptr;
      if (FAILED(
            m_xaudio->CreateSubmixVoice(&submix, m_channels, details.InputSampleRate, 0, SUBMIX_STAGE_BUSES, &premasterSends, nullptr)))
      {
        return Fail("a player bus could not be created");
      }
      bus.reset(submix);
    }
    // AEN-142: every source voice is made now, one pool of mono and one of stereo 16-bit PCM at 48 kHz.
    m_voices.clear();
    m_voices.reserve(static_cast<std::size_t>(m_desc.voicesPerPool) * 2);
    for (const std::uint16_t channels : {std::uint16_t{1}, std::uint16_t{2}})
    {
      WAVEFORMATEX format{};
      format.wFormatTag = WAVE_FORMAT_PCM;
      format.nChannels = channels;
      format.nSamplesPerSec = SOUND_SAMPLE_RATE;
      format.wBitsPerSample = SAMPLE_BITS;
      format.nBlockAlign = static_cast<WORD>(channels * SAMPLE_BITS / 8);
      format.nAvgBytesPerSec = SOUND_SAMPLE_RATE * format.nBlockAlign;
      for (std::uint32_t index = 0; index < m_desc.voicesPerPool; ++index)
      {
        auto voice = std::make_unique<PooledVoice>();
        voice->channels = channels;
        XAUDIO2_SEND_DESCRIPTOR toWorld{0, m_buses[static_cast<std::size_t>(AudioBus::World)].get()};
        XAUDIO2_VOICE_SENDS sends{1, &toWorld};
        IXAudio2SourceVoice* source = nullptr;
        if (FAILED(m_xaudio->CreateSourceVoice(&source, &format, 0, m_desc.maxFrequencyRatio, &voice->callback, &sends, nullptr)))
        {
          return Fail("a source voice could not be created");
        }
        voice->source.reset(source);
        m_voices.push_back(std::move(voice));
      }
    }
    Log(LogCategory::Audio, LogLevel::Info, "audio output: {} channels at {} Hz, {} voices", m_channels, details.InputSampleRate,
        m_voices.size());
    return true;
  }

  [[nodiscard]] std::uint32_t OutputChannels() const noexcept override
  {
    return m_channels;
  }

  [[nodiscard]] std::optional<AudioVoice> Start(const SoundAsset& _sound, std::size_t _startFrame, AudioBus _bus, float _amplitude,
                                                const std::array<float, 2>& _pan) override
  {
    for (std::size_t index = 0; index < m_voices.size(); ++index)
    {
      PooledVoice& voice = *m_voices[index];
      if (voice.busy || voice.channels != _sound.channels)
      {
        continue;
      }
      // A voice stopped and flushed this update may still hold its buffer until XAudio2's next pass; it waits.
      XAUDIO2_VOICE_STATE state{};
      voice.source->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
      if (state.BuffersQueued != 0)
      {
        continue;
      }
      XAUDIO2_SEND_DESCRIPTOR toBus{0, m_buses[static_cast<std::size_t>(_bus)].get()};
      XAUDIO2_VOICE_SENDS sends{1, &toBus};
      XAUDIO2_BUFFER buffer{};
      buffer.Flags = XAUDIO2_END_OF_STREAM;
      buffer.AudioBytes = static_cast<UINT32>(_sound.samples.size() * sizeof(std::int16_t));
      buffer.pAudioData = reinterpret_cast<const BYTE*>(_sound.samples.data());
      buffer.PlayBegin = static_cast<UINT32>(_startFrame);
      voice.callback.Reset();
      if (FAILED(voice.source->SetOutputVoices(&sends)) || FAILED(voice.source->SubmitSourceBuffer(&buffer)))
      {
        return std::nullopt;
      }
      voice.bus = _bus;
      voice.busy = true;
      const auto handle = static_cast<AudioVoice>(index);
      Set(handle, _amplitude, _pan);
      voice.source->Start(0, m_operationSet);
      return handle;
    }
    return std::nullopt;
  }

  void Set(AudioVoice _voice, float _amplitude, const std::array<float, 2>& _pan) override
  {
    PooledVoice& voice = *m_voices[_voice];
    voice.source->SetVolume(_amplitude, m_operationSet);
    // AEN-204: a mono voice panned onto the front left and right, a stereo one left to left and right to right; a
    // mono output takes the pan's power.
    std::array<float, std::size_t{2} * XAUDIO2_MAX_AUDIO_CHANNELS> matrix{};
    const std::uint32_t outputs = m_channels;
    if (outputs == 1)
    {
      for (std::uint32_t input = 0; input < voice.channels; ++input)
      {
        matrix[input] = std::sqrt(_pan[0] * _pan[0] + _pan[1] * _pan[1]);
      }
    }
    else if (voice.channels == 1)
    {
      matrix[0] = _pan[0];
      matrix[1] = _pan[1];
    }
    else
    {
      matrix[0] = _pan[0];                  // left to left
      matrix[voice.channels + 1] = _pan[1]; // right to right
    }
    voice.source->SetOutputMatrix(m_buses[static_cast<std::size_t>(voice.bus)].get(), voice.channels, outputs, matrix.data(),
                                  m_operationSet);
  }

  void Stop(AudioVoice _voice) override
  {
    PooledVoice& voice = *m_voices[_voice];
    voice.source->Stop(0, XAUDIO2_COMMIT_NOW);
    voice.source->FlushSourceBuffers();
    voice.busy = false;
  }

  [[nodiscard]] bool Ended(AudioVoice _voice) noexcept override
  {
    return m_voices[_voice]->callback.Ended();
  }

  void SetBusGain(AudioBus _bus, float _amplitude) override
  {
    m_buses[static_cast<std::size_t>(_bus)]->SetVolume(_amplitude, m_operationSet);
  }

  void SetPreMasterGain(float _amplitude) override
  {
    m_premaster->SetVolume(_amplitude, m_operationSet);
  }

  void Commit() override
  {
    // ADR-026: every change of the update takes effect at the same sample.
    m_xaudio->CommitChanges(m_operationSet);
    m_operationSet = m_operationSet == UINT32_MAX ? 1 : m_operationSet + 1;
  }

  [[nodiscard]] bool Lost() const noexcept override
  {
    return m_xaudio == nullptr || m_engineCallback.Lost();
  }

  [[nodiscard]] bool Rebuild() override
  {
    Teardown();
    return Build();
  }

private:
  bool Fail(const char* _what)
  {
    Log(LogCategory::Audio, LogLevel::Warning, "{}; sound plays through the silent device (AEN-100)", _what);
    Teardown();
    return false;
  }

  void Teardown()
  {
    // The source voices first, then the buses, the pre-master and the mastering voice, then the engine (AEN-106).
    m_voices.clear();
    for (VoicePointer<IXAudio2SubmixVoice>& bus : m_buses)
    {
      bus.reset();
    }
    m_premaster.reset();
    m_master.reset();
    if (m_xaudio != nullptr)
    {
      m_xaudio->StopEngine();
      m_xaudio->UnregisterForCallbacks(&m_engineCallback);
      m_xaudio.Reset();
    }
    m_channels = 0;
  }

  XAudio2DeviceDesc m_desc;
  Microsoft::WRL::ComPtr<IXAudio2> m_xaudio;
  EngineCallback m_engineCallback;
  VoicePointer<IXAudio2MasteringVoice> m_master;
  VoicePointer<IXAudio2SubmixVoice> m_premaster;
  std::array<VoicePointer<IXAudio2SubmixVoice>, AUDIO_BUS_COUNT> m_buses;
  std::vector<std::unique_ptr<PooledVoice>> m_voices; // each where it was made, as its callback's address is XAudio2's
  std::uint32_t m_channels = 0;
  UINT32 m_operationSet = 1;
};

} // namespace

std::unique_ptr<AudioDevice> CreateXAudio2Device(const XAudio2DeviceDesc& _desc)
{
  auto device = std::make_unique<XAudio2Device>(_desc);
  if (!device->Build())
  {
    return nullptr;
  }
  return device;
}

} // namespace Neuron
