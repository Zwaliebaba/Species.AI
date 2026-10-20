#include "pch.h"

#include "DdsHeader.h"
#include "DdsTexture.h"
#include "FrameView.h"
#include "ShaderLayouts.h"
#include "SkyLayers.h"
#include "TestSupport.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <numbers>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronClientTests
{

namespace
{

// A point in render space through the frame's view and projection: its depth and its normalised x and y.
std::array<double, 3> Project(const Neuron::FrameView& _frame, const Neuron::Vec3& _point)
{
  const Neuron::Float4x4& m = _frame.viewProjection;
  std::array<double, 4> clip{};
  for (std::size_t c = 0; c < 4; ++c)
  {
    clip[c] = _point.x * m.m[0][c] + _point.y * m.m[1][c] + _point.z * m.m[2][c] + m.m[3][c];
  }
  return {clip[0] / clip[3], clip[1] / clip[3], clip[2] / clip[3]};
}

// A DDS file with a DX10 header for a texture of _format, _width × _height and a full mip chain, its payload zero.
std::vector<std::byte> DdsFile(std::uint32_t _format, std::uint32_t _width, std::uint32_t _height, std::uint32_t _levels,
                               std::uint64_t _payload)
{
  std::vector<std::byte> file(Neuron::DDS_HEADER_BYTES + Neuron::DDS_DX10_HEADER_BYTES + _payload);
  const auto put = [&file](std::size_t _at, std::uint32_t _value) { std::memcpy(file.data() + _at, &_value, sizeof(_value)); };
  put(0, 0x2053'4444u); // "DDS "
  put(4, 124);          // the header's size
  put(8, 0x1 | 0x2 | 0x4 | 0x1000 | 0x20000);
  put(12, _height);
  put(16, _width);
  put(28, _levels);
  put(76, 32);           // the pixel format's size
  put(80, 0x4);          // DDPF_FOURCC
  put(84, 0x3031'5844u); // "DX10"
  put(128, _format);
  put(132, Neuron::DDS_TEXTURE_2D);
  put(140, 1); // one slice
  return file;
}

} // namespace

TEST_CLASS(WorldFrameTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Rnd210TheNearPlaneIsDepthOneAndTheFarPlaneDepthZero)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-210")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-211")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Rnd210TheNearPlaneIsDepthOneAndTheFarPlaneDepthZero)
  {
    Neuron::CameraPose pose;
    pose.verticalFovRadians = std::numbers::pi / 3.0;
    const Neuron::FrameView frame = Neuron::MakeFrameView(pose, 16.0 / 9.0, {});
    // A yaw of 0 looks along -z (ADR-016).
    Assert::AreEqual(1.0, Project(frame, {0.0, 0.0, -5.0})[2], 1e-5, L"the near plane at 5 wu is depth 1");
    Assert::AreEqual(0.0, Project(frame, {0.0, 0.0, -15'000.0})[2], 1e-5, L"the far plane at 15,000 wu is depth 0");
    Assert::IsTrue(Project(frame, {0.0, 0.0, -100.0})[2] > Project(frame, {0.0, 0.0, -1'000.0})[2], L"nearer is greater");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Rnd212TheVerticalFieldIsKeptAtEveryAspectAndClamped)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-027")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-212")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Rnd212TheVerticalFieldIsKeptAtEveryAspectAndClamped)
  {
    Neuron::CameraPose pose;
    pose.verticalFovRadians = std::numbers::pi / 3.0;
    // A point on the top edge of a 60° field, 1,000 wu ahead, stays on it at 4:3, 16:9 and 21:9 (Hor+).
    const double top = 1'000.0 * std::tan(std::numbers::pi / 6.0);
    for (const double aspect : {4.0 / 3.0, 16.0 / 9.0, 21.0 / 9.0})
    {
      Assert::AreEqual(1.0, Project(Neuron::MakeFrameView(pose, aspect, {}), {0.0, top, -1'000.0})[1], 1e-5);
    }
    // RND-212: 0.5° is drawn at 1°, so the edge of a 1° field is the top of the image.
    pose.verticalFovRadians = 0.5 * std::numbers::pi / 180.0;
    const double edge = 1'000.0 * std::tan(0.5 * std::numbers::pi / 180.0); // the half-angle of 1°
    Assert::AreEqual(1.0, Project(Neuron::MakeFrameView(pose, 1.0, {}), {0.0, edge, -1'000.0})[1], 1e-4);
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Wld246ACameraRelativeOffsetIsExactToAHundredthAtTheFarCorner)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-246")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Wld246ACameraRelativeOffsetIsExactToAHundredthAtTheFarCorner)
  {
    // The far corner of the reserved extent, seen from 100 wu away: the binary64 difference is rounded once.
    const Neuron::Vec3 origin{71'679.99, 12.345, 71'679.99};
    const Neuron::Vec3 camera{71'579.98, 500.0, 71'600.02};
    const Neuron::Float3x4 world = Neuron::CameraRelativeTransform(origin, camera);
    Assert::AreEqual(origin.x - camera.x, static_cast<double>(world.m[0][3]), 0.01);
    Assert::AreEqual(origin.y - camera.y, static_cast<double>(world.m[1][3]), 0.01);
    Assert::AreEqual(origin.z - camera.z, static_cast<double>(world.m[2][3]), 0.01);
    Assert::AreEqual(1.0f, world.m[0][0]);
    Assert::AreEqual(0.0f, world.m[0][1]);
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Rnd138AChunkOutsideTheViewIsCulledAndOneAcrossItsEdgeIsNot)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-138")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-236")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Rnd138AChunkOutsideTheViewIsCulledAndOneAcrossItsEdgeIsNot)
  {
    Neuron::CameraPose pose;
    pose.verticalFovRadians = std::numbers::pi / 3.0;
    const Neuron::FrameView frame = Neuron::MakeFrameView(pose, 16.0 / 9.0, {});
    const std::array<Neuron::Float4, 6> planes = Neuron::FrustumPlanes(frame);
    Assert::IsTrue(Neuron::BoxMayBeVisible(planes, {-100.0, -100.0, -1'100.0}, {100.0, 100.0, -900.0}), L"ahead");
    Assert::IsFalse(Neuron::BoxMayBeVisible(planes, {-100.0, -100.0, 900.0}, {100.0, 100.0, 1'100.0}), L"behind");
    Assert::IsFalse(Neuron::BoxMayBeVisible(planes, {-100.0, -100.0, -17'000.0}, {100.0, 100.0, -16'000.0}), L"beyond the far plane");
    Assert::IsFalse(Neuron::BoxMayBeVisible(planes, {5'000.0, -100.0, -1'100.0}, {5'200.0, 100.0, -900.0}), L"far to the right");
    Assert::IsTrue(Neuron::BoxMayBeVisible(planes, {-1'000.0, -1'000.0, -1'000.0}, {1'000.0, 1'000.0, 1'000.0}), L"around the camera");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Rnd151TheSkyHasItsGridLinesAndItsFiveCloudLayers)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-151")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-153")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Rnd151TheSkyHasItsGridLinesAndItsFiveCloudLayers)
  {
    Neuron::SkyLook sky;
    sky.regionOriginX = 30'720.0;
    sky.regionOriginZ = 30'720.0;
    sky.regionCenter = {33'280.0, 0.0, 33'280.0};
    const Neuron::Vec3 camera{33'280.0, 700.0, 36'240.0};
    const std::vector<Neuron::SkyLayerInstance> layers = Neuron::BuildSkyLayers(sky, camera, 0.0, 7, 9);
    Assert::AreEqual(std::size_t{125 + 125 + 5}, layers.size());
    // The lines stand 1,200 wu up on the 80 wu lattice of the region's origin, relative to the camera.
    Assert::AreEqual(500.0f, layers[0].shape.x);
    for (std::size_t i = 0; i < 125; ++i)
    {
      const double x = camera.x + 0.5 * (static_cast<double>(layers[i].rectangle.x) + static_cast<double>(layers[i].rectangle.z));
      Assert::AreEqual(0.0, std::fmod(x - sky.regionOriginX, 80.0), 1e-3);
      Assert::AreEqual(Neuron::SKY_ALONG_Z, layers[i].sampling.z);
    }
    Assert::AreEqual(16.0f, layers[130].rectangle.w - layers[130].rectangle.y, L"each line is 16 wu wide");
    // Layer 3 at 800 wu; the flat layers wrap in the shader and read the first level only.
    Assert::AreEqual(100.0f, layers[252].shape.x);
    Assert::AreEqual(Neuron::SKY_LEVEL_ZERO | Neuron::SKY_WRAP_IN_SHADER, layers[253].sampling.z);
    Assert::AreEqual(9u, layers[253].sampling.x);
    Assert::AreEqual(7u, layers[0].sampling.x);
    // RND-152: the offset moves with the displayed ticks, in texture repeats.
    const std::vector<Neuron::SkyLayerInstance> later = Neuron::BuildSkyLayers(sky, camera, 10.0, 7, 9);
    Assert::AreEqual(0.03f, later[250].uv.x - layers[250].uv.x, 1e-6f);
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr017ADdsTextureIsDescribedLevelByLevel)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-009")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr017ADdsTextureIsDescribedLevelByLevel)
  {
    // BC4 128 × 32 with 8 levels: 32 × 8 blocks of 8 bytes, down to one block per level.
    std::uint64_t payload = 0;
    for (std::uint32_t level = 0; level < 8; ++level)
    {
      payload += static_cast<std::uint64_t>(std::max(32u >> level, 1u)) * std::max(8u >> level, 1u) * 8;
    }
    const std::vector<std::byte> file = DdsFile(80, 128, 32, 8, payload);
    const auto texture = Neuron::DescribeDds(file, "fx.beam_short");
    Assert::IsTrue(texture.has_value());
    Assert::AreEqual(8u, texture->desc.levels);
    Assert::IsTrue(texture->desc.format == DXGI_FORMAT_BC4_UNORM);
    Assert::AreEqual(std::size_t{8}, texture->levels.size());
    Assert::AreEqual(256u, texture->levels[0].rowPitchBytes, L"a row of 32 blocks");
    Assert::AreEqual(std::size_t{2048}, texture->levels[0].bytes.size(), L"32 × 8 blocks of 8 bytes");
    Assert::AreEqual(8u, texture->levels[7].rowPitchBytes, L"the 1 × 1 level is one block");
    // An sRGB texture keeps its format: the factory creates it typeless and views it UNORM (ADR-013).
    const std::vector<std::byte> srgb = DdsFile(99, 4, 4, 1, 16);
    const auto tile = Neuron::DescribeDds(srgb, "terrain.sea_tile");
    Assert::IsTrue(tile.has_value() && tile->desc.format == DXGI_FORMAT_BC7_UNORM_SRGB);
    Assert::IsFalse(Neuron::DescribeDds(DdsFile(80, 128, 32, 8, 100), "short").has_value(), L"a payload shorter than its levels");
  }
};

} // namespace NeuronClientTests
