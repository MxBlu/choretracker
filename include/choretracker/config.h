#pragma once

#include <optional>
#include <string>

#define CONFIG_BOT_TOKEN "bot_token"
#define CONFIG_TEST_GUILD "test_guild"

bool config_load_file();
std::optional<std::string> config_get_str(std::string property);
std::optional<bool> config_get_bool(std::string property);
std::optional<int> config_get_int(std::string property);