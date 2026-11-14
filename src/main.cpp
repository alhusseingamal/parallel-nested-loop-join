#include <iostream>
#include <math.h>
#include <chrono>
#include "config_loader.h"
#include "db_utils.h"
#include "host.hpp"
int main() {

    // read the query file
    ConfigLoader mainConfig;
    mainConfig.load_config("query.txt");
    std::string table1_path = mainConfig.get_param("table1_path");
    std::string table2_path = mainConfig.get_param("table2_path");
    std::string algorithm_name = mainConfig.get_param("join_algorithm");
    std::string output_directory_path = mainConfig.get_param("output_directory_path");

    std::string natural_alias1 = extractAliasFromFilename(table1_path);
    std::string natural_alias2 = extractAliasFromFilename(table2_path);

    Table table1_data = read_csv(table1_path);
    Table table2_data = read_csv(table2_path);

    const Table* ptr_table_R = nullptr;
    std::string aliasR;
    const Table* ptr_table_S = nullptr;
    std::string aliasS;


    std::cout << "Determining table roles based on size (R = larger/equal, S = smaller):" << std::endl;
    if (table1_data.rows.size() >= table2_data.rows.size()) {
        ptr_table_R = &table1_data;
        aliasR = natural_alias1;
        ptr_table_S = &table2_data;
        aliasS = natural_alias2;
    } else {
        ptr_table_R = &table2_data;
        aliasR = natural_alias2;
        ptr_table_S = &table1_data;
        aliasS = natural_alias1;
    }

    std::cout << "Table R (larger/outer): '" << aliasR << "' (" << ptr_table_R->rows.size() << " rows)" << std::endl;
    std::cout << "Table S (smaller/inner): '" << aliasS << "' (" << ptr_table_S->rows.size() << " rows)" << std::endl << std::endl;

    // Should later modify this to be processed from the file (now there is the function getJoinConditionsFromUser which takes it from the terminal, it can be adapted)
    std::vector<JoinCondition> conditions = {
        {"customer_id", "customer_id", CompareOp::EQ},
        {"customer_id", "age", CompareOp::EQ}
    };

    if (conditions.empty()) {
        std::cout << "No join conditions provided. Exiting." << std::endl;
        return 0;
    } else {
        std::cout << "\nParsed Join Conditions (Table R is '" << aliasR << "', Table S is '" << aliasS << "'):\n";
        for (const auto& cond : conditions) {
            std::cout << "- " << aliasR << "." << cond.column_R
                      << " " << comparisonOperatorToString(cond.op) << " "
                      << aliasS << "." << cond.column_S << std::endl;
        }
    }


    auto start = std::chrono::high_resolution_clock::now();
    Table result_table = nested_loop_join_gpu(*ptr_table_R, *ptr_table_S, conditions);
    auto stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> total_elapsed = stop - start;
    std::cout << "GPU execution time: " << total_elapsed.count() << " seconds\n";
    // Save result to CSV
    std::string output_file_name = algorithm_name + "_GPU.csv";
    save_table(result_table, output_directory_path + output_file_name);

    return 0;
}