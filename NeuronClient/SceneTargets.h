#pragma once

#include "DescriptorHeaps.h"
#include "GraphicsDevice.h"
#include "WindowGeometry.h"

#include <array>
#include <cstdint>
#include <optional>

namespace Neuron
{

inline constexpr DXGI_FORMAT SCENE_COLOR_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM; // SDR (ADR-013, RND-063)
inline constexpr DXGI_FORMAT SCENE_DEPTH_FORMAT = DXGI_FORMAT_D32_FLOAT;      // RND-055, ADR-016
inline constexpr float SCENE_DEPTH_CLEAR = 0.0f;                              // reversed Z: the far plane

// ADR-015: the first sample count of _preferred, 2 and 1 that the device supports for both scene formats.
[[nodiscard]] std::uint32_t SupportedSceneSamples(ID3D12Device* _device, std::uint32_t _preferred);

/// The frame graph's scene targets (RND-063): scene_colour and scene_depth at the render size and the sample count, and
/// scene_resolved, the single-sample image the final pass reads. With one sample, scene_colour is read directly and
/// scene_resolved does not exist. They are committed resources (no aliasing at M1, owner decision O75), created again on
/// a resize (RND-030), each with its optimized clear value.
class SceneTargets
{
public:
  // _readSlot is the persistent slot of the shader-visible heap that holds the view the final pass reads; it is kept
  // across every recreation. Empty when a target cannot be created.
  [[nodiscard]] static std::optional<SceneTargets> Create(ID3D12Device* _device, SizePx _size, std::uint32_t _samples,
                                                          const std::array<float, 4>& _clearColor, ShaderVisibleHeaps& _heaps,
                                                          std::uint32_t _readSlot);

  [[nodiscard]] ID3D12Resource* Color() const noexcept
  {
    return m_color.Get();
  }
  [[nodiscard]] ID3D12Resource* Depth() const noexcept
  {
    return m_depth.Get();
  }
  // The resolve destination, or null with one sample.
  [[nodiscard]] ID3D12Resource* Resolved() const noexcept
  {
    return m_resolved.Get();
  }
  [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE ColorView() const noexcept
  {
    return m_renderTargets->Handle(0);
  }
  [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE DepthView() const noexcept
  {
    return m_depthTargets->Handle(0);
  }
  [[nodiscard]] std::uint32_t ReadSlot() const noexcept
  {
    return m_readSlot;
  }
  [[nodiscard]] std::uint32_t Samples() const noexcept
  {
    return m_samples;
  }
  [[nodiscard]] SizePx Size() const noexcept
  {
    return m_size;
  }
  [[nodiscard]] const std::array<float, 4>& ClearColor() const noexcept
  {
    return m_clearColor;
  }

private:
  SceneTargets() = default;

  ComPtr<ID3D12Resource> m_color;
  ComPtr<ID3D12Resource> m_depth;
  ComPtr<ID3D12Resource> m_resolved;
  std::optional<CpuDescriptorHeap> m_renderTargets;
  std::optional<CpuDescriptorHeap> m_depthTargets;
  std::uint32_t m_readSlot = 0;
  std::uint32_t m_samples = 1;
  SizePx m_size;
  std::array<float, 4> m_clearColor{};
};

} // namespace Neuron
