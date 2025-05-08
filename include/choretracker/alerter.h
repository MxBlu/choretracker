#pragma once

#include <thread>
#include <dpp/dpp.h>

#include "choretracker/db.h"

class Alerter {
   public:
      Alerter(Database& db, dpp::cluster& bot) : db(db), bot(bot) {}

      void begin();
      void run_alerts();
   private:
      void thread_task();

      Database& db;
      dpp::cluster& bot;
      std::thread thread;
};