#include "cost_model_coefficient_reader.hpp"

namespace opossum {

// Hard-coded efficients for now
const TableScanCoefficientsPerGroup CostModelCoefficientReader::read_table_scan_coefficients(
    const std::string& file_path) {
  return {{TableScanModelGroup{OperatorType::TableScan, DataType::Int, false, false},
           {{"left_input_row_count", 3.6579310596},
            //                          {"is_result_empty", -3894.4296670012},
            {"selectivity", 11285.9587666981},
            {"first_column_is_reference_segment", 0},
            {"second_column_is_reference_segment", 0},
            {"third_column_is_reference_segment", 0},
            {"is_column_comparison", -1721.1127107658},
            {"computable_or_column_expression_count", 14227.0873454307},
            {"is_selectivity_below_50_percent", 1874.7617189885},
            {"selectivity_distance_to_50_percent", -17721.3571372186},
            //                          {"is_small_table", 0},
            {"first_column_segment_encoding_Unencoded_percentage", 875.4659477475},
            {"first_column_segment_encoding_Dictionary_percentage", -2466.8790265752},
            {"first_column_segment_encoding_RunLength_percentage", 807.0515189504},
            {"first_column_segment_encoding_FixedStringDictionary_percentage", 0},
            {"first_column_segment_encoding_FrameOfReference_percentage", 2365.7972807862},
            //                          {"first_column_segment_encoding_undefined", 0},
            //                          {"first_column_data_type_null", 1581.4357209088},
            {"first_column_data_type_int", 1581.4357209088},
            {"first_column_data_type_long", 1581.4357209088},
            {"first_column_data_type_float", 1581.4357209088},
            {"first_column_data_type_double", 1581.4357209088},
            {"first_column_data_type_string", 1581.4357209088},
            {"first_column_data_type_undefined", 1581.4357209088},
            {"second_column_segment_encoding_Unencoded_percentage", -156.771035114},
            {"second_column_segment_encoding_Dictionary_percentage", -504.0943341712},
            {"second_column_segment_encoding_RunLength_percentage", -691.4961272456},
            {"second_column_segment_encoding_FixedStringDictionary_percentage", 0},
            {"second_column_segment_encoding_FrameOfReference_percentage", -368.7512142351},
            //                          {"second_column_segment_encoding_undefined", 3302.5484316746},
            //                          {"second_column_data_type_null", -1721.1127107658},
            {"second_column_data_type_int", -1721.1127107658},
            {"second_column_data_type_long", -1721.1127107658},
            {"second_column_data_type_float", -1721.1127107658},
            {"second_column_data_type_double", -1721.1127107658},
            {"second_column_data_type_string", -1721.1127107658},
            {"second_column_data_type_undefined", 3302.5484316746},
            {"third_column_segment_encoding_Unencoded_percentage", 4453.3879124653},
            {"third_column_segment_encoding_Dictionary_percentage", 2251.3754879083},
            {"third_column_segment_encoding_RunLength_percentage", 3768.4614356163},
            {"third_column_segment_encoding_FixedStringDictionary_percentage", 0},
            {"third_column_segment_encoding_FrameOfReference_percentage", 2312.1037783889},
            //                          {"third_column_segment_encoding_undefined", -11203.8928934701},
            //                          {"third_column_data_type_null", 12785.3286143789},
            {"third_column_data_type_int", 12785.3286143789},
            {"third_column_data_type_long", 12785.3286143789},
            {"third_column_data_type_float", 12785.3286143789},
            {"third_column_data_type_double", 12785.3286143789},
            {"third_column_data_type_string", 12785.3286143789},
            {"third_column_data_type_undefined", -11203.892893470}}}};
}

const JoinCoefficientsPerGroup CostModelCoefficientReader::read_join_coefficients(const std::string& file_path) {
  return {{JoinModelGroup{OperatorType::JoinHash},
           {{"input_table_size_ratio", 71.95205481472149},
            {"left_column_memory_usage_bytes", -0.10347867049632953},
            {"left_column_segment_encoding_Dictionary_percentage", 3301.7229066765462},
            {"left_column_segment_encoding_RunLength_percentage", 4961.508429879236},
            {"left_column_segment_encoding_Unencoded_percentage", 89.9545618869306},
            {"left_input_row_count", -11.58212416892127},
            {"right_column_memory_usage_bytes", 3.8911290381568087},
            {"right_column_segment_encoding_Dictionary_percentage", 3301.722906677826},
            {"right_column_segment_encoding_RunLength_percentage", 4961.5084298796755},
            {"right_column_segment_encoding_Unencoded_percentage", 89.95456188722937},
            {"right_input_row_count", 33.33254579163385},
            {"total_row_count", 0.000007342975518461171},
            {"logical_cost_sort_merge", 1.4094741686421433},
            {"logical_cost_hash", 21.750421924907805},
            {"left_column_data_type_int", 8353.185898444724},
            {"operator_type_JoinHash", 8353.185898444724},
            {"operator_type_JoinIndex", 0.0},
            {"operator_type_JoinNestedLoop", 0.0},
            {"operator_type_JoinMPSM", 0.0},
            {"operator_type_JoinSortMerge", 0.0},
            {"right_column_data_type_int", 8353.185898444715}}},
          {JoinModelGroup{OperatorType::JoinNestedLoop},
           {{"input_table_size_ratio", 3.1087845468836157},
            {"left_column_memory_usage_bytes", -1.9993722710901425},
            {"left_column_segment_encoding_Dictionary_percentage", 1229.9267257888494},
            {"left_column_segment_encoding_RunLength_percentage", 5238.365600938088},
            {"left_column_segment_encoding_Unencoded_percentage", 610.8891734392473},
            {"left_input_row_count", 35.36160147065177},
            {"right_column_memory_usage_bytes", 0.4689625993589201},
            {"right_column_segment_encoding_Dictionary_percentage", 1229.9267257883118},
            {"right_column_segment_encoding_RunLength_percentage", 5238.365600938055},
            {"right_column_segment_encoding_Unencoded_percentage", 610.8891734392273},
            {"right_input_row_count", -19.283001324800075},
            {"total_row_count", 1.2436455771499408},
            {"logical_cost_sort_merge", -2.3305021499789733},
            {"logical_cost_hash", 16.07860014612547},
            {"left_column_data_type_int", 7079.1815001656},
            {"operator_type_JoinHash", 0.0},
            {"operator_type_JoinIndex", 0.0},
            {"operator_type_JoinNestedLoop", 7079.1815001656},
            {"operator_type_JoinMPSM", 0.0},
            {"operator_type_JoinSortMerge", 0.0},
            {"right_column_data_type_int", 7079.1815001656}}},
          {JoinModelGroup{OperatorType::JoinMPSM},
           {{"input_table_size_ratio", 16.990260589965715},
            {"left_column_memory_usage_bytes", 1.5567396867451246},
            {"left_column_segment_encoding_Dictionary_percentage", 5059.468038081371},
            {"left_column_segment_encoding_RunLength_percentage", 6698.124288200128},
            {"left_column_segment_encoding_Unencoded_percentage", 3021.6817674160966},
            {"left_input_row_count", 10.523081424486762},
            {"right_column_memory_usage_bytes", 1.7901891109816328},
            {"right_column_segment_encoding_Dictionary_percentage", 5059.468038076947},
            {"right_column_segment_encoding_RunLength_percentage", 6698.124288203066},
            {"right_column_segment_encoding_Unencoded_percentage", 3021.681767416301},
            {"right_input_row_count", 7.179322655900813},
            {"total_row_count", 0.0000063150899070942915},
            {"logical_cost_sort_merge", 2.7026700861586317},
            {"logical_cost_hash", 17.702402827911584},
            {"left_column_data_type_int", 14779.274093696327},
            {"operator_type_JoinHash", 0.0},
            {"operator_type_JoinIndex", 0.0},
            {"operator_type_JoinNestedLoop", 0.0},
            {"operator_type_JoinMPSM", 14779.274093696327},
            {"operator_type_JoinSortMerge", 0.0},
            {"right_column_data_type_int", 14779.274093696322}}},
          {JoinModelGroup{OperatorType::JoinSortMerge},
           {{"input_table_size_ratio", -10.301034786284603},
            {"left_column_memory_usage_bytes", 1.4703527725260641},
            {"left_column_segment_encoding_Dictionary_percentage", 3017.052299178862},
            {"left_column_segment_encoding_RunLength_percentage", 4684.622533375804},
            {"left_column_segment_encoding_Unencoded_percentage", 398.2092169496141},
            {"left_input_row_count", 2.3442492971944446},
            {"right_column_memory_usage_bytes", 2.010167887091694},
            {"right_column_segment_encoding_Dictionary_percentage", 3017.052299176387},
            {"right_column_segment_encoding_RunLength_percentage", 4684.622533372891},
            {"right_column_segment_encoding_Unencoded_percentage", 398.2092169527132},
            {"right_input_row_count", 6.638474326854725},
            {"total_row_count", 0.000007254503033964427},
            {"logical_cost_sort_merge", 3.1036742771901866},
            {"logical_cost_hash", 8.982723684881023},
            {"left_column_data_type_int", 8099.884049502247},
            {"operator_type_JoinHash", 0.0},
            {"operator_type_JoinIndex", 0.0},
            {"operator_type_JoinNestedLoop", 0.0},
            {"operator_type_JoinMPSM", 0.0},
            {"operator_type_JoinSortMerge", 8099.884049502247},
            {"right_column_data_type_int", 8099.8840495022505}}}};
}

}  // namespace opossum