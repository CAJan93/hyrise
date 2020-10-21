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
#include "json_serializer_util.hpp"
#include "string.hpp"
#include "static_assert.hpp"

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
class JsonSerializerInterface;



class JsonSerializer : JsonSerializerUtil {
  // convenience alias for inhereted member enum
  using PredCondExpr = JsonSerializerUtil::PredicateConditionExpression;

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

  // set data in json
  template <typename T>
  static void with_any(jsonVal& data, const std::string& key, const T& val);

  // serialize
  template <typename T>
  static jsonVal to_json(const T& object);

  friend JsonSerializerInterface;
};

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
        data.WithObject(key, to_json(val));
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
      data.WithObject(key, to_json(val));
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

template <typename T>
jsonVal JsonSerializer::to_json(const T& object) {
  jsonVal data;
  typedef typename std::remove_cv_t<T> without_cv;
  typedef typename std::remove_reference_t<without_cv> without_ref_cv_t;

  if constexpr (std::is_pointer<without_ref_cv_t>::value) {
    // add const to pointer to handle both const and non-const raw pointers
    if constexpr (std::is_same<without_ref_cv_t, AbstractOperator*>::value) {
      return to_json<const AbstractOperator*>(object);
    }

    if constexpr (std::is_same<without_ref_cv_t, const AbstractOperator*>::value) {
      // cast Abstract operators
      auto abstract_op = (const AbstractOperator*)object;
      switch (abstract_op->type()) {
        case OperatorType::Aggregate: {
          auto abstract_agg = dynamic_cast<const AbstractAggregateOperator*>(abstract_op);
          if (auto agg_hash = dynamic_cast<const AggregateHash*>(abstract_agg); agg_hash) {
            std::cout << "[json serializer] "
                      << "Aggregate Hash" << std::endl;  //  TODO(CAJan93): Remove this debug msg
            return to_json<AggregateHash>(*agg_hash);
          } else if (auto agg_sort = dynamic_cast<const AggregateSort*>(abstract_agg); agg_sort) {
            // TODO(CAJan93): Test this path
            std::cout << "[json serializer] "
                      << "Aggregate Sort" << std::endl;  //  TODO(CAJan93): Remove this debug msg
            return to_json<AggregateSort>(*agg_sort);
          }
          Fail("Unable to cast AbastractAggregator to concrete instance");
          return data;
        }

        case OperatorType::Alias: {
          auto alias = dynamic_cast<const AliasOperator*>(abstract_op);
          std::cout << "[json serializer] "
                    << "AliasOperator" << std::endl;  //  TODO(CAJan93): Remove this debug msg
          return to_json<AliasOperator>(*alias);
        }

        case OperatorType::GetTable: {
          auto gt = dynamic_cast<const GetTable*>(abstract_op);
          std::cout << "[json serializer] "
                    << "GetTable" << std::endl;  //  TODO(CAJan93): Remove this debug msg
          return to_json<GetTable>(*gt);
        }

        // TODO(CAJan93): Check if I am using the corret limit class
        case OperatorType::Limit: {
          auto limit = dynamic_cast<const Limit*>(abstract_op);
          std::cout << "[json serializer] "
                    << "limit" << std::endl;  // TODO(CAJan93): Remove this debug msg
          return to_json<Limit>(*limit);
        }

        case OperatorType::Projection: {
          auto projection = dynamic_cast<const Projection*>(abstract_op);
          std::cout << "[json serializer] "
                    << "projection" << std::endl;  // TODO(CAJan93): Remove this debug msg
          return to_json<Projection>(*projection);
        }

        case OperatorType::TableScan: {
          auto table_scan = dynamic_cast<const TableScan*>(abstract_op);
          std::cout << "[json serializer] "
                    << "TableScan" << std::endl;  // TODO(CAJan93): Remove this debug msg
          return to_json<TableScan>(*table_scan);
        }

        case OperatorType::Validate: {
          auto validate = dynamic_cast<const Validate*>(abstract_op);
          std::cout << "[json serializer] "
                    << "Validate" << std::endl;  // TODO(CAJan93): Remove this debug msg
          return to_json<Validate>(*validate);
          return data;
        }

        default: {
          // TODO(CAJan93) remove below code
          auto t = abstract_op->type();
          std::cout << "[json serializer] "
                    << "default OperatorType, with type" << newline_spacer << magic_enum::enum_name(t).data() << '\n';
        }
      }
      return data;

    } else if constexpr (std::is_same<without_ref_cv_t, AbstractExpression*>::value) {
      switch (object->type) {
          // TODO(CAJan93): Support the other ExpressionTypes

        case ExpressionType::Arithmetic: {
          const auto arithmetic_expr = dynamic_cast<ArithmeticExpression*>(object);
          std::cout << "[json serializer] "
                    << "Arithmetic expression" << std::endl;  // TODO(CAJan93): Remove this debug msg
          return to_json<ArithmeticExpression>(*arithmetic_expr);
        }

        case ExpressionType::PQPColumn: {
          const auto pqp_col = dynamic_cast<PQPColumnExpression*>(object);
          std::cout << "[json serializer] "
                    << "PQPColumn expression" << std::endl;  // TODO(CAJan93): Remove this debug msg
          return to_json<PQPColumnExpression>(*pqp_col);
        }

        case ExpressionType::Predicate: {
          const auto pred = dynamic_cast<AbstractPredicateExpression*>(object);
          std::cout << "[json serializer] "
                    << "abstract predicate" << std::endl;  // TODO(CAJan93): Remove debug msg
          switch (resolve_predicate_condition(pred->predicate_condition)) {
            case PredCondExpr::Between: {
              const auto pred_between = dynamic_cast<BetweenExpression*>(object);
              return to_json<BetweenExpression>(*pred_between);
            }

            case PredCondExpr::BinaryPredicate: {
              const auto pred_binary = dynamic_cast<BinaryPredicateExpression*>(object);
              return to_json<BinaryPredicateExpression>(*pred_binary);
            }

            case PredCondExpr::In: {
              const auto pred_in = dynamic_cast<InExpression*>(object);
              return to_json<InExpression>(*pred_in);
            }

            case PredCondExpr::IsNull: {
              const auto pred_null = dynamic_cast<IsNullExpression*>(object);
              return to_json<IsNullExpression>(*pred_null);
            }

            default:
              Fail(JOIN_TO_STR("Unable to convert PredicateConditionExpression ",
                               magic_enum::enum_name<JsonSerializerUtil::PredicateConditionExpression>(
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
          std::cout << "[json serializer] "
                    << "Value expression\n";  // TODO(CAJan93): remove debug msg
          return to_json<ValueExpression>(*val_expr);
        }

        default:
          // TODO(CAJan93): Handle the other ExpressionTypes
          std::cout << "[json serializer] "
                    << "Failure. Unsupported ExpressionType " << magic_enum::enum_name(object->type).data() << '\n';
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
    std::cout << "[json serializer] "
              << "type is " << magic_enum::enum_name(object.type()).data() << " is currently not supported.\n";
    return data;
  } else {
    Fail(JOIN_TO_STR("unsupported type ", typeid(object).name(), newline_spacer, "typeid T: ", typeid(T).name(),
                     newline_spacer, "typeid without_ref_cv_t: ", typeid(without_ref_cv_t).name()));
  }
  return data;
}  // namespace opossum
}  // namespace opossum
