#include <chrono>
#include <format>
#include <spdlog/spdlog.h>

#include "choretracker/alerter.h"
#include "choretracker/utils.hpp"

auto get_next_alert_time() {
   const auto now = std::chrono::system_clock::now();
   const auto zone = get_current_tz();
   const auto local_now = zone->to_local(now);

   // Run alerts at 6:00
   auto next_alert_time = floor<std::chrono::days>(local_now) + std::chrono::hours(6);
   if (zone->to_sys(next_alert_time) < now) {
      next_alert_time += std::chrono::days(1);
   }

   return zone->to_sys(next_alert_time);
}

void Alerter::begin() {
   thread = std::thread(&Alerter::thread_task, this);
   spdlog::info("Alerting thread started");
}

void Alerter::run_alerts() {
   auto now = std::chrono::sys_days(get_today_as_ymd());
   spdlog::debug(std::format("Running alerts: now=\"{}\"", now));

   // Map tasks to their respective users
   std::map<dpp::snowflake, std::vector<task_definition>> tasks_by_user;
   auto tasks = db.list_all_tasks();
   for (auto task : tasks) {
      tasks_by_user[task.owner_user_id].push_back(task);
   }

   // Send alerts per user
   for (const auto& pair : tasks_by_user) {
      auto user_id = pair.first;
      auto user_tasks = pair.second;

      // Determine which tasks are due
      std::vector<std::string> one_off_messages;
      std::vector<std::string> repeated_messages;
      for (auto task : user_tasks) {
         switch (task.type) {
            case task_type::once_off:
               one_off_messages.push_back(std::format("* {}", task.name));
               break;
            case task_type::regular: {
               auto next_expected_time = std::chrono::sys_days(task.last_completed) + std::chrono::days(task.frequency_days);
               if (next_expected_time == now) {
                  repeated_messages.push_back(std::format("* {}", task.name));
               } else if (next_expected_time < now) {
                  repeated_messages.push_back(std::format("* {} - {} days late", task.name, (now - next_expected_time).count()));
               } else {
                  spdlog::debug(std::format("Not alerting: name=\"{}\" last_completed=\"{}\" deadline=\"{}\"", 
                     task.name, task.last_completed, next_expected_time));
               }
               break;
            }
            default:
               spdlog::warn(std::format("Unsupported task type: name=\"{}\"", task.name));
         }
      }

      // Send alert if there are any due tasks
      if (!one_off_messages.empty() || !repeated_messages.empty()) {
         std::string alert_message = "You have the following tasks due:\n";
         for (const auto& msg : repeated_messages) {
            alert_message += msg + "\n";
         }
         if (!one_off_messages.empty()) {
            if (!repeated_messages.empty()) {
               alert_message += "\n";
            }
            alert_message += "One-off tasks:\n";
            for (const auto& msg : one_off_messages) {
               alert_message += msg + "\n";
            }
         }

         spdlog::info(std::format("Sending alert to user_id='{}'", user_id.str()));
         bot.direct_message_create(user_id, dpp::message(alert_message));
      }
   }
}

void Alerter::thread_task() {
   spdlog::debug(std::format("Current TZ in alerter thread: {}", get_current_tz()->name()));

   while (true) {
      auto time_to_sleep_until = get_next_alert_time();
      spdlog::debug(std::format("Sleeping until {}", get_current_tz()->to_local(time_to_sleep_until)));
      std::this_thread::sleep_until(time_to_sleep_until);

      run_alerts();
   }
}