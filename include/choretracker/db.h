#pragma once

#include <chrono>
#include <vector>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <mongocxx/pool.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <dpp/dpp.h>

#include "choretracker/utils.hpp"

#define DEFAULT_DB_NAME "choretracker"

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

struct chore_definition {
    dpp::snowflake owner_user_id;
    std::string name;
    int32_t frequency_days;
    std::chrono::year_month_day last_completed;

    auto to_bson() const {
        return make_document(
            kvp("owner_user_id", owner_user_id.str()),
            kvp("name", name),
            kvp("frequency_days", frequency_days),
            kvp("last_completed", ymd_to_string(last_completed))
        );
    }
    
    static std::optional<chore_definition> from_bson(const bsoncxx::document::view& doc) {
        try {
            chore_definition chore;
            chore.owner_user_id = dpp::snowflake(bson_to_string(doc["owner_user_id"]));
            chore.name = bson_to_string(doc["name"]);
            chore.frequency_days = doc["frequency_days"].get_int32().value;
            chore.last_completed = parse_ymd(bson_to_string(doc["last_completed"])).value();

            return chore;
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
    private:
        mongocxx::instance instance;
        mongocxx::pool pool;
        std::string db_name;
};