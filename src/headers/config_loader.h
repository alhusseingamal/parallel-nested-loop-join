#pragma once
#include <string>
#include <map>
class ConfigLoader {
public:
    void load_config(const std::string& filename);
    std::string get_param(const std::string& param_name);
private:
    std::map<std::string, std::string> params;
};