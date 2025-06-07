#pragma once

#include <algorithm>
#include <chrono>
#include <cctype>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <stdexcept>
#include <spdlog/spdlog.h>
#include <bsoncxx/document/view.hpp>
#include <bsoncxx/string/to_string.hpp>
#include <bsoncxx/types.hpp>

#define STRINGIFY_NX(x) #x
#define STRINGIFY(x) STRINGIFY_NX(x)

/// @brief Convert a hex digit character to its integer value
/// @param hex Char
/// @return Integer
inline constexpr int hexDigitToInt(char hex) noexcept {
   if (hex >= '0' && hex <= '9') {
      return hex - '0';
   }
   if (hex >= 'A' && hex <= 'F') {
      return hex - 'A' + 10;
   }
   if (hex >= 'a' && hex <= 'f') {
      return hex - 'a' + 10;
   }
   return 0; // Should not reach here if input is validated
}

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
/// @return year_month_day object if parsable, empty if not
inline std::optional<std::chrono::year_month_day> parse_ymd(const std::string& date_str) {
   auto pos1 = date_str.find('-');
   auto pos2 = date_str.find('-', pos1 + 1);
   
   if (pos1 == std::string::npos || pos2 == std::string::npos) {
      return {};
   }

   int year = std::stoi(date_str.substr(0, pos1));
   int month = std::stoi(date_str.substr(pos1 + 1, pos2 - pos1 - 1));
   int day = std::stoi(date_str.substr(pos2 + 1));

   return std::chrono::year_month_day{
      std::chrono::year{year},
      std::chrono::month{static_cast<unsigned>(month)},
      std::chrono::day{static_cast<unsigned>(day)}
   };
}

/// @brief Convert a year_month_day to a string
/// @param ymd year_month_day object 
/// @return Date string as "YYYY-MM-DD"
inline auto ymd_to_string(const std::chrono::year_month_day& ymd) {
   return std::format("{:%Y}-{:%m}-{:%d}", ymd.year(), ymd.month(), ymd.day());
}

/// @brief Get a string value from a BSON element
/// @param element BSON element
/// @return String value
inline auto bson_to_string(const bsoncxx::document::element& element) {
   return bsoncxx::string::to_string(element.get_string().value);
}

inline std::string uri_decode(std::string_view encoded) {
   std::string result;
   result.reserve(encoded.size()); // Reserve space for efficiency
   
   for (size_t i = 0; i < encoded.size(); ++i) {
      char c = encoded[i];
      
      if (c == '%') {
         // Check if we have enough characters for a percent-encoded sequence
         if (i + 2 >= encoded.size()) {
            throw std::invalid_argument("Invalid percent-encoded sequence: incomplete");
         }
         
         // Get the two hex digits
         char hex1 = encoded[i + 1];
         char hex2 = encoded[i + 2];
         
         if (!std::isxdigit(hex1) || !std::isxdigit(hex2)) {
            throw std::invalid_argument(
               std::format("Invalid percent-encoded sequence: %{}{}", hex1, hex2)
            );
         }
         
         // Convert hex digits to byte value
         int value = (hexDigitToInt(hex1) << 4) | hexDigitToInt(hex2);
         result += static_cast<char>(value);
         
         // Skip the two hex digits
         i += 2;
      }  else if (c == '+') {
         // In application/x-www-form-urlencoded, '+' represents space
         result += ' ';
      } else {
         // Regular character
         result += c;
      }
   }
   
   return result;
}