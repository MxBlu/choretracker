#include <format>
#include <iostream>
#include <optional>
#include <string>

#include <dpp/dpp.h>
#include <dpp/nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "choretracker/config.h"
#include "choretracker/db.h"
#include "choretracker/utils.hpp"

static Database db;

void list_chores(const dpp::slashcommand_t &event) {
    auto user_id = event.command.usr.id;
    auto guild_id = event.command.guild_id;

    auto chores = db.list_chores_by_user({ user_id, guild_id });
    if (chores.size() > 0) {
        std::string chore_list = "Chores: \n";
        for (auto chore : chores) {
            chore_list += std::format("{} - Every {} days - Last performed {}", 
                chore.name, chore.frequency_days, chore.last_completed);
        }
        
        event.reply(dpp::message(chore_list).set_flags(dpp::m_ephemeral));
    } else {
        event.reply(dpp::message("No chores found").set_flags(dpp::m_ephemeral));
    }
}

void add_chore(const dpp::slashcommand_t &event) {
    auto user_id = event.command.usr.id;
    auto guild_id = event.command.guild_id;

    auto chore_name = std::get<std::string>(event.get_parameter("name"));
    auto chore_frequency = std::get<int64_t>(event.get_parameter("frequency"));

    db.add_chore({ user_id, guild_id }, { 
        chore_name, 
        static_cast<int>(chore_frequency), 
        get_today_as_ymd() 
    });

    event.reply(dpp::message("Chore added").set_flags(dpp::m_ephemeral));
}

void delete_chore(const dpp::slashcommand_t &event) {
    auto user_id = event.command.usr.id;
    auto guild_id = event.command.guild_id;

    auto chore_name = std::get<std::string>(event.get_parameter("name"));

    bool deleted = db.delete_chore({ user_id, guild_id }, chore_name);
    if (deleted) {
        event.reply(dpp::message("Chore deleted").set_flags(dpp::m_ephemeral));
    } else {
        event.reply(dpp::message("Chore not found").set_flags(dpp::m_ephemeral));
    }
}

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

    dpp::cluster bot(bot_token.value());

    bot.on_ready([&bot](const dpp::ready_t &event) {
        // Register commands
        if (dpp::run_once<struct register_bot_commands>()) {
            dpp::slashcommand list_chores_command("listchores", "List currently tracked chores", bot.me.id);

            dpp::slashcommand add_chores_command("addchore", "Add a new chore", bot.me.id);
            add_chores_command.add_option(
                dpp::command_option(dpp::co_string, "name", "Name of chore", true));
            add_chores_command.add_option(
                dpp::command_option(dpp::co_integer, "frequency", "Frequency in days", true));
            
            dpp::slashcommand delete_chores_command("deletechore", "Delete a chore", bot.me.id);
            delete_chores_command.add_option(
                dpp::command_option(dpp::co_string, "name", "Name of chore", true));

            auto test_guild = config_get_str(CONFIG_TEST_GUILD).value();
            bot.guild_bulk_command_create({
                list_chores_command, add_chores_command, delete_chores_command
            }, test_guild);
        }
    });

    bot.on_slashcommand([](const dpp::slashcommand_t &event) {
        spdlog::info("Command received: " + event.command.get_command_name());
        if (event.command.get_command_name() == "listchores") {
            list_chores(event);
        } else if (event.command.get_command_name() == "addchore") {
            add_chore(event);
        } else if (event.command.get_command_name() == "deletechore") {
            delete_chore(event);
        }
    });

    // Initialise the bot and wait forever
    bot.start(dpp::st_wait);

    return 0;
}
