#include "index_scan_loop.hpp"

#include <algorithm>

#include "expression/between_expression.hpp"

#include "scheduler/abstract_task.hpp"
#include "scheduler/current_scheduler.hpp"
#include "scheduler/job_task.hpp"

#include "storage/index/base_index.hpp"
#include "storage/reference_segment.hpp"

#include "utils/assert.hpp"

namespace opossum {

IndexScanLoop::IndexScanLoop(const std::shared_ptr<const AbstractOperator>& in, const SegmentIndexType index_type,
                     const std::vector<ColumnID>& left_column_ids, const PredicateCondition predicate_condition,
                     const std::vector<AllTypeVariant>& right_values, const std::vector<AllTypeVariant>& right_values2,
                     const std::shared_ptr<const AbstractLQPNode>& lqp_node)
    : AbstractReadOnlyOperator{OperatorType::IndexScan, in, nullptr, lqp_node},
      _index_type{index_type},
      _left_column_ids{left_column_ids},
      _predicate_condition{predicate_condition},
      _right_values{right_values},
      _right_values2{right_values2} {}

const std::string IndexScanLoop::name() const { return "IndexScanLoop"; }

void IndexScanLoop::set_included_chunk_ids(const std::vector<ChunkID>& chunk_ids) { _included_chunk_ids = chunk_ids; }

std::shared_ptr<const Table> IndexScanLoop::_on_execute() {
  _in_table = input_table_left();

  _validate_input();

  _out_table = std::make_shared<Table>(_in_table->column_definitions(), TableType::References);

  std::mutex output_mutex;

  auto jobs = std::vector<std::shared_ptr<AbstractTask>>{};
  if (_included_chunk_ids.empty()) {
    jobs.reserve(_in_table->chunk_count());
    for (auto chunk_id = ChunkID{0u}; chunk_id < _in_table->chunk_count(); ++chunk_id) {
      jobs.push_back(_create_job_and_schedule(chunk_id, output_mutex));
    }
  } else {
    jobs.reserve(_included_chunk_ids.size());
    for (auto chunk_id : _included_chunk_ids) {
      jobs.push_back(_create_job_and_schedule(chunk_id, output_mutex));
    }
  }

  CurrentScheduler::wait_for_tasks(jobs);

  return _out_table;
}

std::shared_ptr<AbstractOperator> IndexScanLoop::_on_deep_copy(
    const std::shared_ptr<AbstractOperator>& copied_input_left,
    const std::shared_ptr<AbstractOperator>& copied_input_right) const {
  return std::make_shared<IndexScanLoop>(copied_input_left, _index_type, _left_column_ids, _predicate_condition,
                                     _right_values, _right_values2);
}

void IndexScanLoop::_on_set_parameters(const std::unordered_map<ParameterID, AllTypeVariant>& parameters) {}

std::shared_ptr<AbstractTask> IndexScanLoop::_create_job_and_schedule(const ChunkID chunk_id, std::mutex& output_mutex) {
  auto job_task = std::make_shared<JobTask>([=, &output_mutex]() {
    const auto matches_out = std::make_shared<PosList>(_scan_chunk(chunk_id));

    // The output chunk is allocated on the same NUMA node as the input chunk.
    const auto chunk = _in_table->get_chunk(chunk_id);
    Segments segments;

    for (ColumnID column_id{0u}; column_id < _in_table->column_count(); ++column_id) {
      auto ref_segment_out = std::make_shared<ReferenceSegment>(_in_table, column_id, matches_out);
      segments.push_back(ref_segment_out);
    }

    std::lock_guard<std::mutex> lock(output_mutex);
    _out_table->append_chunk(segments, nullptr, chunk->get_allocator());
  });

  job_task->schedule();
  return job_task;
}

void IndexScanLoop::_validate_input() {
  Assert(_predicate_condition != PredicateCondition::Like, "Predicate condition not supported by index scan.");
  Assert(_predicate_condition != PredicateCondition::NotLike, "Predicate condition not supported by index scan.");

  Assert(_left_column_ids.size() == _right_values.size(),
         "Count mismatch: left column IDs and right values don’t have same size.");
  if (is_between_predicate_condition(_predicate_condition)) {
    Assert(_left_column_ids.size() == _right_values2.size(),
           "Count mismatch: left column IDs and right values don’t have same size.");
  }

  Assert(_in_table->type() == TableType::Data, "IndexScanLoop only supports persistent tables right now.");
}

PosList IndexScanLoop::_scan_chunk(const ChunkID chunk_id) {
  const auto to_row_id = [chunk_id](ChunkOffset chunk_offset) { return RowID{chunk_id, chunk_offset}; };

  auto range_begin = BaseIndex::Iterator{};
  auto range_end = BaseIndex::Iterator{};

  const auto chunk = _in_table->get_chunk(chunk_id);
  auto matches_out = PosList{};

  const auto index = chunk->get_index(_index_type, _left_column_ids);
  Assert(index, "Index of specified type not found for segment (vector).");

  switch (_predicate_condition) {
    case PredicateCondition::Equals: {
      range_begin = index->lower_bound(_right_values);
      range_end = index->upper_bound(_right_values);
      break;
    }
    case PredicateCondition::NotEquals: {
      // first, get all values less than the search value
      range_begin = index->cbegin();
      range_end = index->lower_bound(_right_values);

      matches_out.reserve(std::distance(range_begin, range_end));
      std::transform(range_begin, range_end, std::back_inserter(matches_out), to_row_id);

      // set range for second half to all values greater than the search value
      range_begin = index->upper_bound(_right_values);
      range_end = index->cend();
      break;
    }
    case PredicateCondition::LessThan: {
      range_begin = index->cbegin();
      range_end = index->lower_bound(_right_values);
      break;
    }
    case PredicateCondition::LessThanEquals: {
      range_begin = index->cbegin();
      range_end = index->upper_bound(_right_values);
      break;
    }
    case PredicateCondition::GreaterThan: {
      range_begin = index->upper_bound(_right_values);
      range_end = index->cend();
      break;
    }
    case PredicateCondition::GreaterThanEquals: {
      range_begin = index->lower_bound(_right_values);
      range_end = index->cend();
      break;
    }
    case PredicateCondition::BetweenInclusive: {
      range_begin = index->lower_bound(_right_values);
      range_end = index->upper_bound(_right_values2);
      break;
    }
    case PredicateCondition::BetweenLowerExclusive: {
      range_begin = index->upper_bound(_right_values);
      range_end = index->upper_bound(_right_values2);
      break;
    }
    case PredicateCondition::BetweenUpperExclusive: {
      range_begin = index->lower_bound(_right_values);
      range_end = index->lower_bound(_right_values2);
      break;
    }
    case PredicateCondition::BetweenExclusive: {
      range_begin = index->upper_bound(_right_values);
      range_end = index->lower_bound(_right_values2);
      break;
    }
    default:
      Fail("Unsupported comparison type encountered");
  }
  const auto d = static_cast<size_t>(std::distance(range_begin, range_end));
  matches_out.resize(d);
  for (size_t i = 0; i < d; ++i) {
    matches_out[i] = RowID{chunk_id, *range_begin};
    range_begin++;
  }

  // std::sort(matches_out.begin(), matches_out.end(), [](const auto& left, const auto& right) { return left.chunk_offset < right.chunk_offset; });
  return matches_out;
}

}  // namespace opossum
