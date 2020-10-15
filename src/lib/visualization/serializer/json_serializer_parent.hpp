#pragma once

#include <aws/core/utils/json/JsonSerializer.h>
#include <magic_enum.hpp>
#include <string>

#include "../../operators/abstract_aggregate_operator.hpp"
#include "../../operators/abstract_operator.hpp"
#include "../../operators/alias_operator.hpp"
#include "../../operators/get_table.hpp"
#include "../../operators/projection.hpp"
#include "../../operators/table_scan.hpp"
#include "../../operators/validate.hpp"
#include "../../utils/assert.hpp"
#include "../types/is_integral.hpp"
#include "json_serializer.hpp"

namespace opossum {

class Alias;
class GetTable;
class Projection;
class TableScan;
class Validate;

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
 * Provides utility functions to the json serializer and the json deserializer.
 */
class JsonSerializerParent {
  enum class SerializationMode { Serialize, Deserialize };
  /*
   * call from_json or to_json, depending if you pass Serialize or Deserialize as the mode
   * e.g. Will call f<AliasOperator>(args...), if op_t == OperatorType::AliasOperator 
  */
  template <typename... Args>
  auto run_function_on_operator_type(const OperatorType op_t, Args... args,
                                     JsonSerializerParent::SerializationMode mode);

  friend class JsonSerializer;
};

template <typename... Args>
auto JsonSerializerParent::run_function_on_operator_type(const OperatorType op_t, Args... args,
                                                         JsonSerializerParent::SerializationMode mode) {
  const bool serialize = mode == SerializationMode::Serialize ? true : false;
  switch (op_t) {
    case OperatorType::Alias:
      if (serialize) {
        std::cout << "to_json Alias" << std::endl;  // TODO(CAJan93): remove debug msg
        to_json<AliasOperator>(std::forward<Args>(args)...);
        return;
      }
      std::cout << "to_json Alias" << std::endl;  // TODO(CAJan93): remove debug msg
      return from_json<AliasOperator>(std::forward<Args>(args)...);

    case OperatorType::GetTable:
      if (serialize) {
        std::cout << "to_json GetTable" << std::endl;  // TODO(CAJan93): remove debug msg
        to_json<GetTable>(std::forward<Args>(args)...);
        return;
      }
      std::cout << "to_json GetTable" << std::endl;  // TODO(CAJan93): remove debug msg
      return from_json<GetTable>(std::forward<Args>(args)...);

    case OperatorType::Projection:
      if (serialize) {
        std::cout << "to_json Projection" << std::endl;  // TODO(CAJan93): remove debug msg
        to_json<Projection>(std::forward<Args>(args)...);
        return;
      }
      std::cout << "to_json Projection" << std::endl;  // TODO(CAJan93): remove debug msg
      return from_json<Projection>(std::forward<Args>(args)...);

    case OperatorType::TableScan:
      if (serialize) {
        std::cout << "to_json TableScan" << std::endl;  // TODO(CAJan93): remove debug msg
        to_json<TableScan>(std::forward<Args>(args)...);
        return;
      }
      std::cout << "to_json TableScan" << std::endl;  // TODO(CAJan93): remove debug msg
      return from_json<TableScan>(std::forward<Args>(args)...);

    case OperatorType::Validate:
      if (serialize) {
        std::cout << "to_json Validate" << std::endl;  // TODO(CAJan93): remove debug msg
        to_json<Validate>(std::forward<Args>(args)...);
        return;
      }
      std::cout << "to_json Validate" << std::endl;  // TODO(CAJan93): remove debug msg
      return from_json<Validate>(std::forward<Args>(args)...);

    default:
      Fail(JOIN_TO_STR("Unexprected OperatorType ", magic_enum::enum_name(op_t).data()));
  }
}
}  // namespace opossum