#include "pch.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

// The placeholder suite (AGENTS.md §3, ADR-069): vstest reports a suite with no tests as a pass, so every test
// project holds this class until its first real test lands, and it is deleted then, never before.
TEST_CLASS(SuiteSmoke)
{
public:
  TEST_METHOD(SuiteRuns)
  {
    Assert::IsTrue(true);
  }
};

} // namespace NeuronCoreTests
