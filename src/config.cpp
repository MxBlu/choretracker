#include <fstream>
#include <map>
#include <optional>
#include <string>
#include <stdexcept>

#include <dpp/nlohmann/json.hpp>
using json = nlohmann::json;

static json config_json;

/// @brief Convert given string to uppercase
/// @param str 
/// @return New uppercased string
inline std::string uppercase(const std::string str) {
    std::string copied_str(str);
    std::transform(copied_str.begin(), copied_str.end(), copied_str.begin(), ::toupper);
    return copied_str;
}

/// @brief Load config.json to be parsed
/// @return Whether the file was successfully found
bool config_load_file() {
    std::ifstream config_file("config.json");
    if (config_file.good()) {
        config_json = json::parse(config_file);
        return true;
    } else {
        return false;
    }
}

/// @brief Get a string property from either the config file or env
/// @param property Property name
/// @return Property value as string if present
std::optional<std::string> config_get_str(const std::string property) {
    char *env_value = std::getenv(("CHORETRACKER_" + uppercase(property)).c_str());
    if (env_value != nullptr) {
        return std::string(env_value);
    } else if (config_json.contains(property)) {
        return config_json[property];
    }

    return {};
}

/// @brief Get a boolean property from either the config file or env
/// @param property Property name
/// @return Property value as boolean if present
std::optional<bool> config_get_bool(const std::string property) {
    char *env_value = std::getenv(("CHORETRACKER_" + uppercase(property)).c_str());
    if (env_value != nullptr) {
        return std::strcmp(env_value, "1") == 0 || 
                std::strcmp(env_value, "true") == 0 || 
                std::strcmp(env_value, "TRUE") == 0;
    } else if (config_json.contains(property)) {
        return config_json[property];
    }

    return {};
}

/// @brief Get a integer property from either the config file or env
/// @param property Property name
/// @return Property value as integer if present
std::optional<int> config_get_int(const std::string property) {
    char *env_value = std::getenv(("CHORETRACKER_" + uppercase(property)).c_str());
    if (env_value != nullptr) {
        return std::atoi(env_value);
    } else if (config_json.contains(property)) {
        return config_json[property];
    }

    return {};
}