#include <format>
#include <bsoncxx/builder/stream/document.hpp>
#include <spdlog/spdlog.h>

#include "choretracker/db.h"
#include "choretracker/utils.hpp"

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

#define TASK_COL "chores"
#define USER_SESSION_COL "user_sessions"

std::vector<task_definition> Database::list_all_tasks() {
   auto client = pool.acquire();
   auto db = client[db_name];

   auto cursor = db[TASK_COL].find({});

   std::vector<task_definition> tasks;
   for (auto&& doc : cursor) {
      auto def = task_definition::from_bson(doc);
      if (def.has_value()) {
         tasks.emplace_back(def.value());
      } else {
         spdlog::warn(std::format("Invalid task document in db: id='{}'", doc["_id"].get_oid().value.to_string()));
      }
   }

   return tasks;
}

std::vector<task_definition> Database::list_tasks_by_user(const dpp::snowflake& user_id) {
   auto client = pool.acquire();
   auto db = client[db_name];

   auto cursor = db[TASK_COL].find(make_document(
      kvp("owner_user_id", user_id.str())
   ));

   std::vector<task_definition> tasks;
   for (auto&& doc : cursor) {
      auto def = task_definition::from_bson(doc);
      if (def.has_value()) {
         tasks.emplace_back(def.value());
      } else {
         spdlog::warn(std::format("Invalid task document in db: id='{}'", doc["_id"].get_oid().value.to_string()));
      }
   }

   return tasks;
}

std::vector<task_definition> Database::find_tasks_by_name(const dpp::snowflake& user_id, const std::string &query) {
   auto client = pool.acquire();
   auto db = client[db_name];

   auto cursor = db[TASK_COL].find(make_document(
      kvp("owner_user_id", user_id.str()),
      kvp("$text", make_document(
         kvp("$search", query)
      ))
   ));

   std::vector<task_definition> tasks;
   for (auto&& doc : cursor) {
      auto def = task_definition::from_bson(doc);
      if (def.has_value()) {
         tasks.emplace_back(def.value());
      } else {
         spdlog::warn(std::format("Invalid task document in db: id='{}'", doc["_id"].get_oid().value.to_string()));
      }
   }
   
   return tasks;
}

bool Database::add_task(const task_definition& task) {
   auto client = pool.acquire();
   auto db = client[db_name];

   auto doc = task.to_bson();
   auto result = db[TASK_COL].insert_one(std::move(doc));

   return result.has_value() && result.value().inserted_id().type() == bsoncxx::type::k_oid ;
}

bool Database::delete_task(const dpp::snowflake& user_id, const std::string& task_name) {
   auto client = pool.acquire();
   auto db = client[db_name];

   auto result = db[TASK_COL].delete_one(make_document(
      kvp("owner_user_id", user_id.str()),
      kvp("name", task_name)
   ));

   return result.has_value() && result.value().deleted_count() > 0;
}

bool Database::complete_task(const dpp::snowflake& user_id, const std::string& task_name) {
   auto client = pool.acquire();
   auto db = client[db_name];

   auto result = db[TASK_COL].update_one(make_document(
      kvp("owner_user_id", user_id.str()),
      kvp("name", task_name)
   ), make_document(
      kvp("$set", make_document(
         kvp("last_completed", ymd_to_string(get_today_as_ymd()))
      ))
   ));

   return result.has_value() && result.value().modified_count() > 0;
}

std::optional<user_session> Database::get_session_by_cookie(const std::string& session_cookie) {
   auto client = pool.acquire();
   auto db = client[db_name];

   auto doc = db[USER_SESSION_COL].find_one(make_document(
      kvp("session_cookie", session_cookie)
   ));

   if (doc.has_value()) {
      return user_session::from_bson(doc.value());
   } else {
      return {};
   }
}

bool Database::add_session(const user_session& session) {
   auto client = pool.acquire();
   auto db = client[db_name];

   auto doc = session.to_bson();
   auto result = db[USER_SESSION_COL].insert_one(std::move(doc));

   return result.has_value() && result.value().inserted_id().type() == bsoncxx::type::k_oid;
}