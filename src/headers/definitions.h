#include <string>
#include <vector>
#include <algorithm>
#include <type_traits>
#include <variant>

struct Date {
    int day, month, year;
    
    bool operator==(const Date& other) const { return day == other.day && month == other.month && year == other.year; }
    
    bool operator<(const Date& other) const {
        if (year != other.year) return year < other.year;
        if (month != other.month) return month < other.month;
        return day < other.day;
    }
    
    bool operator>(const Date& other) const { return other < *this; }
    bool operator<=(const Date& other) const { return !(*this > other); }
    bool operator>=(const Date& other) const { return !(*this < other); }
    bool operator!=(const Date& other) const { return !(*this == other); }
};

using DataType = std::variant<int, float, bool, char, std::string, Date>;
using Row = std::vector<DataType>;

struct Table {
    std::vector<std::string> column_names;
    std::vector<Row> rows;
    
    // Helper to get column index
    int get_column_index(const std::string& name) const {
        auto it = std::find(column_names.begin(), column_names.end(), name);
        if (it == column_names.end()) return -1;
        return static_cast<int>(std::distance(column_names.begin(), it));
    }
    int get_num_cols() const { return static_cast<int>(column_names.size()); }
    int get_num_rows() const { return static_cast<int>(rows.size()); }

    std::pair<int, std::string> get_column_index_case_insensitive(const std::string& name_to_find) const {
        std::string name_to_find_lower = name_to_find;
        // Convert name_to_find to lowercase
        std::transform(name_to_find_lower.begin(), name_to_find_lower.end(), name_to_find_lower.begin(),
                       [](unsigned char c){ return std::tolower(c); });

        for (size_t i = 0; i < column_names.size(); ++i) {
            std::string current_col_name_lower = column_names[i];
            // Convert current column name in table to lowercase for comparison
            std::transform(current_col_name_lower.begin(), current_col_name_lower.end(), current_col_name_lower.begin(),
                           [](unsigned char c){ return std::tolower(c); });
            if (current_col_name_lower == name_to_find_lower) {
                return {static_cast<int>(i), column_names[i]}; // Return index and original cased name
            }
        }
        return {-1, ""}; // Not found
    }
};



// Comparison operations
enum CompareOp { EQ, NE, LT, GT, LE, GE };

// 4. Join condition with multiple columns
struct JoinCondition {
    std::string column_R;
    std::string column_S;
    CompareOp op;
};

// Visitor for comparing DataType values with different operations
struct DataTypeComparator {
    CompareOp op;
    
    bool operator()(const DataType& a, const DataType& b) const {
        return std::visit([this](auto&& arg1, auto&& arg2) {
            using T1 = std::decay_t<decltype(arg1)>;
            using T2 = std::decay_t<decltype(arg2)>;
            
            if constexpr (!std::is_same_v<T1, T2>) {
                return false; // Different types can't be compared
            } else {
                switch (op) {
                    case CompareOp::EQ: return arg1 == arg2;
                    case CompareOp::NE: return arg1 != arg2;
                    case CompareOp::LT: return arg1 < arg2;
                    case CompareOp::GT: return arg1 > arg2;
                    case CompareOp::LE: return arg1 <= arg2;
                    case CompareOp::GE: return arg1 >= arg2;
                    default: return false;
                }
            }
        }, a, b);
    }
};


// Structure to hold parts of a parsed "alias.column" string
struct AliasColumnPair {
    std::string alias;
    std::string column;
    std::string original_text; // For error messages

    bool isValid() const { return !alias.empty() && !column.empty(); }
};