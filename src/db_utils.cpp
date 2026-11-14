#include <fstream>
#include <sstream>
#include <iostream>
#include "db_utils.h"

// Flatten strings into a single char array and store offsets
void flattenStrings(const std::vector<std::string>& hostStrings, std::vector<char>& h_flattenedStrings, std::vector<int>& h_offsets) {
    int currentOffset = 0;
    for (const auto& str : hostStrings) {
        h_offsets.push_back(currentOffset); // Store the starting offset
        h_flattenedStrings.insert(h_flattenedStrings.end(), str.begin(), str.end());
        h_flattenedStrings.push_back('\0'); // Null-terminate each string
        currentOffset += static_cast<int>(str.size() + 1); // +1 for null terminator
    }
}

Date parse_date(const std::string& s) {
    std::istringstream iss(s);
    char sep;
    Date d;
    if (iss >> d.day >> sep >> d.month >> sep >> d.year) {
        return d;
    }
    throw std::runtime_error("Invalid date format: " + s);
}

DataType parse_cell(const std::string& s) {
    
    std::string lower_s = s;
    std::transform(lower_s.begin(), lower_s.end(), lower_s.begin(), ::tolower);

    // Try parsing as boolean
    if (lower_s == "true") return true;
    if (lower_s == "false") return false;
    
    // Handle Null values
    if (lower_s == "null") return std::string("NULL"); // Handle null as string for now

    // Try parsing as int
    try {
        size_t pos;
        int i = std::stoi(s, &pos);
        if (pos == s.size()) return i;
    } catch (...) {}
    
    // Try parsing as float
    try {
        size_t pos;
        float f = std::stof(s, &pos);
        if (pos == s.size()) return f;
    } catch (...) {}
    
    // Try parsing as date
    if (s.find('/') != std::string::npos) {
        try {
            return parse_date(s);
        } catch (...) {}
    }

    // Try parsing as char (single character string)
    // There would be a problem if some tuples have single-character values intented-to-be strings, but it's a rare case which we can ignore
    if (s.size() == 1) {
        return s[0];
    }
    
    // Default to string
    return s;
}

Table read_csv(const std::string& filename) {
    Table table;
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    auto trim = [](std::string& s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), s.end());
    };

    // Read header
    std::string line;
    if (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string col;
        while (std::getline(iss, col, ',')) {
            trim(col);
            table.column_names.push_back(col);
        }
    }

    // Read data rows
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string cell;
        Row row;

        while (std::getline(iss, cell, ',')) {
            trim(cell);
            row.push_back(parse_cell(cell));
        }

        if (!row.empty()) {
            table.rows.push_back(std::move(row));
        }
    }

    return table;
}


void print_table(const Table& table) {
    constexpr int FIELD_WIDTH = 13; // Fixed width for each column

    // Print column names with fixed width
    for (const auto& col : table.column_names) {
        std::cout << std::setw(FIELD_WIDTH) << std::left << col; // Align to the left
    }
    std::cout << "\n";

    // Print rows with fixed-width fields
    for (const auto& row : table.rows) {
        for (const auto& cell : row) {
            std::visit([&](auto&& arg) { 
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, Date>) {
                    // Format date values as dd/mm/yyyy
                    std::cout << std::setw(FIELD_WIDTH) << std::left
                              << (std::to_string(arg.day) + "/" + 
                                  std::to_string(arg.month) + "/" + 
                                  std::to_string(arg.year));
                } else {
                    // Format other types
                    std::cout << std::setw(FIELD_WIDTH) << std::left << arg;
                }
            }, cell);
        }
        std::cout << "\n";
    }
}


