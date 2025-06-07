#pragma once

#include <crow.h>
#include <crow/middlewares/cors.h>
#include <crow/middlewares/cookie_parser.h>
#include <optional>

#include "choretracker/db.h"
#include "choretracker/discord_oauth.h"

#define DEFAULT_WEB_PORT 8080
#define DEFAULT_WEB_BASE_URL "http://localhost:" STRINGIFY(DEFAULT_WEB_PORT)

class Web {
    public:
        Web(int port, const std::string& base_url, const std::string& client_id, 
                const std::string& client_secret, Database& db) 
                : oauth(client_id, client_secret, base_url + "/auth/callback"), db(db) {
            init(base_url, port);
        }
        ~Web();

        std::future<void> running_future;
    private:
        void init(const std::string& base_url, int port);
        std::optional<user_session> check_auth(const crow::request& req);

        crow::response auth_callback(const crow::request& req, const std::string& code);
        crow::response user_get(const user_session& user_session);
        crow::response chores_list(const std::string& user_id);
        crow::response chores_add(const std::string& user_id, const std::string& chore_name, int32_t chore_frequency);
        crow::response chores_delete(const std::string& user_id, const std::string& chore_name);
        crow::response chores_complete(const std::string& user_id, const std::string& chore_name);

        crow::App<crow::CORSHandler, crow::CookieParser> server;
        DiscordOAuth oauth;
        Database& db;

};