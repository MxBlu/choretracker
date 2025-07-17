#include <dpp/nlohmann/json.hpp>
#include <uuid.h>

#include "choretracker/web.h"

std::string generate_session_token();

void Web::init(const std::string& base_url, int port) {
    server.get_middleware<crow::CORSHandler>().global()
        .methods("GET"_method, "POST"_method, "PUT"_method, "DELETE"_method)
        .origin(base_url)
        .allow_credentials();

    /* Static endpoint */

    CROW_ROUTE(server, "/")
    ([this](const crow::request& req) {
        if (!check_auth(req)) {
            crow::response res(302);
            res.set_header("Location", "/auth/login");
            return res;
        }

        crow::response res;
        res.set_static_file_info("web/index.html");
        return res;
    });

    /* Auth endpoints */

    CROW_ROUTE(server, "/auth/login")
    ([this](const crow::request& req) {
        crow::response res(302);

        if (check_auth(req)) {
            res.set_header("Location", "/");
        } else {
            res.set_header("Location", oauth.generate_authorize_url());
        }

        return res;
    });

    CROW_ROUTE(server, "/auth/callback")
    ([this](const crow::request& req) {
        std::string code = req.url_params.get("code");

        return auth_callback(req, code);
    });

    /* API endpoints */

    CROW_ROUTE(server, "/api/user")
    ([this](const crow::request& req) {
        auto user_session = check_auth(req);
        if (!user_session) {
            crow::response res(302);
            res.set_header("Location", "/auth/login");
            return res;
        }

        return user_get(user_session.value());
    });

    CROW_ROUTE(server, "/api/tasks")
    ([this](const crow::request& req) {
        auto user_session = check_auth(req);
        if (!user_session) {
            crow::response res(302);
            res.set_header("Location", "/auth/login");
            return res;
        }
        
        return tasks_list(user_session.value().user_id);
    });

    CROW_ROUTE(server, "/api/tasks").methods("POST"_method)
    ([this](const crow::request& req) {
        auto user_session = check_auth(req);
        if (!user_session) {
            crow::response res(302);
            res.set_header("Location", "/auth/login");
            return res;
        }

        std::string task_name;
        int32_t task_frequency;
        try {
            auto body_json = nlohmann::json::parse(req.body);
            task_name = body_json["task_name"];
            task_frequency = body_json["task_frequency"];
        } catch (const std::exception&) {
            return crow::response(400);
        }

        return tasks_add(user_session.value().user_id, task_name, task_frequency);
    });

    CROW_ROUTE(server, "/api/tasks/<string>/complete").methods("PUT"_method)
    ([this](const crow::request& req, const std::string& task_name) {
        auto user_session = check_auth(req);
        if (!user_session) {
            crow::response res(302);
            res.set_header("Location", "/auth/login");
            return res;
        }

        std::string decoded_task_name = uri_decode(task_name);
        return tasks_complete(user_session.value().user_id, decoded_task_name);
    });

    CROW_ROUTE(server, "/api/tasks/<string>").methods("DELETE"_method)
    ([this](const crow::request& req, const std::string& task_name) {
        auto user_session = check_auth(req);
        if (!user_session) {
            crow::response res(302);
            res.set_header("Location", "/auth/login");
            return res;
        }

        std::string decoded_task_name = uri_decode(task_name);
        return tasks_delete(user_session.value().user_id, decoded_task_name);
    });

    running_future = server.port(port).multithreaded().run_async();
    spdlog::info("Web server started");
}

Web::~Web() {
    server.stop();
}

std::optional<user_session> Web::check_auth(const crow::request& req) {
    auto& cookie_ctx = server.get_context<crow::CookieParser>(req);
    
    auto auth_cookie = cookie_ctx.get_cookie("session_id");
    if (auth_cookie.empty()) {
        return {};
    }

    auto user_session = db.get_session_by_cookie(auth_cookie);
    if (!user_session.has_value()) {
        return {};
    }

    return user_session.value();
}

crow::response Web::auth_callback(const crow::request& req, const std::string& code) {
    auto& cookie_ctx = server.get_context<crow::CookieParser>(req);

    auto token_resp = oauth.exchange_code_for_token(code);
    if (!token_resp) {
        return crow::response(500);
    }

    auto user_info = oauth.get_user_info(token_resp.value()["access_token"]);
    if (!user_info) {
        return crow::response(500);
    }

    user_session user_session;
    user_session.user_id = user_info.value()["id"];
    user_session.user_name = user_info.value()["username"];
    user_session.avatar = user_info.value()["avatar"];
    user_session.session_cookie = generate_session_token();
    if (!db.add_session(user_session)) {
        return crow::response(500);
    }

    cookie_ctx.set_cookie("session_id", user_session.session_cookie)
        .path("/")
        .max_age(2592000ll) // 30 days
        .same_site(crow::CookieParser::Cookie::SameSitePolicy::Lax)
        .httponly();

    crow::response res(302);
    res.set_header("Location", "/");
    return res;
}

crow::response Web::user_get(const user_session& user_session) {
    auto json_user = user_session.to_json();
    json_user.erase("session_cookie");

    return crow::response(200, "application/json", json_user.dump());
}

crow::response Web::tasks_list(const std::string& user_id) {
    nlohmann::json resp_json;
    resp_json["tasks"] =  nlohmann::json::array();

    for (auto task : db.list_tasks_by_user(user_id)) {
        resp_json["tasks"].push_back(task.to_json());
    }

    return crow::response(200, "application/json", resp_json.dump());
}

crow::response Web::tasks_add(const std::string& user_id, const std::string& task_name, int32_t task_frequency) {
    task_definition task;
    task.owner_user_id = user_id;
    task.name = task_name;
    task.last_completed = get_today_as_ymd();
    task.frequency_days = task_frequency;

    if (db.add_task(task)) {
        return crow::response(200, "application/json", task.to_json().dump());
    } else {
        return crow::response(400, "Already exists");
    }
}

crow::response Web::tasks_delete(const std::string& user_id, const std::string& task_name) {
    if (db.delete_task(user_id, task_name)) {
        return crow::response(200);
    } else {
        return crow::response(404, "Not found");
    }
}

crow::response Web::tasks_complete(const std::string& user_id, const std::string& task_name) {
    if (db.complete_task(user_id, task_name)) {
        return crow::response(200);
    } else {
        return crow::response(404, "Not found");
    }
}

std::string generate_session_token() {
    std::random_device rd;
    auto seed_data = std::array<int, std::mt19937::state_size> {};
    std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
    std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
    std::mt19937 generator(seq);
    uuids::uuid_random_generator gen{generator};

    return uuids::to_string(gen());
}