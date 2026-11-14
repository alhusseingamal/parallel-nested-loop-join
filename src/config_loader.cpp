#include "config_loader.h"
#include <fstream>
#include <sstream>
#include <iostream>

// Implements the ConfigLoader class methods.
void ConfigLoader::load_config(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open config file " << filename << std::endl;
        return;
    }
    std::string line;
    while (std::getline(file, line)) {
        // Simple key=value parser
        std::istringstream iss(line);
        std::string key, value;
        if (std::getline(iss, key, '=') && std::getline(iss, value)) {
            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t\r\n"));
            key.erase(key.find_last_not_of(" \t\r\n") + 1);
            value.erase(0, value.find_first_not_of(" \t\r\n"));
            value.erase(value.find_last_not_of(" \t\r\n") + 1);
            if (!key.empty()) {
               params[key] = value;
            }
        }
    }
}

std::string ConfigLoader::get_param(const std::string& param_name) {
    if (params.find(param_name) != params.end()) {
        return params[param_name];
    }
    std::cerr << "Warning: Parameter '" << param_name << "' not found in config." << std::endl;
    return "";
}
