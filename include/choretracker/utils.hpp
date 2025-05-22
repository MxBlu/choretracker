#pragma once

#include <algorithm>
#include <chrono>
#include <string>

/// @brief Convert given string to uppercase
/// @param str 
/// @return New uppercased string
inline std::string uppercase(const std::string str) {
   std::string copied_str(str);
   std::transform(copied_str.begin(), copied_str.end(), copied_str.begin(), ::toupper);
   return copied_str;
}

/// @brief Get the current day as a year_month_day object
/// @return Current day as a year_month_day object
inline std::chrono::year_month_day get_today_as_ymd() {
   const auto now = std::chrono::system_clock::now();
   const auto local_now = std::chrono::current_zone()->to_local(now);
   return std::chrono::year_month_day{ std::chrono::floor<std::chrono::days>(local_now) };
}

/// @brief Get the current time zone, respecting the TZ env var if set
/// @return Current time zone
inline auto get_current_tz() {
   // First try the env var if present
   auto name = std::getenv("TZ");
   if (name != nullptr) {
      try {
         return std::chrono::locate_zone(name);
      } catch (std::runtime_error&) {
         // Just log on an error, not a catastrophic issue
         spdlog::warn("Invalid TZ env var value, defaulting to the host's time zone");
      }
   }

   // Fallback to using chrono::current_zone(), which is likely based on /etc/localtime
   return std::chrono::current_zone();
}