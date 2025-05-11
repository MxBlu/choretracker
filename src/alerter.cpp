#include <chrono>
#include <format>
#include <spdlog/spdlog.h>

#include "choretracker/alerter.h"
#include "choretracker/utils.hpp"

auto get_next_alert_time() {
   const auto now = std::chrono::system_clock::now();
   const auto zone = std::chrono::current_zone();
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

   auto chores = db.list_all_chores();
   for (auto chore : chores) {
      auto next_expected_time = std::chrono::sys_days(chore.last_completed) + std::chrono::days(chore.frequency_days);

      if (next_expected_time == now) {
         bot.direct_message_create(chore.owner_user_id, dpp::message(std::format("{} should be done!", chore.name)));
      } else if (next_expected_time < now) {
         bot.direct_message_create(chore.owner_user_id, dpp::message(std::format("{} should have been done on {}!", chore.name, next_expected_time)));
      } else {
         spdlog::debug(std::format("Not alerting: name=\"{}\" last_completed=\"{}\" deadline=\"{}\"", chore.name, chore.last_completed, next_expected_time));
      }
   }
}

void Alerter::thread_task() {
   while (true) {
      auto time_to_sleep_until = get_next_alert_time();
      spdlog::debug(std::format("Sleeping until {}", std::chrono::current_zone()->to_local(time_to_sleep_until)));
      std::this_thread::sleep_until(time_to_sleep_until);

      run_alerts();
   }
}