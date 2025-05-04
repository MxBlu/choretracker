#include <iostream>
#include <optional>
#include <string>

#include <dpp/dpp.h>
#include <dpp/nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "choretracker/config.h"

int main(int argc, char const *argv[]) {
    // Ensure config file is loaded
    bool config_was_loaded = config_load_file();
    if (config_was_loaded) {
        spdlog::info("Config file loaded");
    } else {
        spdlog::info("Config not found, will use env vars");
    }

    // Find the bot token or quit
    auto bot_token = config_get_str(CONFIG_BOT_TOKEN);
    if (!bot_token.has_value()) {
        spdlog::error("Bot token not defined, exiting");
        exit(1);
    }

    // Initialise the bot and wait forever
    dpp::cluster bot(bot_token.value());
    bot.start(dpp::st_wait);

    return 0;
}
