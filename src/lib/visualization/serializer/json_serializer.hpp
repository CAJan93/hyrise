#pragma once

#include <iostream>
#include <map>
#include <string>
#include <tuple>
#include <variant>

#include <aws/core/utils/json/JsonSerializer.h>
#include <magic_enum.hpp>

// TODO(CAJan93): #include "../assert.hpp"
// TODO(CAJan93): #include "../string.hpp"
// TODO(CAJan93): #include "assert.hpp"
// TODO(CAJan93): #include "../types/types.hpp"
#include "../../expression/abstract_expression.hpp"
#include "../../expression/abstract_predicate_expression.hpp"
#include "../../expression/aggregate_expression.hpp"
#include "../../expression/arithmetic_expression.hpp"
#include "../../expression/between_expression.hpp"
#include "../../expression/binary_predicate_expression.hpp"
#include "../../expression/case_expression.hpp"
#include "../../expression/cast_expression.hpp"
#include "../../expression/exists_expression.hpp"
#include "../../expression/extract_expression.hpp"
#include "../../expression/in_expression.hpp"
#include "../../expression/is_null_expression.hpp"
#include "../../expression/list_expression.hpp"
#include "../../expression/logical_expression.hpp"
#include "../../expression/placeholder_expression.hpp"
#include "../../expression/pqp_column_expression.hpp"
#include "../../expression/pqp_subquery_expression.hpp"
#include "../../expression/unary_minus_expression.hpp"
#include "../../expression/value_expression.hpp"
#include "../../logical_query_plan/predicate_node.hpp"
#include "../../operators/abstract_aggregate_operator.hpp"
#include "../../operators/abstract_operator.hpp"
#include "../../operators/aggregate_hash.hpp"
#include "../../operators/aggregate_sort.hpp"
#include "../../operators/alias_operator.hpp"
#include "../../operators/get_table.hpp"
#include "../../operators/limit.hpp"
#include "../../operators/projection.hpp"
#include "../../operators/table_scan.hpp"
#include "../../operators/validate.hpp"
#include "../../types.hpp"
#include "../../utils/assert.hpp"
#include "../types/get_inner_type.hpp"
#include "../types/is_integral.hpp"
#include "../types/is_smart_ptr.hpp"
#include "../types/is_vector.hpp"
#include "has_member.hpp"
#include "json_serializer_parent.hpp"
#include "string.hpp"

namespace opossum {

// forward declaration and aliases
class AbstractExpression;
class AbstractAggregateOperator;
class AggregateExpression;
class AggregateHash;
class AggregateSort;
class AliasOperator;
class ArithmeticExpression;
class BetweenExpression;
class BinaryPredicateExpression;
class GetTable;
class InExpression;
class IsNullExpression;
class PQPColumnExpression;
class PredicateNode;
class TableScan;
class Validate;
class ValueExpression;

// TODO(CAJan93): Remove this function. This is from assert.hpp
template <bool b>
struct StaticAssert {};

// TODO(CAJan93): Remove this function. This is from assert.hpp
// template specialized on true
template <>
struct StaticAssert<true> {
  static void stat_assert(const std::string& msg) { (void)msg; }
};

class JsonSerializer : public JsonSerializerParent {
  /***
  * convert a vector<T> into a json object
  * {
  *  "0": 1
  *  "1": {
  *      // some nested value
  *    }
  * }
  */
  template <typename T>
  static jsonVal vec_to_json(const std::vector<T>& vec);

  // retrieve data from json
  template <typename T>
  static T as_any(const jsonView&, const std::string&);

  // set data in json
  template <typename T>
  static void with_any(jsonVal& data, const std::string& key, const T& val);

 public:
  // deserialize
  template <typename T>
  static T from_json(const jsonView& data);

  // serialize from string. Wrapper for from_json
  template <typename T>
  static auto from_json_str(const std::string& json_str);

  // serialize
  template <typename T>
  static jsonVal to_json(const T& object);

