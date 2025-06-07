#pragma once

#include <chrono>
#include <vector>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <mongocxx/pool.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <dpp/dpp.h>
#include <dpp/nlohmann/json.hpp>

#include "choretracker/utils.hpp"

#define DEFAULT_DB_NAME "choretracker"

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

struct chore_definition {
    dpp::snowflake owner_user_id;
    std::string name;
    int32_t frequency_days;
    std::chrono::year_month_day last_completed;

    // Computed values
    int32_t days_since_completed;
    int32_t days_overdue;

    auto to_bson() const {
        return make_document(
            kvp("owner_user_id", owner_user_id.str()),
            kvp("name", name),
            kvp("frequency_days", frequency_days),
            kvp("last_completed", ymd_to_string(last_completed))
        );
    }

    nlohmann::json to_json() const {
        return {
            { "owner_user_id", owner_user_id.str() },
            { "name", name },
            { "frequency_days", frequency_days },
            { "last_completed", ymd_to_string(last_completed) },
            { "days_since_completed", days_since_completed },
            { "days_overdue", days_overdue }
        };
    }
    
    static std::optional<chore_definition> from_bson(const bsoncxx::document::view& doc) {
        try {
            chore_definition chore;
            chore.owner_user_id = dpp::snowflake(bson_to_string(doc["owner_user_id"]));
            chore.name = bson_to_string(doc["name"]);
            chore.frequency_days = doc["frequency_days"].get_int32().value;
            chore.last_completed = parse_ymd(bson_to_string(doc["last_completed"])).value();
            // Computed values
            chore.days_since_completed = (std::chrono::sys_days(get_today_as_ymd()) - std::chrono::sys_days(chore.last_completed)).count();
            chore.days_overdue = chore.days_since_completed - chore.frequency_days;

            return chore;
        } catch (const std::exception&) {
            return {};
        }
    }
};

struct user_session {
    std::string user_id;
    std::string session_cookie;
    std::string user_name;
    std::string avatar;

    auto to_bson() const {
        return make_document(
            kvp("user_id", user_id),
            kvp("session_cookie", session_cookie),
            kvp("user_name", user_name),
            kvp("avatar", avatar)
        );
    }

    nlohmann::json to_json() const {
        return {
            { "user_id", user_id },
            { "session_cookie", session_cookie },
            { "user_name", user_name },
            { "avatar", avatar }
        };
    }
    
    static std::optional<user_session> from_bson(const bsoncxx::document::view& doc) {
        try {
            user_session session;
            session.user_id = bson_to_string(doc["user_id"]);
            session.session_cookie = bson_to_string(doc["session_cookie"]);
            session.user_name = bson_to_string(doc["user_name"]);
            session.avatar = bson_to_string(doc["avatar"]);

            return session;
        } catch (const std::exception&) {
            return {};
        }
    }
};

class Database {
    public:
        Database(const std::string& connection_uri, const std::string& db_name) : pool(mongocxx::uri(connection_uri)), db_name(db_name) {}

        std::vector<chore_definition> list_all_chores();
        std::vector<chore_definition> list_chores_by_user(const dpp::snowflake& user_id);
        bool add_chore(const chore_definition& chore);
        bool delete_chore(const dpp::snowflake& user_id, const std::string& chore_name);
        bool complete_chore(const dpp::snowflake& user_id, const std::string& chore_name);

        std::optional<user_session> get_session_by_cookie(const std::string& session_cookie);
        bool add_session(const user_session& session);
    private:
        mongocxx::instance instance;
        mongocxx::pool pool;
        std::string db_name;
};