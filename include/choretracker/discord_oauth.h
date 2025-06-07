#pragma once

#include <dpp/nlohmann/json.hpp>
#include <string>

class DiscordOAuth {
    public:
        DiscordOAuth(const std::string& client_id, const std::string& client_secret, const std::string& redirect_uri) 
            : client_id(client_id), client_secret(client_secret), redirect_uri(redirect_uri) {}

        std::string generate_authorize_url();
        std::optional<nlohmann::json> exchange_code_for_token(const std::string& code);
        std::optional<nlohmann::json> get_user_info(const std::string& access_token);
    private:
        std::string client_id;
        std::string client_secret;
        std::string redirect_uri;
};