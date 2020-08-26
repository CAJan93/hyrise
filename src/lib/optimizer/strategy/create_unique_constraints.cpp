#include "create_unique_constraints.hpp"

#include "logical_query_plan/abstract_lqp_node.hpp"
#include "logical_query_plan/abstract_non_query_node.hpp"

namespace opossum {

void CreateUniqueConstraintsRule::apply_to(const std::shared_ptr<AbstractLQPNode>& lqp) const {

  for(size_t i = 0; i < 100; ++i) {
    Assert(!lqp->right_input(), "Did not expect right input node");
    if(!dynamic_pointer_cast<AbstractNonQueryNode>(lqp->left_input())) {
      const auto& unique_constraints = lqp->left_input()->unique_constraints();
      Assert(unique_constraints->size() > 0 || unique_constraints->size() == 0, "Dummy usage");
    }
  }

}

}  // namespace opossum
