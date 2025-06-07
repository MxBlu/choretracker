#include <format>
#include <bsoncxx/builder/stream/document.hpp>
#include <spdlog/spdlog.h>

#include "choretracker/db.h"
#include "choretracker/utils.hpp"

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

#define CHORE_COL "chores"
#define USER_SESSION_COL "user_sessions"

std::vector<chore_definition> Database::list_all_chores() {
   auto client = pool.acquire();
   auto db = client[db_name];

   auto cursor = db[CHORE_COL].find({});

   std::vector<chore_definition> chores;
   for (auto&& doc : cursor) {
      auto def = chore_definition::from_bson(doc);
      if (def.has_value()) {
         chores.emplace_back(def.value());
      } else {
         spdlog::warn(std::format("Invalid chore document in db: id='{}'", doc["_id"].get_oid().value.to_string()));
      }
   }

   return chores;
}

std::vector<chore_definition> Database::list_chores_by_user(const dpp::snowflake& user_id) {
   auto client = pool.acquire();
   auto db = client[db_name];

   auto cursor = db[CHORE_COL].find(make_document(
      kvp("owner_user_id", user_id.str())
   ));

   std::vector<chore_definition> chores;
   for (auto&& doc : cursor) {
      auto def = chore_definition::from_bson(doc);
      if (def.has_value()) {
         chores.emplace_back(def.value());
      } else {
         spdlog::warn(std::format("Invalid chore document in db: id='{}'", doc["_id"].get_oid().value.to_string()));
      }
   }

   return chores;
}

bool Database::add_chore(const chore_definition& chore) {
   auto client = pool.acquire();
   auto db = client[db_name];

   auto doc = chore.to_bson();
   auto result = db[CHORE_COL].insert_one(std::move(doc));

   return result.has_value() && result.value().inserted_id().type() == bsoncxx::type::k_oid ;
}

bool Database::delete_chore(const dpp::snowflake& user_id, const std::string& chore_name) {
   auto client = pool.acquire();
   auto db = client[db_name];

   auto result = db[CHORE_COL].delete_one(make_document(
      kvp("owner_user_id", user_id.str()),
      kvp("name", chore_name)
   ));

   return result.has_value() && result.value().deleted_count() > 0;
}

bool Database::complete_chore(const dpp::snowflake& user_id, const std::string& chore_name) {
   auto client = pool.acquire();
   auto db = client[db_name];

   auto result = db[CHORE_COL].update_one(make_document(
      kvp("owner_user_id", user_id.str()),
      kvp("name", chore_name)
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