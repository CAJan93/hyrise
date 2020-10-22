#pragma once

#include "../operators/property.hpp"
#include "abstract_expression.hpp"
#include "all_type_variant.hpp"

namespace opossum {

/**
 * Wraps an AllTypeVariant
 */
class ValueExpression : public AbstractExpression {
 public:
  explicit ValueExpression(const AllTypeVariant& init_value);

  bool requires_computation() const override;
  std::shared_ptr<AbstractExpression> deep_copy() const override;
  std::string description(const DescriptionMode mode) const override;
  DataType data_type() const override;

  const AllTypeVariant value;

 protected:
  bool _shallow_equals(const AbstractExpression& expression) const override;
  size_t _shallow_hash() const override;
  bool _on_is_nullable_on_lqp(const AbstractLQPNode& lqp) const override;

 public:
  inline constexpr static auto json_serializer_properties = std::make_tuple(
      // TODO(CAJan93): support value
      property(&ValueExpression::value, "value"),
      // From AbstractExpression
      property(&ValueExpression::arguments, "arguments"), property(&ValueExpression::type, "type"));
};

}  // namespace opossum
