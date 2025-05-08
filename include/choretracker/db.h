#pragma once

#include <chrono>
#include <unordered_map>
#include <mutex>
#include <span>
#include <vector>

#include <dpp/dpp.h>

// TODO: Thread safety

struct user_identifier {
    dpp::snowflake user_id;
    dpp::snowflake guild_id;

    bool operator==(const user_identifier &other) const { 
        return user_id == other.user_id && guild_id == other.guild_id;
    }
};

template<>
struct std::hash<user_identifier> {
  std::size_t operator()(const user_identifier& k) const
  {
    return (std::hash<dpp::snowflake>()(k.user_id)
             ^ (std::hash<dpp::snowflake>()(k.guild_id) << 1)) >> 1;
  }
};

struct chore_definition {
    user_identifier owner;
    std::string name;
    int frequency_days;
    std::chrono::year_month_day last_completed;
};

class Database {
    public:
        Database() {};

        std::span<chore_definition> list_all_chores();
        std::vector<chore_definition> list_chores_by_user(user_identifier user_identifier);
        void add_chore(chore_definition chore);
        bool delete_chore(user_identifier user_identifier, std::string chore_name);
        void reset_chore(user_identifier user_identifier, std::string chore_name);
    private:
        std::vector<chore_definition> db;
        std::mutex db_mutex;
};