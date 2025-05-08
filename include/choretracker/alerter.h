#pragma once

#include <thread>

#include "choretracker/db.h"

class Alerter {
   public:
      Alerter(Database& db) : db(db) {}

      void begin();
      void run_alerts();
   private:
      void thread_task();

      Database& db;
      std::thread thread;
};