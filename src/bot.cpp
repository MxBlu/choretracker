#include <format>

#include <spdlog/spdlog.h>

#include "choretracker/bot.h"
#include "choretracker/config.h"
#include "choretracker/utils.hpp"

void list_chores(Database &db, const dpp::slashcommand_t &event);
void add_chore(Database &db, const dpp::slashcommand_t &event);
void delete_chore(Database &db, const dpp::slashcommand_t &event);
void complete_chore(Database &db, const dpp::slashcommand_t &event);

void Bot::init() {
    cluster.on_ready([this](const dpp::ready_t &event) {
        spdlog::info("Discord connected");

        // Register commands
        if (dpp::run_once<struct register_bot_commands>()) {
            dpp::slashcommand list_chores_command("listchores", "List currently tracked chores", cluster.me.id);
            list_chores_command.set_interaction_contexts({ dpp::itc_private_channel, dpp::itc_bot_dm, dpp::itc_guild });

            dpp::slashcommand add_chores_command("addchore", "Add a new chore", cluster.me.id);
            list_chores_command.set_interaction_contexts({ dpp::itc_private_channel, dpp::itc_bot_dm, dpp::itc_guild });
            add_chores_command.add_option(
                dpp::command_option(dpp::co_string, "name", "Name of chore", true));
            add_chores_command.add_option(
                dpp::command_option(dpp::co_integer, "frequency", "Frequency in days", true));
            
            dpp::slashcommand delete_chores_command("deletechore", "Delete a chore", cluster.me.id);
            list_chores_command.set_interaction_contexts({ dpp::itc_private_channel, dpp::itc_bot_dm, dpp::itc_guild });
            delete_chores_command.add_option(
                dpp::command_option(dpp::co_string, "name", "Name of chore", true));
                
            dpp::slashcommand reset_chore_command("resetchore", "Reset a timer on a chore", cluster.me.id);
            list_chores_command.set_interaction_contexts({ dpp::itc_private_channel, dpp::itc_bot_dm, dpp::itc_guild });
            reset_chore_command.add_option(
                dpp::command_option(dpp::co_string, "name", "Name of chore", true));

            if (config_get_bool(CONFIG_REGISTER_COMMANDS).value_or(false)) {
                auto test_guild = config_get_str(CONFIG_TEST_GUILD);
                if (test_guild.has_value()) {
                    // Command only available when testing in a guild
                    dpp::slashcommand run_alerts_command("runalerts", "Run alerts for chores", cluster.me.id);

                    cluster.guild_bulk_command_create({
                        list_chores_command, add_chores_command, delete_chores_command, reset_chore_command, run_alerts_command
                    }, test_guild.value());
                    spdlog::info(std::format("Discord commands registered to guild: guildId={}", test_guild.value()));
                } else {
                    cluster.global_bulk_command_create({
                        list_chores_command, add_chores_command, delete_chores_command, reset_chore_command
                    });
                    spdlog::info("Discord commands registered globally");
                }
            } else {
                spdlog::warn("Not registering Discord commands");
            }
        }

        // Start alerting thread
        if (dpp::run_once<struct start_alert_threads>()) {
            alerter.begin();
        }
    });

    cluster.on_log([](const dpp::log_t &log) {
        switch (log.severity) {
            case dpp::ll_info:
                spdlog::info("Discord: " + log.message);
                break;
            case dpp::ll_warning:
                spdlog::warn("Discord: " + log.message);
                break;
            case dpp::ll_error:
            case dpp::ll_critical:
                spdlog::error("Discord: " + log.message);
                break;
            default:
                break;
        }
    });

    cluster.on_slashcommand([this](const dpp::slashcommand_t &event) {
        spdlog::info(std::format("Command received: {} from {}", event.command.get_command_name(), event.command.usr.username));
        if (event.command.get_command_name() == "listchores") {
            list_chores(db, event);
        } else if (event.command.get_command_name() == "addchore") {
            add_chore(db, event);
        } else if (event.command.get_command_name() == "deletechore") {
            delete_chore(db, event);
        } else if (event.command.get_command_name() == "resetchore") {
            complete_chore(db, event);
        } else if (event.command.get_command_name() == "runalerts") {
            alerter.run_alerts();
        } else {
            spdlog::error("Unknown command received");
        }
    });

    // Start the bot itself
    cluster.start(dpp::st_return);
}

/* Commands */

void list_chores(Database &db, const dpp::slashcommand_t &event) {
    auto user_id = event.command.usr.id;

    auto chores = db.list_chores_by_user(user_id);
    if (chores.size() > 0) {
        std::string chore_list = "Chores: \n";
        for (auto chore : chores) {
            chore_list += std::format("{} - Every {} days - Last performed {}\n", 
                chore.name, chore.frequency_days, chore.last_completed);
        }
        
        event.reply(dpp::message(chore_list).set_flags(dpp::m_ephemeral));
    } else {
        event.reply(dpp::message("No chores found").set_flags(dpp::m_ephemeral));
    }
}

void add_chore(Database &db, const dpp::slashcommand_t &event) {
    auto user_id = event.command.usr.id;
    auto guild_id = event.command.guild_id;

    auto chore_name = std::get<std::string>(event.get_parameter("name"));
    auto chore_frequency = std::get<int64_t>(event.get_parameter("frequency"));

    db.add_chore({ 
        user_id,
        chore_name, 
        static_cast<int>(chore_frequency), 
        get_today_as_ymd() 
    });

    event.reply(dpp::message("Chore added").set_flags(dpp::m_ephemeral));
}

void delete_chore(Database &db, const dpp::slashcommand_t &event) {
    auto user_id = event.command.usr.id;

    auto chore_name = std::get<std::string>(event.get_parameter("name"));

    bool deleted = db.delete_chore(user_id, chore_name);
    if (deleted) {
        event.reply(dpp::message("Chore deleted").set_flags(dpp::m_ephemeral));
    } else {
        event.reply(dpp::message("Chore not found").set_flags(dpp::m_ephemeral));
    }
}

void complete_chore(Database &db, const dpp::slashcommand_t &event) {
    auto user_id = event.command.usr.id;

    auto chore_name = std::get<std::string>(event.get_parameter("name"));

    db.complete_chore(user_id, chore_name);
    event.reply(dpp::message("Chore reset").set_flags(dpp::m_ephemeral));
}