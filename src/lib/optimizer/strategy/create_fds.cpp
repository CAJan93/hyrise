#include "create_fds.hpp"

#include "logical_query_plan/abstract_lqp_node.hpp"
#include "logical_query_plan/abstract_non_query_node.hpp"

namespace opossum {

void CreateFDsRule::apply_to(const std::shared_ptr<AbstractLQPNode>& lqp) const {
    Assert(!lqp->right_input(), "Did not expect right input node");
    if(!dynamic_pointer_cast<AbstractNonQueryNode>(lqp->left_input())) {
      const auto& fds = lqp->left_input()->functional_dependencies();
      Assert(fds.size() > 0 || fds.size() == 0, "Dummy usage");
    }
}

}  // namespace opossum
