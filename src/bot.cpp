#include <format>

#include <spdlog/spdlog.h>

#include "choretracker/bot.h"
#include "choretracker/config.h"
#include "choretracker/utils.hpp"

void list_tasks(Database &db, const dpp::slashcommand_t &event);
void add_task(Database &db, const dpp::slashcommand_t &event);
void delete_task(Database &db, const dpp::slashcommand_t &event);
void complete_task(Database &db, const dpp::slashcommand_t &event);

void Bot::init() {
    cluster.on_ready([this](const dpp::ready_t &event) {
        spdlog::info("Discord connected");

        // Register commands
        if (dpp::run_once<struct register_bot_commands>()) {
            dpp::slashcommand list_tasks_command("listtasks", "List currently tracked tasks", cluster.me.id);
            list_tasks_command.set_interaction_contexts({ dpp::itc_private_channel, dpp::itc_bot_dm, dpp::itc_guild });

            dpp::slashcommand add_tasks_command("addtask", "Add a new task", cluster.me.id);
            add_tasks_command.set_interaction_contexts({ dpp::itc_private_channel, dpp::itc_bot_dm, dpp::itc_guild });
            add_tasks_command.add_option(
                dpp::command_option(dpp::co_string, "name", "Name of task", true));
            add_tasks_command.add_option(
                dpp::command_option(dpp::co_integer, "frequency", "Frequency in days", true));
            
            dpp::slashcommand delete_tasks_command("deletetask", "Delete a task", cluster.me.id);
            delete_tasks_command.set_interaction_contexts({ dpp::itc_private_channel, dpp::itc_bot_dm, dpp::itc_guild });
            delete_tasks_command.add_option(
                dpp::command_option(dpp::co_string, "name", "Name of task", true).set_auto_complete(true));
            
            dpp::slashcommand reset_task_command("resettask", "Reset a timer on a task", cluster.me.id);
            reset_task_command.set_interaction_contexts({ dpp::itc_private_channel, dpp::itc_bot_dm, dpp::itc_guild });
            reset_task_command.add_option(
                dpp::command_option(dpp::co_string, "name", "Name of task", true).set_auto_complete(true));

            if (config_get_bool(CONFIG_REGISTER_COMMANDS).value_or(false)) {
                auto test_guild = config_get_str(CONFIG_TEST_GUILD);
                if (test_guild.has_value()) {
                    // Command only available when testing in a guild
                    dpp::slashcommand run_alerts_command("runalerts", "Run alerts for tasks", cluster.me.id);

                    cluster.guild_bulk_command_create({
                        list_tasks_command, add_tasks_command, delete_tasks_command, reset_task_command, run_alerts_command
                    }, test_guild.value());
                    spdlog::info(std::format("Discord commands registered to guild: guildId={}", test_guild.value()));
                } else {
                    cluster.global_bulk_command_create({
                        list_tasks_command, add_tasks_command, delete_tasks_command, reset_task_command
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
        spdlog::info(std::format("Command received: command='{}' user='{}'", event.command.get_command_name(), event.command.usr.username));
        if (event.command.get_command_name() == "listtasks") {
            list_tasks(db, event);
        } else if (event.command.get_command_name() == "addtask") {
            add_task(db, event);
        } else if (event.command.get_command_name() == "deletetask") {
            delete_task(db, event);
        } else if (event.command.get_command_name() == "resettask") {
            complete_task(db, event);
        } else if (event.command.get_command_name() == "runalerts") {
            alerter.run_alerts();
        } else {
            spdlog::error("Unknown command received");
        }
    });

    cluster.on_autocomplete([this](const dpp::autocomplete_t &event) {
        spdlog::info(std::format("Autocomplete triggered for command: command='{}' user='{}'", event.name, event.command.usr.username));
        
        std::optional<dpp::command_option> o_focused_opt;
        for (auto &opt : event.options) {
            if (opt.focused) {
                o_focused_opt = opt;
            }
        }

        if (!o_focused_opt.has_value()) {
            spdlog::warn("Autocomplete command had no focused opt?");
            return;
        }

        auto user_id = event.command.usr.id;
        auto focused_opt = o_focused_opt.value();
        if (focused_opt.name == "name") {
            std::string value = std::get<std::string>(focused_opt.value);
            dpp::interaction_response resp(dpp::ir_autocomplete_reply);

            std::vector<task_definition> tasks;
            if (value.empty()) {
                tasks = db.list_tasks_by_user(user_id);
            } else {
                tasks = db.find_tasks_by_name(user_id, value);
            }
            for (auto task : tasks) {
                resp.add_autocomplete_choice(dpp::command_option_choice(task.name, task.name));
            }

            cluster.interaction_response_create(event.command.id, event.command.token, resp);
        }
    });

    // Start the bot itself
    cluster.start(dpp::st_return);
}

/* Commands */

void list_tasks(Database &db, const dpp::slashcommand_t &event) {
    auto user_id = event.command.usr.id;

    auto tasks = db.list_tasks_by_user(user_id);
    if (tasks.size() > 0) {
        std::string task_list = "Tasks: \n";
        for (auto task : tasks) {
            task_list += std::format("{} - Every {} days - Last performed {}\n", 
                task.name, task.frequency_days, task.last_completed);
        }
        
        event.reply(dpp::message(task_list).set_flags(dpp::m_ephemeral));
    } else {
        event.reply(dpp::message("No tasks found").set_flags(dpp::m_ephemeral));
    }
}

void add_task(Database &db, const dpp::slashcommand_t &event) {
    auto user_id = event.command.usr.id;
    auto guild_id = event.command.guild_id;

    auto task_name = std::get<std::string>(event.get_parameter("name"));
    auto task_frequency = std::get<int64_t>(event.get_parameter("frequency"));

    db.add_task({ 
        user_id,
        task_name,
        task_type::regular,
        static_cast<int>(task_frequency), 
        get_today_as_ymd() 
    });

    event.reply(dpp::message("Task added").set_flags(dpp::m_ephemeral));
}

void delete_task(Database &db, const dpp::slashcommand_t &event) {
    auto user_id = event.command.usr.id;

    auto task_name = std::get<std::string>(event.get_parameter("name"));

    bool deleted = db.delete_task(user_id, task_name);
    if (deleted) {
        event.reply(dpp::message("Task deleted").set_flags(dpp::m_ephemeral));
    } else {
        event.reply(dpp::message("Task not found").set_flags(dpp::m_ephemeral));
    }
}

void complete_task(Database &db, const dpp::slashcommand_t &event) {
    auto user_id = event.command.usr.id;

    auto task_name = std::get<std::string>(event.get_parameter("name"));

    db.complete_task(user_id, task_name);
    event.reply(dpp::message("Task reset").set_flags(dpp::m_ephemeral));
}