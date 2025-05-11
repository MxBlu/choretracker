#include <ranges>

#include "choretracker/db.h"
#include "choretracker/utils.hpp"

std::span<chore_definition> Database::list_all_chores() {
   return db;
}

std::vector<chore_definition> Database::list_chores_by_user(dpp::snowflake user_id) {
   std::lock_guard<std::mutex> lock(db_mutex);

   std::vector<chore_definition> user_chores;
   for (auto chore : db | std::views::filter([user_id](chore_definition &cd) { return cd.owner_user_id == user_id; })) {
      user_chores.push_back(chore);
   }

   return user_chores;
}

void Database::add_chore(chore_definition chore) {
   std::lock_guard<std::mutex> lock(db_mutex);

   db.emplace_back(chore);
}

bool Database::delete_chore(dpp::snowflake user_id, std::string chore_name) {
   std::lock_guard<std::mutex> lock(db_mutex);

   auto deleted_count = std::erase_if(db, [user_id, chore_name](const chore_definition chore) { 
      return chore.owner_user_id == user_id && chore.name == chore_name; 
   });

   return deleted_count > 0;
}

void Database::reset_chore(dpp::snowflake user_id, std::string chore_name) {
   std::lock_guard<std::mutex> lock(db_mutex);

   auto chore = std::find_if(db.begin(), db.end(), [user_id, chore_name](const chore_definition& chore) {
      return chore.owner_user_id == user_id && chore.name == chore_name;
   });

   if (chore != db.end()) {
      chore[0].last_completed = get_today_as_ymd();
   }
}