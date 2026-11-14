#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <type_traits>
#include <variant>
#include <iomanip>
#include "definitions.h"

// Flatten strings into a single char array and store offsets
void flattenStrings(const std::vector<std::string>& hostStrings, std::vector<char>& h_flattenedStrings, std::vector<int>& h_offsets);

// CSV Reader functions
Date parse_date(const std::string& s);

DataType parse_cell(const std::string& s);

Table read_csv(const std::string& filename);


// Helper function to print a table with fixed-width fields
void print_table(const Table& table);

// Helper template for the static_assert
template<class> inline constexpr bool always_false_v = false;

// Helper function to convert DataType to string
std::string data_type_to_string(const DataType& value);

void save_table(const Table& table, const std::string& filename);

// Helper to trim from both ends (in place)
static inline void trim_string_inplace(std::string &s);

AliasColumnPair parseAliasColumnString(std::string part_str);

CompareOp flipComparisonOperator(CompareOp op);

std::string comparisonOperatorToString(CompareOp op);

// Helper function to extract a base name from a filepath to use as an alias
std::string extractAliasFromFilename(const std::string& filepath);