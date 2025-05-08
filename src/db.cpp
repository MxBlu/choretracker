#include <ranges>

#include "choretracker/db.h"
#include "choretracker/utils.hpp"

std::span<chore_definition> Database::list_all_chores() {
   return db;
}

std::vector<chore_definition> Database::list_chores_by_user(user_identifier user_identifier) {
   std::lock_guard<std::mutex> lock(db_mutex);

   std::vector<chore_definition> user_chores;
   for (auto chore : db | std::views::filter([&user_identifier](chore_definition &cd) { return cd.owner == user_identifier; })) {
      user_chores.push_back(chore);
   }

   return user_chores;
}

void Database::add_chore(chore_definition chore) {
   std::lock_guard<std::mutex> lock(db_mutex);

   db.emplace_back(chore);
}

bool Database::delete_chore(user_identifier user_identifier, std::string chore_name) {
   std::lock_guard<std::mutex> lock(db_mutex);

   auto deleted_count = std::erase_if(db, [user_identifier, chore_name](const chore_definition chore) { 
      return chore.owner == user_identifier && chore.name == chore_name; 
   });

   return deleted_count > 0;
}

void Database::reset_chore(user_identifier user_identifier, std::string chore_name) {
   std::lock_guard<std::mutex> lock(db_mutex);

   auto chore = std::find_if(db.begin(), db.end(), [user_identifier, chore_name](const chore_definition& chore) {
      return chore.owner == user_identifier && chore.name == chore_name;
   });

   if (chore != db.end()) {
      chore[0].last_completed = get_today_as_ymd();
   }
}