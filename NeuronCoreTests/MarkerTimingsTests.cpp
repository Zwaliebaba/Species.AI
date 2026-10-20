#include "pch.h"

#include "MarkerTimings.h"
#include "Profile.h"
#include "TestSupport.h"

#include <cstdint>
#include <string>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

namespace
{

const Neuron::MarkerTiming* Find(const std::vector<Neuron::MarkerTiming>& _timings, const std::string& _name)
{
  for (const Neuron::MarkerTiming& timing : _timings)
  {
    if (timing.name == _name)
    {
      return &timing;
    }
  }
  return nullptr;
}

} // namespace

TEST_CLASS(MarkerTimingsTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    Neuron::SetMarkerTiming(false);
    static_cast<void>(Neuron::TakeMarkerTimings());
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dbg080AMarkerIsTimedOnlyWhileTimingIsOn)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-080")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dbg080AMarkerIsTimedOnlyWhileTimingIsOn)
  {
    // ADR-110: a marker records its time only while timing is on, and a take empties the record.
    static_cast<void>(Neuron::TakeMarkerTimings());
    {
      ENGINE_PROFILE_SCOPE("test.marker.off");
    }
    Assert::IsTrue(Find(Neuron::TakeMarkerTimings(), "test.marker.off") == nullptr, L"nothing recorded while off");
    Neuron::SetMarkerTiming(true);
    for (int scope = 0; scope < 3; ++scope)
    {
      ENGINE_PROFILE_SCOPE("test.marker.outer");
      ENGINE_PROFILE_SCOPE("test.marker.inner");
    }
    const std::vector<Neuron::MarkerTiming> timings = Neuron::TakeMarkerTimings();
    const Neuron::MarkerTiming* outer = Find(timings, "test.marker.outer");
    const Neuron::MarkerTiming* inner = Find(timings, "test.marker.inner");
    Assert::IsTrue(outer != nullptr && inner != nullptr, L"both markers recorded");
    ExpectEqual(3, static_cast<std::int64_t>(outer->count), L"each scope counted");
    Assert::IsTrue(outer->nanoseconds >= inner->nanoseconds && inner->nanoseconds >= 0, L"an outer scope's time holds the inner's");
    const std::vector<Neuron::MarkerTiming> again = Neuron::TakeMarkerTimings();
    const Neuron::MarkerTiming* emptied = Find(again, "test.marker.outer");
    Assert::IsTrue(emptied == nullptr || emptied->count == 0, L"a take empties the record");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dbg080TimingsMergeByNameInNameOrder)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-080")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dbg080TimingsMergeByNameInNameOrder)
  {
    // One name at two addresses is one marker, and the record is in name order. The record keeps a name's address, so
    // the second one has static storage too.
    static const char SECOND_ADDRESS[] = "test.merge.b";
    static_cast<void>(Neuron::TakeMarkerTimings());
    Neuron::RecordMarkerTime("test.merge.b", 5);
    Neuron::RecordMarkerTime(SECOND_ADDRESS, 7);
    Neuron::RecordMarkerTime("test.merge.a", 1);
    const std::vector<Neuron::MarkerTiming> timings = Neuron::TakeMarkerTimings();
    const Neuron::MarkerTiming* merged = Find(timings, "test.merge.b");
    Assert::IsTrue(merged != nullptr && merged->count == 2 && merged->nanoseconds == 12, L"one marker");
    for (std::size_t n = 1; n < timings.size(); ++n)
    {
      Assert::IsTrue(timings[n - 1].name < timings[n].name, L"in name order");
    }
  }
};

} // namespace NeuronCoreTests
