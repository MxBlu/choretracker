#include <format>
#include <iostream>
#include <optional>
#include <string>
#include <future>
#include <csignal>

#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>

#include "choretracker/config.h"
#include "choretracker/web.h"
#include "choretracker/db.h"
#include "choretracker/bot.h"

std::promise<void> thread_wait;

void graceful_shutdown(int signal) {
    spdlog::info(std::format("Received signal: {}", signal));
    thread_wait.set_value();
}

int main(int argc, char const *argv[]) {
    spdlog::cfg::load_env_levels();

    // TODO: Fix shutdown handler
    
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

    auto db_connection_string = config_get_str(CONFIG_DB_CONNECTION);
    if (!db_connection_string.has_value()) {
        spdlog::error("DB connection string not defined, exiting");
        exit(1);
    }

    auto discord_client_id = config_get_str(CONFIG_DISCORD_CLIENT_ID);
    if (!discord_client_id.has_value()) {
        spdlog::error("Discord client ID not defined, exiting");
        exit(1);
    }

    auto discord_client_secret = config_get_str(CONFIG_DISCORD_CLIENT_SECRET);
    if (!discord_client_secret.has_value()) {
        spdlog::error("Discord client secret not defined, exiting");
        exit(1);
    }

    auto db_name = config_get_str(CONFIG_DB_NAME).value_or(DEFAULT_DB_NAME);
    auto web_port = config_get_int(CONFIG_WEB_PORT).value_or(DEFAULT_WEB_PORT);
    auto web_base_url = config_get_str(CONFIG_WEB_BASE_URL).value_or(DEFAULT_WEB_BASE_URL);

    Database db(db_connection_string.value(), db_name);
    Bot bot(bot_token.value(), db);
    Web web(web_port, web_base_url, discord_client_id.value(), discord_client_secret.value(), db);

    // Sleep forever
    thread_wait.get_future().get();

    return 0;
}
