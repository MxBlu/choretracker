#pragma once

#include <chrono>
#include <unordered_map>
#include <mutex>
#include <span>
#include <vector>

#include <dpp/dpp.h>

struct chore_definition {
    dpp::snowflake owner_user_id;
    std::string name;
    int frequency_days;
    std::chrono::year_month_day last_completed;
};

class Database {
    public:
        Database() {};

        std::span<chore_definition> list_all_chores();
        std::vector<chore_definition> list_chores_by_user(dpp::snowflake user_id);
        void add_chore(chore_definition chore);
        bool delete_chore(dpp::snowflake user_id, std::string chore_name);
        void reset_chore(dpp::snowflake user_id, std::string chore_name);
    private:
        std::vector<chore_definition> db;
        std::mutex db_mutex;
};