void save_table(const Table& table, const std::string& filename) {
    std::ofstream out_file(filename);
    if (!out_file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    // Write column headers
    for (size_t i = 0; i < table.column_names.size(); ++i) {
        if (i != 0) out_file << ",";
        out_file << table.column_names[i];
    }
    out_file << "\n";

    // Write data rows
    for (const auto& row : table.rows) {
        for (size_t i = 0; i < row.size(); ++i) {
            if (i != 0) out_file << ",";
            out_file << data_type_to_string(row[i]);
        }
        out_file << "\n";
    }

    out_file.close();
}


std::string data_type_to_string(const DataType& value) {
    return std::visit([](auto&& arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, int>) {
            return std::to_string(arg);
        }
        else if constexpr (std::is_same_v<T, float>) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << arg;
            return oss.str();
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            if (arg.find('"') != std::string::npos || arg.find(',') != std::string::npos) {
                std::string escaped = arg;
                size_t pos = 0;
                while ((pos = escaped.find('"', pos)) != std::string::npos) {
                    escaped.replace(pos, 1, "\"\"");
                    pos += 2;
                }
                return "\"" + escaped + "\"";
            }
            return arg;
        }
        else if constexpr (std::is_same_v<T, Date>) {
            std::ostringstream oss;
            oss << std::setw(2) << std::setfill('0') << arg.day << "/"
                << std::setw(2) << std::setfill('0') << arg.month << "/"
                << arg.year;
            return oss.str();
        }
        else if constexpr (std::is_same_v<T, bool>) {
            return arg ? "true" : "false";
        }
        else if constexpr (std::is_same_v<T, char>) {
            return std::string(1, arg);
        }
        else {
            static_assert(always_false_v<T>, "Unsupported type in data_type_to_string");
            return ""; // This line will never be reached
        }
    }, value);
}

static inline void trim_string_inplace(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}


AliasColumnPair parseAliasColumnString(std::string part_str) {
    trim_string_inplace(part_str);
    AliasColumnPair result;
    result.original_text = part_str;

    // Use rfind for dot, in case column names could hypothetically contain dots (aliases typically don't)
    size_t dot_pos = part_str.rfind('.');
    if (dot_pos == std::string::npos || dot_pos == 0 || dot_pos == part_str.length() - 1) {
        // No dot, or dot is at the very beginning or end, which is invalid
        return result; // Will be invalid due to empty alias or column
    }

    result.alias = part_str.substr(0, dot_pos);
    result.column = part_str.substr(dot_pos + 1);
    trim_string_inplace(result.alias);
    trim_string_inplace(result.column);

    // Ensure neither part is empty after trimming
    if (result.alias.empty() || result.column.empty()) {
         return {}; // Return an explicitly invalid AliasColumnPair
    }
    return result;
}


CompareOp flipComparisonOperator(CompareOp op) {
    switch (op) {
        case CompareOp::EQ: return CompareOp::EQ;
        case CompareOp::NE: return CompareOp::NE;
        case CompareOp::LT: return CompareOp::GT;
        case CompareOp::GT: return CompareOp::LT;
        case CompareOp::LE: return CompareOp::GE;
        case CompareOp::GE: return CompareOp::LE;
    }
    // Should not be reached if all ops are handled
    throw std::logic_error("Unhandled CompareOp in flipComparisonOperator");
}

std::string comparisonOperatorToString(CompareOp op) {
    switch (op) {
        case CompareOp::EQ: return "==";
        case CompareOp::NE: return "!=";
        case CompareOp::LT: return "<";
        case CompareOp::GT: return ">";
        case CompareOp::LE: return "<=";
        case CompareOp::GE: return ">=";
    }
    return "??"; // Should not be reached
}

// Helper function to extract a base name from a filepath to use as an alias
std::string extractAliasFromFilename(const std::string& filepath) {
    // Find the last path separator ('/' or '\')
    size_t last_slash_idx = filepath.find_last_of("/\\");
    std::string filename_with_ext;
    if (last_slash_idx == std::string::npos) {
        filename_with_ext = filepath; // No path separator, just filename
    } else {
        filename_with_ext = filepath.substr(last_slash_idx + 1);
    }

    // Find the last dot '.' to remove the extension
    size_t dot_idx = filename_with_ext.rfind('.');
    if (dot_idx == std::string::npos || dot_idx == 0) { 
        // No extension found, or filename starts with a dot (e.g., hidden file)
        return filename_with_ext;
    }
    return filename_with_ext.substr(0, dot_idx);
}