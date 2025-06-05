#pragma once

#include <dpp/dpp.h>

#include "choretracker/db.h"
#include "choretracker/alerter.h"

class Bot {
    public:
        Bot(const std::string& bot_token, Database& db) : cluster(bot_token), db(db), alerter(db, cluster) {
            init();
        }

        void waitForExit();
    private:
        void init();

        dpp::cluster cluster;
        Database& db;
        Alerter alerter;
};