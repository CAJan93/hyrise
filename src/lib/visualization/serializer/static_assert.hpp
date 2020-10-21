#pragma once

// TODO(CAJan93): Remove this function. This is from assert.hpp
template <bool b>
struct StaticAssert {};

// TODO(CAJan93): Remove this function. This is from assert.hpp
// template specialized on true
template <>
struct StaticAssert<true> {
  static void stat_assert(const std::string& msg) { (void)msg; }
};