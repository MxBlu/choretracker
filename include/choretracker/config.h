#pragma once

#include <optional>
#include <string>

#define CONFIG_BOT_TOKEN "bot_token"
#define CONFIG_TEST_GUILD "test_guild"
#define CONFIG_REGISTER_COMMANDS "register_commands"
#define CONFIG_DB_CONNECTION "db_connection"
#define CONFIG_DB_NAME "db_name"

bool config_load_file();
std::optional<std::string> config_get_str(const std::string& property);
std::optional<bool> config_get_bool(const std::string& property);
std::optional<int> config_get_int(const std::string& property);