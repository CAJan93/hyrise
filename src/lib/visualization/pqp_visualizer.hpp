#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <chrono> // only needed for benchmarking json (de)serilizer

#include "expression/abstract_expression.hpp"
#include "operators/abstract_operator.hpp"
#include "visualization/abstract_visualizer.hpp"

namespace opossum {

class PQPVisualizer : public AbstractVisualizer<std::vector<std::shared_ptr<AbstractOperator>>> {
 public:
  PQPVisualizer();

  PQPVisualizer(GraphvizConfig graphviz_config, VizGraphInfo graph_info = {}, VizVertexInfo vertex_info = {},
                VizEdgeInfo edge_info = {});

  void build_json(const std::vector<std::shared_ptr<AbstractOperator>>& plans);

 protected:
  void _build_graph(const std::vector<std::shared_ptr<AbstractOperator>>& plans) override;

  void _build_subtree(const std::shared_ptr<const AbstractOperator>& op,
                      std::unordered_set<std::shared_ptr<const AbstractOperator>>& visualized_ops);

  void _build_subjson(const std::shared_ptr<const AbstractOperator>& op);

  void _visualize_subqueries(const std::shared_ptr<const AbstractOperator>& op,
                             const std::shared_ptr<AbstractExpression>& expression,
                             std::unordered_set<std::shared_ptr<const AbstractOperator>>& visualized_ops);

  void _visualize_subqueries_json(const std::shared_ptr<const AbstractOperator>& op,
                             const std::shared_ptr<AbstractExpression>& expression);

  void _build_dataflow(const std::shared_ptr<const AbstractOperator>& from,
                       const std::shared_ptr<const AbstractOperator>& to, const InputSide side);

  void _add_operator(const std::shared_ptr<const AbstractOperator>& op);

  std::unordered_map<std::string, std::chrono::nanoseconds> _duration_by_operator_name;
};

}  // namespace opossum
