#pragma once

#include <algorithm>
#include <chrono>
#include <string>
#include <spdlog/spdlog.h>
#include <bsoncxx/document/view.hpp>
#include <bsoncxx/string/to_string.hpp>
#include <bsoncxx/types.hpp>

/// @brief Convert given string to uppercase
/// @param str 
/// @return New uppercased string
inline std::string uppercase(const std::string& str) {
   std::string copied_str(str);
   std::transform(copied_str.begin(), copied_str.end(), copied_str.begin(), ::toupper);
   return copied_str;
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

/// @brief Get the current day as a year_month_day object
/// @return Current day as a year_month_day object
inline std::chrono::year_month_day get_today_as_ymd() {
   const auto now = std::chrono::system_clock::now();
   const auto local_now = get_current_tz()->to_local(now);
   return std::chrono::year_month_day{ std::chrono::floor<std::chrono::days>(local_now) };
}

/// @brief Parse a date string and return a year_month_day object 
/// @param date_str Date in "YYYY-MM-DD" format
/// @return year_month_day object 
inline auto parse_ymd(const std::string& date_str) {
   std::istringstream in{date_str};
   std::chrono::year_month_day ymd;
   in >> std::chrono::parse("%F", ymd); // expects "YYYY-MM-DD"
   return ymd;
}

/// @brief Convert a year_month_day to a string
/// @param ymd year_month_day object 
/// @return Date string as "YYYY-MM-DD"
inline auto ymd_to_string(const std::chrono::year_month_day& ymd) {
   return std::format("{:%F}", ymd);
}

/// @brief Get a string value from a BSON element
/// @param element BSON element
/// @return String value
inline auto bson_to_string(const bsoncxx::document::element& element) {
   return bsoncxx::string::to_string(element.get_string().value);
}