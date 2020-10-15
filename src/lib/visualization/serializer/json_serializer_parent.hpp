#pragma once

#include <aws/core/utils/json/JsonSerializer.h>
#include <magic_enum.hpp>

#include <string>

#include "../../utils/assert.hpp"
#include "../types/is_integral.hpp"

namespace opossum {

// convenience alii for AWS json
using jsonVal = Aws::Utils::Json::JsonValue;
using jsonView = Aws::Utils::Json::JsonView;

// alias for "boost string"
using string_t =
    std::__cxx11::basic_string<char, std::char_traits<char>, boost::container::pmr::polymorphic_allocator<char>>;

// TODO(CAJan93): Remove this function. This is from helper.hpp
/**
 * Joins the provided arguments into a string using a stringstream.
 * @param args The values to join into a string
 * @return Returns a string
 */
inline constexpr auto JOIN_TO_STR = [](auto... args) -> std::string {
  std::stringstream strs;
  strs << std::boolalpha;
  (strs << ... << args);
  return strs.str();
};

/**
 * Provides utility functions for aws json to the JsonSerializer.
 */
class JsonSerializerParent {
  // used to align error messages after \n char
  static inline const std::string newline_spacer = "\n           ";

  // Call a function f with all integers from the integer sequence
  template <typename T, T... S, typename F>
  static constexpr void for_sequence(std::integer_sequence<T, S...>, F&& f);

  // asserts that the json value hold the a key with a value T
  template <typename T>
  static void key_of_type_exists(const jsonView& value, const std::string& key);

  // retrieve data of type T from Json at key
  template <typename T>
  static T get_entry_from_json(const jsonView& value, const std::string& key);

  // represnets the different expression types
  enum class PredicateConditionExpression { Between, BinaryPredicate, In, IsNull };

  // mapps a PredicateCondition to the different Expression types
  static JsonSerializerParent::PredicateConditionExpression resolve_predicate_condition(
      const PredicateCondition pred_cond);

  friend class JsonSerializer;
};

template <typename T, T... S, typename F>
constexpr void JsonSerializerParent::for_sequence(std::integer_sequence<T, S...>, F&& f) {
  using unpack_t = int[];
  (void)unpack_t{(static_cast<void>(f(std::integral_constant<T, S>{})), 0)..., 0};
}

template <typename T>
void JsonSerializerParent::key_of_type_exists(const jsonView& value, const std::string& key) {
  Assert(value.KeyExists(key), JOIN_TO_STR("key '", key, "' does not exist in json ", value.WriteReadable()));
  if constexpr (is_integral<T>::value) {
  } else if constexpr (std::is_floating_point<T>::value) {
    Assert(value.GetObject(key).IsFloatingPointType(),
           JOIN_TO_STR("value at key '", key, "' is not of floating point type in json", newline_spacer,
                       value.WriteReadable()));
  } else if constexpr (std::is_same<bool, T>::value) {
    Assert(value.GetObject(key).IsBool(), JOIN_TO_STR("value of key '", key, "' is not of boolean type in json",
                                                      newline_spacer, value.WriteReadable()));
  } else if constexpr (std::is_same<std::string, T>::value || std::is_same<string_t, T>::value) {
    Assert(value.GetObject(key).IsString(), JOIN_TO_STR("value of key '", key, "' is not of string type in json",
                                                        newline_spacer, value.WriteReadable()));
  } else if constexpr (std::is_same<jsonView, T>::value) {
    Assert(value.GetObject(key).IsObject(),
           JOIN_TO_STR("value of key '", key, "' is not an object in json", newline_spacer, value.WriteReadable()));
  } else {
    Fail(JOIN_TO_STR("Unable to retrieve data from object of type ", typeid(T).name(), newline_spacer, "Data was",
                     newline_spacer, value.WriteReadable(), newline_spacer, "Key was: '", key, "'"));
  }
}

template <typename T>
T JsonSerializerParent::get_entry_from_json(const jsonView& value, const std::string& key) {
  key_of_type_exists<T>(value, key);
  if constexpr (is_integral<T>::value) {
    return static_cast<T>(value.GetInteger(key));
  } else if constexpr (std::is_floating_point<T>::value) {
    return static_cast<T>(value.GetDouble(key));
  } else if constexpr (std::is_same<bool, T>::value) {
    return static_cast<T>(value.GetBool(key));
  } else if constexpr (std::is_same<std::string, T>::value || std::is_same<string_t, T>::value) {
    return static_cast<T>(value.GetString(key));
  } else if constexpr (std::is_same<jsonView, T>::value) {
    return value.GetObject(key);
  } else {
    Fail(JOIN_TO_STR("Unable to retrieve data from object of type ", typeid(T).name(), newline_spacer, "Data was",
                     newline_spacer, value.WriteReadable(), newline_spacer, "Key was: '", key, "'"));
  }
}
}  // namespace opossum