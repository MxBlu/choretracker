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

enum task_type {
    regular = 0
};

struct task_definition {
    dpp::snowflake owner_user_id;
    std::string name;
    task_type type;
    int32_t frequency_days;
    std::chrono::year_month_day last_completed;

    // Computed values
    int32_t days_since_completed;
    int32_t days_overdue;

    auto to_bson() const {
        return make_document(
            kvp("owner_user_id", owner_user_id.str()),
            kvp("name", name),
            kvp("type", type),
            kvp("frequency_days", frequency_days),
            kvp("last_completed", ymd_to_string(last_completed))
        );
    }

    nlohmann::json to_json() const {
        return {
            { "owner_user_id", owner_user_id.str() },
            { "name", name },
            { "type", type },
            { "frequency_days", frequency_days },
            { "last_completed", ymd_to_string(last_completed) },
            { "days_since_completed", days_since_completed },
            { "days_overdue", days_overdue }
        };
    }
    
    static std::optional<task_definition> from_bson(const bsoncxx::document::view& doc) {
        try {
            task_definition task;
            task.owner_user_id = dpp::snowflake(bson_to_string(doc["owner_user_id"]));
            task.name = bson_to_string(doc["name"]);
            task.type = static_cast<task_type>(doc["type"].get_int32().value);
            task.frequency_days = doc["frequency_days"].get_int32().value;
            task.last_completed = parse_ymd(bson_to_string(doc["last_completed"])).value();
            // Computed values
            task.days_since_completed = (std::chrono::sys_days(get_today_as_ymd()) - std::chrono::sys_days(task.last_completed)).count();
            task.days_overdue = task.days_since_completed - task.frequency_days;

            return task;
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

        std::vector<task_definition> list_all_tasks();
        std::vector<task_definition> list_tasks_by_user(const dpp::snowflake& user_id);
        std::vector<task_definition> find_tasks_by_name(const dpp::snowflake& user_id, const std::string &query);
        bool add_task(const task_definition& task);
        bool delete_task(const dpp::snowflake& user_id, const std::string& task_name);
        bool complete_task(const dpp::snowflake& user_id, const std::string& task_name);

        std::optional<user_session> get_session_by_cookie(const std::string& session_cookie);
        bool add_session(const user_session& session);
    private:
        mongocxx::instance instance;
        mongocxx::pool pool;
        std::string db_name;
};