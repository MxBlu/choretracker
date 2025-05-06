#include "choretracker/db.h"

Database::Database() {
}

std::span<chore_definition> Database::list_chores_by_user(user_identifier user_identifier) {
   std::lock_guard<std::mutex> lock(db_mutex);

   return db[user_identifier];
}

void Database::add_chore(user_identifier user_identifier, chore_definition chore) {
   std::lock_guard<std::mutex> lock(db_mutex);

   auto &chores = db[user_identifier];
   chores.emplace_back(chore);
}

bool Database::delete_chore(user_identifier user_identifier, std::string chore_name) {
   std::lock_guard<std::mutex> lock(db_mutex);
   
   auto &chores = db[user_identifier];
   auto deleted_count = std::erase_if(chores, [chore_name](const chore_definition chore) { return chore.name == chore_name; });

   return deleted_count > 0;
}
