#include "pch.h"

#include "NineSlice.h"
#include "TestSupport.h"

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronClientTests
{

namespace
{

// The accent panel's frame as the owner drew it (ADR-175): 128 × 1020 texels, slices of 40, 25, 24 and 6.
Neuron::NineSlice AccentPlate()
{
  return {{128, 1020}, {40.0f, 25.0f, 24.0f, 6.0f}};
}

std::vector<Neuron::UiQuad> QuadsOf(const std::vector<Neuron::UiBatch>& _batches)
{
  std::vector<Neuron::UiQuad> quads;
  for (const Neuron::UiBatch& batch : _batches)
  {
    Assert::IsTrue(batch.source == Neuron::UiSource::Image && batch.blend == Neuron::UiBlend::Alpha && batch.image == 3, L"the image");
    quads.insert(quads.end(), batch.quads.begin(), batch.quads.end());
  }
  return quads;
}

double Area(const std::vector<Neuron::UiQuad>& _quads)
{
  double area = 0.0;
  for (const Neuron::UiQuad& quad : _quads)
  {
    area += static_cast<double>(quad.rectangle[2] - quad.rectangle[0]) * static_cast<double>(quad.rectangle[3] - quad.rectangle[1]);
  }
  return area;
}

} // namespace

TEST_CLASS(NineSliceTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Ui054APlateKeepsItsFrameAndStretchesItsMiddleAcross)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-054")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui054APlateKeepsItsFrameAndStretchesItsMiddleAcross)
  {
    const Neuron::NineSlice plate = AccentPlate();
    const Neuron::UiRect box{10.0f, 20.0f, 410.0f, 220.0f};
    Assert::AreEqual(1.0, static_cast<double>(Neuron::NineSliceScale(plate, box, 1.0f)), 1e-9, L"the box holds the slices");
    std::vector<Neuron::UiBatch> batches;
    Neuron::AppendNineSlice(batches, 3, plate, box, 1.0f, {1.0f, 1.0f, 1.0f, 0.5f});
    const std::vector<Neuron::UiQuad> quads = QuadsOf(batches);
    ExpectEqual(9, static_cast<std::int64_t>(quads.size()), L"three rows of three");
    Assert::AreEqual(400.0 * 200.0, Area(quads), 1e-3, L"the quads cover the box once");
    // The top-left corner keeps its texels: 40 × 25 px of u 0 to 40/128 and v 0 to 25/1020.
    const Neuron::UiQuad& corner = quads[0];
    Assert::IsTrue(corner.rectangle == std::array<float, 4>{10.0f, 20.0f, 50.0f, 45.0f}, L"the corner's rectangle");
    Assert::AreEqual(40.0 / 128.0, static_cast<double>(corner.uv[2]), 1e-6, L"the corner's texels across");
    Assert::AreEqual(25.0 / 1020.0, static_cast<double>(corner.uv[3]), 1e-6, L"the corner's texels down");
    Assert::AreEqual(0.5, static_cast<double>(corner.color[3]), 1e-9, L"its color");
    // The middle stretches across, from 40/128 to 104/128, and is cut down: 169 px of its rows at one texel a pixel.
    const Neuron::UiQuad& middle = quads[4];
    Assert::IsTrue(middle.rectangle == std::array<float, 4>{50.0f, 45.0f, 386.0f, 214.0f}, L"the middle's rectangle");
    Assert::AreEqual(104.0 / 128.0, static_cast<double>(middle.uv[2]), 1e-6, L"the middle's texels across");
    Assert::AreEqual((25.0 + 169.0) / 1020.0, static_cast<double>(middle.uv[3]), 1e-6, L"cut, not squeezed");
    // The bottom-right corner ends the image.
    const Neuron::UiQuad& last = quads[8];
    Assert::IsTrue(last.rectangle == std::array<float, 4>{386.0f, 214.0f, 410.0f, 220.0f}, L"the last corner");
    Assert::IsTrue(last.uv[2] == 1.0f && last.uv[3] == 1.0f, L"at the image's end");
    const Neuron::UiRect inner = Neuron::NineSliceInner(plate, box, 1.0f);
    Assert::IsTrue(inner == Neuron::UiRect{50.0f, 45.0f, 386.0f, 214.0f}, L"inside the frame");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ui054ATallPlateRepeatsItsMiddleRows)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-054")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui054ATallPlateRepeatsItsMiddleRows)
  {
    // 989 texels of middle rows at 2 px a texel: 1,978 px a run, so a box of 25 × 2 + 3,000 + 6 × 2 px takes two.
    const Neuron::NineSlice plate = AccentPlate();
    const Neuron::UiRect box{0.0f, 0.0f, 200.0f, 3062.0f};
    std::vector<Neuron::UiBatch> batches;
    Neuron::AppendNineSlice(batches, 3, plate, box, 2.0f, {1.0f, 1.0f, 1.0f, 1.0f});
    const std::vector<Neuron::UiQuad> quads = QuadsOf(batches);
    ExpectEqual(std::int64_t{3} * 4, static_cast<std::int64_t>(quads.size()), L"the top, two runs of the middle and the bottom");
    Assert::AreEqual(200.0 * 3062.0, Area(quads), 1e-3, L"the quads cover the box once");
    for (const Neuron::UiQuad& quad : quads)
    {
      Assert::IsTrue(quad.uv[1] >= 0.0f && quad.uv[3] <= 1.0f && quad.uv[1] < quad.uv[3], L"within the image");
    }
    Assert::AreEqual(1.0 - 6.0 / 1020.0, static_cast<double>(quads[5].uv[3]), 1e-6, L"the first run ends at the middle's end");
    Assert::AreEqual(25.0 / 1020.0, static_cast<double>(quads[6].uv[1]), 1e-6, L"the second starts at its start");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ui054ASmallBoxShrinksTheFrame)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-054")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui054ASmallBoxShrinksTheFrame)
  {
    const Neuron::NineSlice plate = AccentPlate();
    // 32 px across holds 64 texels of slices at half a pixel each; 31 px down holds 31 at one.
    Assert::AreEqual(0.5, static_cast<double>(Neuron::NineSliceScale(plate, {0.0f, 0.0f, 32.0f, 31.0f}, 1.0f)), 1e-9, L"across");
    Assert::AreEqual(0.5, static_cast<double>(Neuron::NineSliceScale(plate, {0.0f, 0.0f, 100.0f, 15.5f}, 1.0f)), 1e-9, L"down");
    Assert::AreEqual(1.0, static_cast<double>(Neuron::NineSliceScale(plate, {0.0f, 0.0f, 100.0f, 100.0f}, 1.0f)), 1e-9, L"no less");
    // An image of no known size is drawn whole.
    std::vector<Neuron::UiBatch> batches;
    Neuron::AppendNineSlice(batches, 3, Neuron::NineSlice{}, {0.0f, 0.0f, 20.0f, 10.0f}, 1.0f, {1.0f, 1.0f, 1.0f, 1.0f});
    const std::vector<Neuron::UiQuad> quads = QuadsOf(batches);
    ExpectEqual(1, static_cast<std::int64_t>(quads.size()), L"one quad");
    Assert::IsTrue(quads[0].uv == std::array<float, 4>{0.0f, 0.0f, 1.0f, 1.0f}, L"the whole image");
  }
};

} // namespace NeuronClientTests
