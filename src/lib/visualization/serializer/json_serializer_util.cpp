#include "json_serializer_util.hpp"

#include <iostream>

#include "../../types.hpp"
#include "../../utils/assert.hpp"

namespace opossum {
JsonSerializerUtil::PredicateConditionExpression JsonSerializerUtil::resolve_predicate_condition(
    const PredicateCondition pred_cond) {
  switch (pred_cond) {
    case PredicateCondition::BetweenExclusive:
    case PredicateCondition::BetweenInclusive:
    case PredicateCondition::BetweenLowerExclusive:
    case PredicateCondition::BetweenUpperExclusive:
      std::cout << "between expression" << std::endl;  // TODO(CAJan93): Remove debug msg
      return PredicateConditionExpression::Between;

      // TODO(CAJan93): Is this correct? Does the binary pred. expr. cover all these cases?
    case PredicateCondition::Equals:
    case PredicateCondition::GreaterThan:
    case PredicateCondition::GreaterThanEquals:
    case PredicateCondition::LessThan:
    case PredicateCondition::LessThanEquals:
    case PredicateCondition::Like:
    case PredicateCondition::NotEquals:
    case PredicateCondition::NotLike:
      std::cout << "binary predicate expression" << std::endl;  // TODO(CAJan93): Remove debug msg
      return PredicateConditionExpression::BinaryPredicate;

    case PredicateCondition::In:
    case PredicateCondition::NotIn:
      std::cout << "in expression" << std::endl;  // TODO(CAJan93): Remove debug msg
      return PredicateConditionExpression::In;

    case PredicateCondition::IsNotNull:
    case PredicateCondition::IsNull:
      std::cout << "is null expression" << std::endl;  // TODO(CAJan93): Remove debug msg
      return PredicateConditionExpression::IsNull;

    default:
      Fail(JOIN_TO_STR("Unable to convert PredicateCondition ",
                       magic_enum::enum_name<PredicateCondition>(pred_cond).data()));
  }
}
}  // namespace opossum