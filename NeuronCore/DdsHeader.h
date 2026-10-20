#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace Neuron
{

// The texture formats of ADR-017's table, by the DXGI_FORMAT values that a DDS file's DX10 header stores. NeuronCore
// names them itself, because the headless server includes no DXGI header (ADR-090 item 3).
enum class TextureFormat : std::uint32_t
{
  Unknown = 0,
  R8G8B8A8Unorm = 28,
  R8G8B8A8UnormSrgb = 29,
  R8G8Unorm = 49,
  R8Unorm = 61,
  Bc4Unorm = 80,
  Bc5Unorm = 83,
  Bc7UnormSrgb = 99
};

// The DXGI spelling without its prefix, which the asset manifest uses: "BC4_UNORM", "R8G8B8A8_UNORM_SRGB".
[[nodiscard]] std::string_view TextureFormatName(TextureFormat _format) noexcept;
// The format of that spelling, or none for a spelling outside ADR-017's table.
[[nodiscard]] std::optional<TextureFormat> TextureFormatNamed(std::string_view _name) noexcept;
[[nodiscard]] bool IsBlockCompressed(TextureFormat _format) noexcept;
// AST-020: the sRGB-tagged formats; every other format of the table is linear.
[[nodiscard]] bool IsSrgb(TextureFormat _format) noexcept;
// The bytes of one level of _width × _height texels: whole 4 × 4 blocks for a block-compressed format.
[[nodiscard]] std::uint64_t LevelBytes(TextureFormat _format, std::uint32_t _width, std::uint32_t _height) noexcept;
// The level count of a full chain down to 1 × 1 (AST-070).
[[nodiscard]] std::uint32_t FullMipLevels(std::uint32_t _width, std::uint32_t _height) noexcept;

inline constexpr std::size_t DDS_HEADER_BYTES = 128;     // the magic "DDS " and DDS_HEADER
inline constexpr std::size_t DDS_DX10_HEADER_BYTES = 20; // DDS_HEADER_DXT10
inline constexpr std::uint32_t DDS_TEXTURE_2D = 3;       // D3D10_RESOURCE_DIMENSION_TEXTURE2D
inline constexpr std::uint32_t DDS_PIXEL_RGB = 0x40;     // DDPF_RGB
inline constexpr std::uint32_t DDS_PIXEL_ALPHA = 0x1;    // DDPF_ALPHAPIXELS

/// What a DDS file's headers state (the DDS programming guide of the Windows SDK documentation): its size and levels,
/// and either the DX10 header's format or, for a file without one, the legacy pixel format.
struct DdsHeader
{
  std::uint32_t width = 0;
  std::uint32_t height = 0;
  std::uint32_t mipLevels = 1; // 1 when the header states none
  bool dx10 = false;
  std::uint32_t dxgiFormat = 0; // the DX10 header's DXGI_FORMAT value, which may name no format of the table
  std::uint32_t resourceDimension = 0;
  std::uint32_t miscFlag = 0;
  std::uint32_t arraySize = 0;
  std::uint32_t pixelFlags = 0; // the legacy pixel format, DDS_PIXELFORMAT
  std::uint32_t rgbBitCount = 0;
  std::array<std::uint32_t, 4> masks{}; // red, green, blue and alpha
  std::size_t payloadOffset = 0;        // the first byte after the headers
  std::uint64_t payloadBytes = 0;       // the bytes after the headers

  [[nodiscard]] TextureFormat Format() const noexcept;
};

// The headers of a DDS file, or why the bytes are not one: a wrong magic or header size, or a file shorter than its
// headers. It checks no format, size or level against anything; the content build and the loader do.
[[nodiscard]] std::expected<DdsHeader, std::string> ReadDdsHeader(std::span<const std::byte> _file);

// The bytes that the levels of a 2-D texture of one array slice hold in the header's DX10 format.
[[nodiscard]] std::uint64_t ExpectedPayloadBytes(const DdsHeader& _header) noexcept;

} // namespace Neuron
