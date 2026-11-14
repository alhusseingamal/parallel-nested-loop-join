#pragma once
#include "db_utils.h"
Table nested_loop_join_gpu(const Table& table_R, const Table& table_S, const std::vector<JoinCondition>& conditions);

// Table sort_merge_join_gpu(const Table& table_R, const Table& table_S, const std::vector<JoinCondition>& conditions);