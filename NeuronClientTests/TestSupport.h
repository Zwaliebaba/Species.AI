#pragma once

#include "CppUnitTest.h"
#include "WindowGeometry.h"

#include <cstdint>
#include <format>
#include <string_view>

namespace NeuronClientTests
{

// The framework's AreEqual needs a ToString for every type it compares; these compare and describe the values
// themselves.
inline void ExpectEqual(std::int64_t _expected, std::int64_t _actual, std::wstring_view _what)
{
  if (_expected != _actual)
  {
    Microsoft::VisualStudio::CppUnitTestFramework::Assert::Fail(
      std::format(L"{}: expected {}, found {}", _what, _expected, _actual).c_str());
  }
}

inline void ExpectText(std::string_view _expected, std::string_view _actual, std::wstring_view _what)
{
  if (_expected != _actual)
  {
    Microsoft::VisualStudio::CppUnitTestFramework::Assert::Fail(
      std::format(L"{}: expected {} bytes, found {} bytes", _what, _expected.size(), _actual.size()).c_str());
  }
}

inline void ExpectContains(std::string_view _text, std::string_view _part, std::wstring_view _what)
{
  if (_text.find(_part) == std::string_view::npos)
  {
    Microsoft::VisualStudio::CppUnitTestFramework::Assert::Fail(std::format(L"{}: the text lacks a part", _what).c_str());
  }
}

inline void ExpectRect(const Neuron::RectPx& _expected, const Neuron::RectPx& _actual, std::wstring_view _what)
{
  if (_expected != _actual)
  {
    Microsoft::VisualStudio::CppUnitTestFramework::Assert::Fail(
      std::format(L"{}: expected ({}, {}, {}, {}), found ({}, {}, {}, {})", _what, _expected.left, _expected.top, _expected.right,
                  _expected.bottom, _actual.left, _actual.top, _actual.right, _actual.bottom)
        .c_str());
  }
}

inline void ExpectSize(const Neuron::SizePx& _expected, const Neuron::SizePx& _actual, std::wstring_view _what)
{
  if (_expected != _actual)
  {
    Microsoft::VisualStudio::CppUnitTestFramework::Assert::Fail(
      std::format(L"{}: expected {}x{}, found {}x{}", _what, _expected.width, _expected.height, _actual.width, _actual.height).c_str());
  }
}

} // namespace NeuronClientTests
