#include <format>
#include <iostream>
#include <optional>
#include <string>
#include <future>
#include <csignal>

#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>

#include "choretracker/config.h"
#include "choretracker/db.h"
#include "choretracker/bot.h"

std::promise<void> thread_wait;

void graceful_shutdown(int signal) {
    thread_wait.set_value();
}

int main(int argc, char const *argv[]) {
    spdlog::cfg::load_env_levels();
    
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

    auto db_name = config_get_str(CONFIG_DB_NAME).value_or(DEFAULT_DB_NAME);

    Database db(db_connection_string.value(), db_name);
    Bot bot(bot_token.value(), db);

    // Create signal handler to correctly shut down threads
    std::signal(SIGINT, graceful_shutdown);

    // Sleep forever
    thread_wait.get_future().wait();

    return 0;
}