  // serialize to string. Wrapper for to_json
  template <typename T>
  static std::string to_json_str(const T& object);
};

/**
 * AllTypeVariant is encoded like this:
 * "value": {
 *      "val_t":  5
 *      "val":    "some string"
 * }
 * Retrieve an AllTypeVariant by passing the key "value". This function will then use the key
 * "val_t" to determine the type of the value. The key "val" will be used to retrieve the value
 */
template <>
inline AllTypeVariant JsonSerializer::as_any<AllTypeVariant>(const jsonView& value, const std::string& key) {
  const jsonView inner_json_view = get_entry_from_json<jsonView>(value, "value");
  const int value_t = get_entry_from_json<int>(inner_json_view, "val_t");
  switch (value_t) {
    case 0:
      return nullptr;
    case 1:
    case 2:
      return get_entry_from_json<int>(inner_json_view, "val");
    case 3:
    case 4:
      return get_entry_from_json<double>(inner_json_view, "val");
    case 5:
      return get_entry_from_json<string_t>(inner_json_view, "val");

    default:
      Fail(JOIN_TO_STR("Unable to retrieve value for AllTypeVariant. Json was ", inner_json_view.WriteReadable()));
  }
  return -1;
}

// for non-trivial objects
template <typename T>
inline T JsonSerializer::as_any(const jsonView& value, const std::string& key) {
  typedef typename std::remove_cv_t<T> without_cv_t;
  typedef typename std::remove_pointer_t<without_cv_t> without_cv_value_t;

  // e.g. as_any<std::string>(v, k) == as_any<const std::string>(v, k)
  if constexpr (!std::is_same<without_cv_t, T>::value) return as_any<without_cv_t>(value, key);

  // simple types
  if constexpr (is_integral<T>::value || std::is_same<double, T>::value || std::is_same<bool, T>::value ||
                std::is_same<std::string, T>::value || std::is_same<string_t, T>::value) {
    return get_entry_from_json<T>(value, key);
  }

  if constexpr (std::is_pointer<without_cv_t>::value) {
    // nullpointers
    if (value.GetObject(key).IsString() && value.GetString(key) == "NULL") {
      return nullptr;
    }
    if (value.GetObject(key).IsObject()) {
      if constexpr (has_member_properties<without_cv_value_t>::value ||
                    std::is_same<without_cv_value_t, AbstractOperator>::value ||
                    std::is_same<without_cv_value_t, AbstractExpression>::value) {
        // handle nested object (pointer)
        const jsonView sub_json = get_entry_from_json<jsonView>(value, key);
        // T:: properties exist
        without_cv_t sub_obj = from_json<without_cv_t>(sub_json);
        return sub_obj;
      }
      Fail(JOIN_TO_STR("Unable to process key ", key, newline_spacer, "Key is a raw pointer with the type ",
                       typeid(T).name(), "*. Current JSON object is", newline_spacer, value.WriteReadable()));
    } else {
      /*
      TODO(CAJan93): Hotfix. I think I get an issue here, because the
      compiler does not know that this path is never chosen at runtime with
      without_cv_value_t == AbstractOperator
      */
      if constexpr (!std::is_same<without_cv_value_t, AbstractOperator>::value &&
                    !std::is_same<without_cv_value_t, AbstractExpression>::value) {
        without_cv_value_t sub_obj = as_any<without_cv_value_t>(value, key);
        without_cv_value_t* new_sub_obj = new without_cv_value_t{sub_obj};
        return new_sub_obj;
      } else {
        const std::string type =
            std::is_same<without_cv_value_t, AbstractOperator>::value ? "AbstractOperator" : "AbstractExpression";
        Fail(JOIN_TO_STR("Unable to serialize abstract type ", type));
      }
    }
  } else if constexpr (is_weak_ptr<T>::value) {
    Fail("weak ptr currently not supported");
    return -1;
  } else if constexpr (is_smart_ptr<without_cv_t>::value) {
    StaticAssert<!is_unique_ptr<without_cv_t>::value>::stat_assert(
        "Unique pointers are currently not supported by this json serializer");

    typedef typename std::remove_cv_t<without_cv_t> smart_ptr_t;
    typedef get_inner_t<smart_ptr_t> inner_t;            // type of the object the pointer is pointing to
    inner_t* object_ptr = as_any<inner_t*>(value, key);  // a pointer to such an object

    return smart_ptr_t(object_ptr);
  } else {
    if constexpr (std::is_enum<without_cv_t>::value) {
      key_of_type_exists<std::string>(value, key);
      const std::string enum_str = get_entry_from_json<std::string>(value, key);
      auto enum_opt = magic_enum::enum_cast<without_cv_t>(get_entry_from_json<std::string>(value, key));
      if (!enum_opt.has_value()) {
        Fail(
            JOIN_TO_STR("Unable to create enum of type ", typeid(without_cv_t).name(), "from string '", enum_str, "'"));
      }
      return enum_opt.value();
    } else {
      if (value.GetObject(key).IsObject()) {
        if constexpr (has_member_properties<without_cv_t>::value) {
          // handle nested object (pointer or non-pointer
          key_of_type_exists<jsonVal>(value, key);
          jsonView sub_json = get_entry_from_json<jsonView>(value, key);
          without_cv_t sub_obj = from_json<without_cv_t>(sub_json);
          without_cv_t new_sub_obj{sub_obj};
          return new_sub_obj;
        } else if constexpr (is_vector<without_cv_t>::value) {
          // deserialize a vector
          const jsonView obj = get_entry_from_json<jsonView>(value, key);
          without_cv_t vec;
          typedef get_inner_vec_t<without_cv_t> vec_inner_t;
          typedef std::remove_cv_t<vec_inner_t> without_cv_vec_inner_t;

          if constexpr (std::is_pointer<without_cv_vec_inner_t>::value) {
            typedef std::remove_pointer_t<without_cv_vec_inner_t> without_ptr_without_cv_vec_inner_t;
            for (size_t idx = 0; obj.KeyExists(std::to_string(idx)); ++idx) {
              const jsonView data = get_entry_from_json<jsonView>(obj, std::to_string(idx));
              if constexpr (has_member_properties<without_ptr_without_cv_vec_inner_t>::value) {
                without_ptr_without_cv_vec_inner_t* tmp = from_json<without_cv_vec_inner_t>(data);
                vec.emplace_back(tmp);  // serializer only supports vectors and no other containers
              } else if constexpr (std::is_same<without_ptr_without_cv_vec_inner_t, int>::value) {
                without_ptr_without_cv_vec_inner_t* tmp = new without_ptr_without_cv_vec_inner_t(data.AsInteger());
                vec.emplace_back(tmp);
              } else if constexpr (std::is_same<without_ptr_without_cv_vec_inner_t, double>::value) {
                without_ptr_without_cv_vec_inner_t* tmp = new without_ptr_without_cv_vec_inner_t(data.AsDouble());
                vec.emplace_back(tmp);
              } else if constexpr (std::is_same<without_ptr_without_cv_vec_inner_t, std::string>::value) {
                without_ptr_without_cv_vec_inner_t* tmp = new without_ptr_without_cv_vec_inner_t(data.AsString());
                vec.emplace_back(tmp);
              } else if constexpr (std::is_same<without_ptr_without_cv_vec_inner_t, bool>::value) {
                without_ptr_without_cv_vec_inner_t* tmp = new without_ptr_without_cv_vec_inner_t(data.AsBool());
                vec.emplace_back(tmp);
              } else {
                Fail("Unsupported vector type");
              }
            }
          } else if constexpr (is_smart_ptr<without_cv_vec_inner_t>::value) {
            StaticAssert<!is_unique_ptr<without_cv_t>::value>::stat_assert(
                "Unique pointers are currently not supported by this json serializer");
            StaticAssert<!is_weak_ptr<without_cv_t>::value>::stat_assert(
                "Weak pointers are currently not supported by this json serializer");
            typedef without_cv_vec_inner_t smart_ptr_t;  // type of the smart pointer
            typedef get_inner_t<without_cv_vec_inner_t>
                smart_ptr_inner_t;  // type of the object the pointer is pointing to
            for (size_t idx = 0; obj.KeyExists(std::to_string(idx)); ++idx) {
              const jsonView data = get_entry_from_json<jsonView>(obj, std::to_string(idx));
              //  if constexpr (has_member_properties<without_cv_vec_inner_t>::value) {
              smart_ptr_inner_t* t = from_json<smart_ptr_inner_t*>(data);
              smart_ptr_t ptr(t);
              vec.emplace_back(ptr);
            }
          } else {
            for (size_t idx = 0; obj.KeyExists(std::to_string(idx)); ++idx) {
              const jsonView data = obj.GetObject(std::to_string(idx));
              if constexpr (has_member_properties<without_cv_vec_inner_t>::value) {
                vec.emplace_back(from_json<without_cv_vec_inner_t>(
                    data));  // serializer only supports vectors and no other containers
              } else if constexpr (is_integral<without_cv_vec_inner_t>::value) {
                vec.emplace_back(data.AsInteger());
              } else if constexpr (std::is_same<without_cv_vec_inner_t, double>::value) {
                vec.emplace_back(data.AsDouble());
              } else if constexpr (std::is_same<without_cv_vec_inner_t, std::string>::value) {
                vec.emplace_back(data.AsString());
              } else if constexpr (std::is_same<without_cv_vec_inner_t, bool>::value) {
                vec.emplace_back(data.AsBool());
              } else {
                Fail(JOIN_TO_STR("Unsupported vector type '", typeid(without_cv_vec_inner_t).name(), "'. Json is ",
                                 obj.WriteReadable()));
              }
            }
          }
          return vec;
        } else {
          // TODO(CAJan93): same case as above. Simplify!
          const jsonView sub_json = get_entry_from_json<jsonView>(value, key);
          without_cv_t sub_obj = from_json<without_cv_t>(sub_json);
          without_cv_t new_sub_obj{sub_obj};
          return new_sub_obj;
        }
      } else {
        Fail(JOIN_TO_STR("Unable to process key ", key, " Current JSON object is", newline_spacer,
                         value.WriteReadable()));
      }
    }
  }
  Fail("unreachable statement reached");
}

template <>
inline void JsonSerializer::with_any<AllTypeVariant>(jsonVal& data, const std::string& key, const AllTypeVariant& val) {
  const unsigned int val_t = val.which();
  jsonVal variant_jv;
  variant_jv.WithInteger("val_t", val_t);
  if (val_t == 0) {
    variant_jv.WithString("val", "NULL");
  } else if (val_t == 1) {
    variant_jv.WithInteger("val", boost::get<int>(val));
  } else if (val_t == 2) {
    variant_jv.WithInteger("val", boost::get<long int>(val));
  } else if (val_t == 3) {
    variant_jv.WithDouble("val", boost::get<float>(val));
  } else if (val_t == 4) {
    variant_jv.WithDouble("val", boost::get<double>(val));
  } else if (val_t == 5) {
    variant_jv.WithString("val", std::string(boost::get<string_t>(val)));
  }
  data.WithObject(key, variant_jv);
}

template <typename T>
inline void JsonSerializer::with_any(jsonVal& data, const std::string& key, const T& val) {
  // handle simple types
  if constexpr (std::is_same<T, bool>::value) {
    data.WithBool(key, val);
    return;
  }
  if constexpr (std::is_floating_point<T>::value) {
    data.WithDouble(key, val);
    return;
  }
  if constexpr (is_integral<T>::value) {
    data.WithInteger(key, val);
    return;
  }
  if constexpr (std::is_same<T, std::string>::value) {
    data.WithString(key, val);
    return;
  }

  if constexpr (std::is_pointer<T>::value) {
    if (val == nullptr) {
      data.WithString(key, "NULL");
    } else {
      // const AbstractOperator* const&
      typedef typename std::remove_reference_t<std::remove_cv_t<std::remove_pointer_t<T>>> without_ref_cv_ptr_t;
      if constexpr (has_member_properties<without_ref_cv_ptr_t>::value ||
                    std::is_same<without_ref_cv_ptr_t, AbstractExpression>::value ||
                    std::is_same<without_ref_cv_ptr_t, AbstractOperator>::value) {
        // nested (T::properties present)
        data.WithObject(key, JsonSerializer::to_json(val));
      } else {
        // non-nested (T::properties not present)
        with_any(data, key, *val);
      }
    }
    return;
  } else if constexpr (is_weak_ptr<T>::value) {
    with_any(data, key, val.lock().get());
  } else if constexpr (is_smart_ptr<T>::value) {
    StaticAssert<!is_unique_ptr<T>::value>::stat_assert(
        "Unique pointers are currently not supported by this json serializer");
    with_any(data, key, val.get());
  } else if constexpr (is_vector<T>::value) {
    data.WithObject(key, vec_to_json(val));
  } else if constexpr (std::is_enum<T>::value) {
    const std::string enum_name = magic_enum::enum_name(val).data();
    data.WithString(key, enum_name);
  } else {
    if constexpr (has_member_properties<T>::value) {
      // nested (T::properties present)
      data.WithObject(key, JsonSerializer::to_json(val));
    } else {
      with_any(data, key, val);
    }
  }
}

template <typename T>
jsonVal JsonSerializer::vec_to_json(const std::vector<T>& vec) {
  jsonVal jv;
  for (size_t idx = 0; idx < vec.size(); ++idx) {
    with_any<T>(jv, std::to_string(idx), vec.at(idx));
  }
  return jv;
}

// unserialize function
template <typename T>
T JsonSerializer::from_json(const jsonView& data) {
  typedef typename std::remove_cv_t<T> without_cv_t;

  if constexpr (is_smart_ptr<without_cv_t>::value) {
    StaticAssert<!is_unique_ptr<without_cv_t>::value>::stat_assert(
        "Unique pointers are currently not supported by this json serializer");
    StaticAssert<!is_weak_ptr<without_cv_t>::value>::stat_assert(
        "Weak pointers are currently not supported by this json serializer");
    std::cout << "is shared ptr" << std::endl;  // TODO(CAJan93): remove debug msg
    typedef T smart_ptr_t;                      // type of the smart pointer
    typedef get_inner_t<smart_ptr_t> inner_t;   // type of the object the pointer is pointing to
    inner_t* object = from_json<inner_t*>(data);
    smart_ptr_t sp = smart_ptr_t(object);
    return sp;
  } else {
    // pointer or object
    constexpr bool is_raw_ptr = std::is_pointer<without_cv_t>::value;
    typedef typename std::remove_pointer_t<without_cv_t> without_ptr_t;

    if constexpr (std::is_same<AbstractOperator, without_ptr_t>::value) {
      std::cout << "AbstractOperator" << std::endl;  // TODO(CAJan93) remove debug msg

      Assert(data.KeyExists("_type") || data.KeyExists("type"),
             JOIN_TO_STR("AbstractOperator needs type in order to be casted to concrete operator. Json was ",
                         data.WriteReadable()));

      // TODO(CAJan93): Do I really have to support _type and type? Not just _type?
      const std::string type_key = data.KeyExists("_type") ? "_type" : "type";
      const std::string enum_str = get_entry_from_json<std::string>(data, type_key);
      auto operator_type_opt = magic_enum::enum_cast<OperatorType>(enum_str);
      if (!operator_type_opt.has_value()) {
        Fail(JOIN_TO_STR("Unable to create OperatorType from string '", enum_str, "'"));
      }

      const OperatorType operator_type = operator_type_opt.value();
      if (operator_type == OperatorType::Aggregate) {
        if (data.KeyExists("_has_aggregate_functions")) {
          std::cout << "AggregateHash" << std::endl;  //  TODO(CAJan93): Remove this debug msg
          return from_json<AggregateHash*>(data);
        } else {
          std::cout << "AggregateSort" << std::endl;  //  TODO(CAJan93): Remove this debug msg
          return from_json<AggregateSort*>(data);
        }
      }
      std::cout << "OperatorType: " << magic_enum::enum_name(operator_type).data()
                << std::endl;  // TODO(CAJan93): remove debug msg
      switch (operator_type) {
        case OperatorType::Alias:
          return from_json<AliasOperator*>(data);
        case OperatorType::GetTable:
          return from_json<GetTable*>(data);
        case OperatorType::Limit:
          return from_json<Limit*>(data);
        case OperatorType::Projection:
          return from_json<Projection*>(data);
        case OperatorType::TableScan:
          return from_json<TableScan*>(data);
        case OperatorType::Validate:
          return from_json<Validate*>(data);

        default:
          Fail(JOIN_TO_STR("Unsupported OperatorType '", get_entry_from_json<std::string>(data, type_key), "'"));
      }

    } else if constexpr (std::is_same<AbstractExpression, without_ptr_t>::value) {
      auto expression_type_opt = magic_enum::enum_cast<ExpressionType>(get_entry_from_json<std::string>(data, "type"));
      if (!expression_type_opt.has_value()) {
        Fail(JOIN_TO_STR("Unable to create ExpressionType enum from string ",
                         get_entry_from_json<std::string>(data, "type")));
      }

      const ExpressionType expression_type = expression_type_opt.value();
      std::cout << "ExpressionType: " << magic_enum::enum_name(expression_type).data()
                << std::endl;  // TODO(CAJan93): remove debug msg

      switch (expression_type) {
        case ExpressionType::Aggregate:
          return from_json<AggregateExpression*>(data);
        case ExpressionType::Arithmetic:
          return from_json<ArithmeticExpression*>(data);
        case ExpressionType::Cast:
          return from_json<CastExpression*>(data);
        case ExpressionType::Case:
          return from_json<CaseExpression*>(data);
        case ExpressionType::CorrelatedParameter:
          return from_json<CorrelatedParameterExpression*>(data);
        case ExpressionType::PQPColumn:
          return from_json<PQPColumnExpression*>(data);
        case ExpressionType::LQPColumn:
          Fail("Serializer does not support LQPColumn expression");
        case ExpressionType::Exists:
          return from_json<ExistsExpression*>(data);
        case ExpressionType::Extract:
          return from_json<ExtractExpression*>(data);
        case ExpressionType::Function:
          return from_json<FunctionExpression*>(data);
        case ExpressionType::List:
          return from_json<ListExpression*>(data);
        case ExpressionType::Logical:
          return from_json<LogicalExpression*>(data);
        case ExpressionType::Placeholder:
          return from_json<PlaceholderExpression*>(data);
        case ExpressionType::Predicate: {
          const std::string predicate_type = get_entry_from_json<std::string>(data, "predicate_condition");
          const auto predicate_contition_opt = magic_enum::enum_cast<PredicateCondition>(predicate_type);
          // TODO(CAJan93): Write these statements as Asserts
          if (!predicate_contition_opt.has_value()) {
            Fail(JOIN_TO_STR("Unable to create enum of type PredicateCondition from string '", predicate_type, "'"));
          }

          const PredicateCondition pred_cond = predicate_contition_opt.value();
          switch (resolve_predicate_condition(pred_cond)) {
            case JsonSerializerParent::PredicateConditionExpression::Between:
              return from_json<BetweenExpression*>(data);

            case JsonSerializerParent::PredicateConditionExpression::BinaryPredicate:
              return from_json<BinaryPredicateExpression*>(data);

            case JsonSerializerParent::PredicateConditionExpression::In:
              return from_json<InExpression*>(data);

            case JsonSerializerParent::PredicateConditionExpression::IsNull:
              return from_json<IsNullExpression*>(data);

            default:
              Fail(JOIN_TO_STR("Unable to convert PredicateConditionExpression ",
                               magic_enum::enum_name<JsonSerializerParent::PredicateConditionExpression>(
                                   resolve_predicate_condition(pred_cond))
                                   .data()));
          }
          Fail("Unexpected path reached");
        }

        case ExpressionType::PQPSubquery:
          return from_json<PQPSubqueryExpression*>(data);
        case ExpressionType::LQPSubquery:
          Fail("Serializer does not support LQPSubquery expression");
        case ExpressionType::UnaryMinus:
          return from_json<UnaryMinusExpression*>(data);
        case ExpressionType::Value:
          return from_json<ValueExpression*>(data);
        default:
          Fail(JOIN_TO_STR("ExpressionType: ", magic_enum::enum_name(expression_type).data()));
      }
      Fail("not implemented");
    } else if constexpr (std::is_same<AggregateExpression, without_ptr_t>::value ||
                         std::is_same<IsNullExpression, without_ptr_t>::value) {
      constexpr auto property_0 = std::get<0>(without_ptr_t::properties);
      typedef typename decltype(property_0)::Type property_0_t;

      constexpr auto property_1 = std::get<1>(without_ptr_t::properties);
      typedef typename decltype(property_1)::Type property_1_t;

      if constexpr (is_raw_ptr) {
        without_cv_t t = new without_ptr_t(as_any<property_0_t>(data, property_0.name),
                                           as_any<property_1_t>(data, property_1.name).at(0));
        return t;
      } else {
        without_ptr_t t(as_any<property_0_t>(data, property_0.name), as_any<property_1_t>(data, property_1.name).at(0));
        return t;
      }
    } else if constexpr (std::is_same<ArithmeticExpression, without_ptr_t>::value ||
                         std::is_same<BinaryPredicateExpression, without_ptr_t>::value) {
      constexpr auto property_0 = std::get<0>(without_ptr_t::properties);
      typedef typename decltype(property_0)::Type property_0_t;

      constexpr auto property_1 = std::get<1>(without_ptr_t::properties);
      typedef typename decltype(property_1)::Type property_1_t;

      if constexpr (is_raw_ptr) {
        without_cv_t t = new without_ptr_t(as_any<property_0_t>(data, property_0.name),
                                           as_any<property_1_t>(data, property_1.name).at(0),
                                           as_any<property_1_t>(data, property_1.name).at(1));
        return t;
      } else {
        without_ptr_t t(as_any<property_0_t>(data, property_0.name), as_any<property_1_t>(data, property_1.name).at(0),
                        as_any<property_1_t>(data, property_1.name).at(1));
        return t;
      }
    } else if constexpr (std::is_same<BetweenExpression, without_ptr_t>::value) {
      constexpr auto property_0 = std::get<0>(without_ptr_t::properties);
      typedef typename decltype(property_0)::Type property_0_t;

      constexpr auto property_1 = std::get<1>(without_ptr_t::properties);
      typedef typename decltype(property_1)::Type property_1_t;
      if constexpr (is_raw_ptr) {
        without_cv_t t = new without_ptr_t(
            as_any<property_0_t>(data, property_0.name), as_any<property_1_t>(data, property_1.name).at(0),
            as_any<property_1_t>(data, property_1.name).at(1), as_any<property_1_t>(data, property_1.name).at(2));
        return t;
      } else {
        without_ptr_t t(as_any<property_0_t>(data, property_0.name), as_any<property_1_t>(data, property_1.name).at(0),
                        as_any<property_1_t>(data, property_1.name).at(1),
                        as_any<property_1_t>(data, property_1.name).at(2));
        return t;
      }
    } else if constexpr (std::is_same<PQPColumnExpression, without_ptr_t>::value) {
      constexpr auto property_0 = std::get<0>(without_ptr_t::properties);
      typedef typename decltype(property_0)::Type property_0_t;

      constexpr auto property_1 = std::get<1>(without_ptr_t::properties);
      typedef typename decltype(property_1)::Type property_1_t;

      constexpr auto property_2 = std::get<2>(without_ptr_t::properties);
      typedef typename decltype(property_2)::Type property_2_t;

      constexpr auto property_3 = std::get<3>(without_ptr_t::properties);
      typedef typename decltype(property_3)::Type property_3_t;

      if constexpr (is_raw_ptr) {
        without_cv_t t =
            new without_ptr_t(as_any<property_0_t>(data, property_0.name), as_any<property_1_t>(data, property_1.name),
                              as_any<property_2_t>(data, property_2.name), as_any<property_3_t>(data, property_3.name));
        return t;
      } else {
        without_ptr_t t(as_any<property_0_t>(data, property_0.name), as_any<property_1_t>(data, property_1.name),
                        as_any<property_2_t>(data, property_2.name), as_any<property_3_t>(data, property_3.name));
        return t;
      }
    } else if constexpr (std::is_same<ValueExpression, without_ptr_t>::value) {
      constexpr auto property_0 = std::get<0>(without_ptr_t::properties);
      typedef typename decltype(property_0)::Type property_0_t;

      if constexpr (is_raw_ptr) {
        without_cv_t t = new without_ptr_t(as_any<property_0_t>(data, property_0.name));
        return t;
      } else {
        without_ptr_t t(as_any<property_0_t>(data, property_0.name));
        return t;
      }
    } else if constexpr (std::is_same<GetTable, without_ptr_t>::value) {
      constexpr auto property_0 = std::get<0>(without_ptr_t::properties);
      typedef typename decltype(property_0)::Type property_0_t;

      constexpr auto property_1 = std::get<1>(without_ptr_t::properties);
      typedef typename decltype(property_1)::Type property_1_t;

      constexpr auto property_2 = std::get<2>(without_ptr_t::properties);
      typedef typename decltype(property_2)::Type property_2_t;

      if constexpr (is_raw_ptr) {
        without_cv_t t =
            new without_ptr_t(as_any<property_0_t>(data, property_0.name), as_any<property_1_t>(data, property_1.name),
                              as_any<property_2_t>(data, property_2.name));
        return t;
      } else {
        without_ptr_t t(as_any<property_0_t>(data, property_0.name), as_any<property_1_t>(data, property_1.name),
                        as_any<property_2_t>(data, property_2.name));
        return t;
      }
    }

    else if constexpr (std::is_same<AggregateHash, without_ptr_t>::value ||
                       std::is_same<AggregateSort, without_ptr_t>::value ||
                       std::is_same<AliasOperator, without_ptr_t>::value) {
      constexpr auto property_0 = std::get<0>(without_ptr_t::properties);
      // typedef typename decltype(property_0) property_0_t; // TODO(CAJan93): do not hardcode this
      typedef typename std::shared_ptr<AbstractOperator> property_0_t;

      constexpr auto property_1 = std::get<1>(without_ptr_t::properties);
      typedef typename decltype(property_1)::Type property_1_t;

      constexpr auto property_2 = std::get<2>(without_ptr_t::properties);
      typedef typename decltype(property_2)::Type property_2_t;

      if constexpr (is_raw_ptr) {
        without_cv_t t =
            new without_ptr_t(as_any<property_0_t>(data, property_0.name), as_any<property_1_t>(data, property_1.name),
                              as_any<property_2_t>(data, property_2.name));
        return t;
      } else {
        without_ptr_t t(as_any<property_0_t>(data, property_0.name), as_any<property_1_t>(data, property_1.name),
                        as_any<property_2_t>(data, property_2.name));
        return t;
      }
    } else if constexpr (std::is_same<Projection, without_ptr_t>::value ||
                         std::is_same<TableScan, without_ptr_t>::value) {
      constexpr auto property_0 = std::get<0>(without_ptr_t::properties);
      typedef typename decltype(property_0)::Type property_0_t;

      constexpr auto property_1 = std::get<1>(without_ptr_t::properties);
      typedef typename decltype(property_1)::Type property_1_t;
      if constexpr (is_raw_ptr) {
        without_cv_t t =
            new without_ptr_t(as_any<property_0_t>(data, property_0.name), as_any<property_1_t>(data, property_1.name));
        return t;
      } else {
        without_ptr_t t(as_any<property_0_t>(data, property_0.name), as_any<property_1_t>(data, property_1.name));
        return t;
      }
    } else if constexpr (std::is_same<Validate, without_ptr_t>::value) {
      constexpr auto property_0 = std::get<0>(without_ptr_t::properties);
      // typedef typename decltype(property_0) property_0_t; // TODO(CAJan93): do not hardcode this
      typedef typename std::shared_ptr<AbstractOperator> property_0_t;

      if constexpr (is_raw_ptr) {
        without_cv_t t = new without_ptr_t(as_any<property_0_t>(data, property_0.name));
        return t;
      } else {
        without_ptr_t t(as_any<property_0_t>(data, property_0.name));
        return t;
      }
    } else if constexpr (std::is_same<T, AllTypeVariant>::value) {
      return as_any<AllTypeVariant>(data, "value");
    } else {
      Fail(JOIN_TO_STR("Unsupported type", typeid(T).name(), " in json deserialization"));
    }
  }
}  // namespace opossum

template <typename T>
jsonVal JsonSerializer::to_json(const T& object) {
  jsonVal data;
  typedef typename std::remove_cv_t<T> without_cv;
  typedef typename std::remove_reference_t<without_cv> without_ref_cv_t;

  if constexpr (std::is_pointer<without_ref_cv_t>::value) {
    if constexpr (std::is_same<without_ref_cv_t, const AbstractOperator*>::value) {
      // cast Abstract operators
      auto abstract_op = (const AbstractOperator*)object;
      switch (abstract_op->type()) {
        case OperatorType::Alias: {
          const auto alias = dynamic_cast<const AliasOperator*>(abstract_op);
          std::cout << "AliasOperator" << std::endl;  //  TODO(CAJan93): Remove this debug msg
          return to_json<AliasOperator>(*alias);
        }

        case OperatorType::Aggregate: {
          const auto abstract_agg = dynamic_cast<const AbstractAggregateOperator*>(abstract_op);
          if (const auto agg_hash = dynamic_cast<const AggregateHash*>(abstract_agg); agg_hash) {
            std::cout << "Aggregate Hash" << std::endl;  //  TODO(CAJan93): Remove this debug msg
            return to_json<AggregateHash>(*agg_hash);
          } else if (const auto agg_sort = dynamic_cast<const AggregateSort*>(abstract_agg); agg_sort) {
            // TODO(CAJan93): Test this path
            std::cout << "Aggregate Sort" << std::endl;  //  TODO(CAJan93): Remove this debug msg
            return to_json<AggregateSort>(*agg_sort);
          }
          Fail("Unable to cast AbastractAggregator to concrete instance");
          return data;
        }

        case OperatorType::GetTable: {
          const auto gt = dynamic_cast<const GetTable*>(abstract_op);
          std::cout << "GetTable" << std::endl;  //  TODO(CAJan93): Remove this debug msg
          return to_json<GetTable>(*gt);
        }

        // TODO(CAJan93): Check if I am using the corret limit class
        case OperatorType::Limit: {
          const auto limit = dynamic_cast<const Limit*>(abstract_op);
          std::cout << "limit" << std::endl;  // TODO(CAJan93): Remove this debug msg
          return to_json<Limit>(*limit);
        }

        case OperatorType::Projection: {
          const auto projection = dynamic_cast<const Projection*>(abstract_op);
          std::cout << "projection" << std::endl;  // TODO(CAJan93): Remove this debug msg
          return to_json<Projection>(*projection);
        }

        case OperatorType::TableScan: {
          const auto table_scan = dynamic_cast<const TableScan*>(abstract_op);
          std::cout << "TableScan" << std::endl;  // TODO(CAJan93): Remove this debug msg
          return to_json<TableScan>(*table_scan);
        }

        case OperatorType::Validate: {
          const auto validate = dynamic_cast<const Validate*>(abstract_op);
          std::cout << "Validate" << std::endl;  // TODO(CAJan93): Remove this debug msg
          return to_json<Validate>(*validate);
          return data;
        }

        default: {
          // TODO(CAJan93) remove below code
          auto t = abstract_op->type();
          std::cout << "default OperatorType, with type" << newline_spacer << magic_enum::enum_name(t).data() << '\n';
        }
      }
      return data;

    } else if constexpr (std::is_same<without_ref_cv_t, AbstractExpression*>::value) {
      switch (object->type) {
          // TODO(CAJan93): Support the other ExpressionTypes

        case ExpressionType::Arithmetic: {
          const auto arithmetic_expr = dynamic_cast<ArithmeticExpression*>(object);
          std::cout << "Arithmetic expression" << std::endl;  // TODO(CAJan93): Remove this debug msg
          return to_json<ArithmeticExpression>(*arithmetic_expr);
        }

        case ExpressionType::PQPColumn: {
          const auto pqp_col = dynamic_cast<PQPColumnExpression*>(object);
          std::cout << "PQPColumn expression" << std::endl;  // TODO(CAJan93): Remove this debug msg
          return to_json<PQPColumnExpression>(*pqp_col);
        }

        case ExpressionType::Predicate: {
          const auto pred = dynamic_cast<AbstractPredicateExpression*>(object);
          std::cout << "abstract predicate" << std::endl;  // TODO(CAJan93): Remove debug msg
          switch (resolve_predicate_condition(pred->predicate_condition)) {
            case JsonSerializerParent::PredicateConditionExpression::Between: {
              const auto pred_between = dynamic_cast<BetweenExpression*>(object);
              return to_json<BetweenExpression>(*pred_between);
            }

            case JsonSerializerParent::PredicateConditionExpression::BinaryPredicate: {
              const auto pred_binary = dynamic_cast<BinaryPredicateExpression*>(object);
              return to_json<BinaryPredicateExpression>(*pred_binary);
            }

            case JsonSerializerParent::PredicateConditionExpression::In: {
              const auto pred_in = dynamic_cast<InExpression*>(object);
              return to_json<InExpression>(*pred_in);
            }

            case JsonSerializerParent::PredicateConditionExpression::IsNull: {
              const auto pred_null = dynamic_cast<IsNullExpression*>(object);
              return to_json<IsNullExpression>(*pred_null);
            }

            default:
              Fail(JOIN_TO_STR("Unable to convert PredicateConditionExpression ",
                               magic_enum::enum_name<JsonSerializerParent::PredicateConditionExpression>(
                                   resolve_predicate_condition(pred->predicate_condition))
                                   .data()));
          }
        }

        case ExpressionType::LQPColumn: {
          Fail("JsonSerializer does not support ExpressionType::LQPColumn");  // TODO(CAJan93): remove this?
          return data;
        }

        case ExpressionType::Value: {
          const auto val_expr = dynamic_cast<ValueExpression*>(object);
          std::cout << "Value expression\n";  // TODO(CAJan93): remove debug msg
          return to_json<ValueExpression>(*val_expr);
        }

        default:
          // TODO(CAJan93): Handle the other ExpressionTypes
          std::cout << "Failure. Unsupported ExpressionType " << magic_enum::enum_name(object->type).data() << '\n';
          return data;
          break;
      }

    } else if constexpr (std::is_same<without_ref_cv_t, const AbstractLQPNode*>::value) {
      Fail("JsonSerializer does not support ExpressionType::AbstractLQPNode");  // TODO(CAJan93): remove this?
      return data;
    }

    else {
      return to_json<std::remove_pointer_t<without_ref_cv_t>>(*object);
    }
  } else if constexpr (is_weak_ptr<without_ref_cv_t>::value) {
    return to_json(object.lock().get());
  } else if constexpr (is_smart_ptr<without_ref_cv_t>::value) {
    StaticAssert<!is_unique_ptr<without_ref_cv_t>::value>::stat_assert(
        "Unique pointers are currently not supported by this json serializer");
    return to_json(object.get());  // keep const qualifier, since get() might return a const pointer
  } else if constexpr (has_member_properties<without_ref_cv_t>::value) {
    // serialize a class that provides properties tuple
    constexpr auto nb_properties = std::tuple_size<decltype(without_ref_cv_t::properties)>::value;
    for_sequence(std::make_index_sequence<nb_properties>{}, [&](auto i) {
      constexpr auto property = std::get<i>(without_ref_cv_t::properties);
      with_any(data, property.name, object.*(property.member));
    });
    return data;
  } else if constexpr (has_member__type<without_ref_cv_t>::value) {
    // TODO(CAJan93): remove this case? If so, remove has_member__type?
    std::cout << "type is " << magic_enum::enum_name(object.type()).data() << " is currently not supported.\n";
    return data;
  } else {
    Fail(JOIN_TO_STR("unsupported type ", typeid(object).name(), newline_spacer, "typeid T: ", typeid(T).name(),
                     newline_spacer, "typeid without_ref_cv_t: ", typeid(without_ref_cv_t).name()));
  }
  return data;
}  // namespace opossum

template <typename T>
std::string JsonSerializer::to_json_str(const T& object) {
  return to_json(object).View().WriteReadable();
}

template <typename T>
auto JsonSerializer::from_json_str(const std::string& json_str) {
  const jsonVal data(json_str);
  const jsonView jv = data.View();
  return from_json<T>(jv);
}
}  // namespace opossum