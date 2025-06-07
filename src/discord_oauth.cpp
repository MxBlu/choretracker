#include <httplib.h>
#include <spdlog/spdlog.h>
#include <format>

#include "choretracker/discord_oauth.h"

#define DISCORD_API_BASE_URL "https://discord.com"

std::string DiscordOAuth::generate_authorize_url() {
    return std::format(
        "https://discord.com/api/oauth2/authorize?client_id={}&redirect_uri={}&response_type=code&scope=identify", 
        client_id, redirect_uri);
}

std::optional<nlohmann::json> DiscordOAuth::exchange_code_for_token(const std::string& code) {
    httplib::Client client(DISCORD_API_BASE_URL);
    httplib::Params params;
    params.emplace("client_id", client_id);
    params.emplace("client_secret", client_secret);
    params.emplace("grant_type", "authorization_code");
    params.emplace("code", code);
    params.emplace("redirect_uri", redirect_uri);

    auto res = client.Post("/api/oauth2/token", params);
    if (res && res->status == httplib::StatusCode::OK_200) {
        return nlohmann::json::parse(res->body);
    } else {
        if (!res) {
            spdlog::error(std::format("Exception exchanging code: {}", httplib::to_string(res.error())));
        } else {
            spdlog::error(std::format("Exception exchanging code: {}", res->body));
        }
        return {};
    }
}

std::optional<nlohmann::json> DiscordOAuth::get_user_info(const std::string& access_token) {
    httplib::Client client(DISCORD_API_BASE_URL);
    httplib::Headers headers;
    headers.emplace("Authorization", std::format("Bearer {}", access_token));

    auto res = client.Get("/api/users/@me", headers);
    if (res && res->status == httplib::StatusCode::OK_200) {
        return nlohmann::json::parse(res->body);
    } else {
        if (!res) {
            spdlog::error(std::format("Exception getting user info: {}", httplib::to_string(res.error())));
        } else {
            spdlog::error(std::format("Exception getting user info: {}", res->body));
        }
        return {};
    }
}