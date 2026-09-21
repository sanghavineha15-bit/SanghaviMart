#include "AuthController.h"
#include "../utils/PasswordUtil.h"
#include "../utils/JwtUtil.h"
#include <regex>

namespace sanghavimart::controllers {

void AuthController::registerUser(const drogon::HttpRequestPtr& req,
                                  std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto json = req->getJsonObject();
    if (!json) {
        Json::Value err;
        err["success"] = false;
        err["error"] = "Invalid JSON payload in request.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    std::string name = (*json)["name"].asString();
    std::string email = (*json)["email"].asString();
    std::string password = (*json)["password"].asString();
    std::string role = (*json)["role"].asString(); // "buyer" or "seller"
    std::string phone = (*json).get("phone", "").asString();
    std::string address = (*json).get("address", "").asString();

    // Input Validation
    if (name.empty() || email.empty() || password.empty() || role.empty()) {
        Json::Value err;
        err["success"] = false;
        err["error"] = "Name, email, password, and role are required fields.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    if (role != "buyer" && role != "seller") {
        Json::Value err;
        err["success"] = false;
        err["error"] = "Role must be either 'buyer' or 'seller'. Admin accounts cannot be registered publicly.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    if (password.length() < 6) {
        Json::Value err;
        err["success"] = false;
        err["error"] = "Password must be at least 6 characters long.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    const std::regex emailPattern(R"(^[\w\.-]+@[\w\.-]+\.\w+$)");
    if (!std::regex_match(email, emailPattern)) {
        Json::Value err;
        err["success"] = false;
        err["error"] = "Please provide a valid email address.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    auto dbClient = drogon::app().getDbClient();
    std::string passwordHash = utils::PasswordUtil::hashPassword(password);

    // Check if email already exists
    dbClient->execSqlAsync(
        "SELECT id FROM users WHERE email = $1",
        [dbClient, name, email, passwordHash, role, phone, address, callback](const drogon::orm::Result& result) {
            if (!result.empty()) {
                Json::Value err;
                err["success"] = false;
                err["error"] = "An account with this email address already exists.";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
                resp->setStatusCode(drogon::k409Conflict);
                callback(resp);
                return;
            }

            // Insert new user
            dbClient->execSqlAsync(
                "INSERT INTO users (name, email, password_hash, role, phone, address) "
                "VALUES ($1, $2, $3, $4, $5, $6) RETURNING id, name, email, role",
                [callback, role, dbClient](const drogon::orm::Result& insertResult) {
                    if (insertResult.empty()) {
                        Json::Value err;
                        err["success"] = false;
                        err["error"] = "Failed to register user account.";
                        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
                        resp->setStatusCode(drogon::k500InternalServerError);
                        callback(resp);
                        return;
                    }

                    int userId = insertResult[0]["id"].as<int>();
                    std::string userName = insertResult[0]["name"].as<std::string>();
                    std::string userEmail = insertResult[0]["email"].as<std::string>();
                    std::string userRole = insertResult[0]["role"].as<std::string>();

                    // If buyer, create their initial cart
                    if (userRole == "buyer") {
                        dbClient->execSqlAsync(
                            "INSERT INTO cart (buyer_id) VALUES ($1) ON CONFLICT (buyer_id) DO NOTHING",
                            [](const drogon::orm::Result&) {},
                            [](const drogon::orm::DrogonDbException&) {}
                        );
                    }

                    std::string token = utils::JwtUtil::generateToken(userId, userEmail, userName, userRole);

                    Json::Value res;
                    res["success"] = true;
                    res["message"] = "User registered successfully.";
                    res["token"] = token;
                    res["user"]["id"] = userId;
                    res["user"]["name"] = userName;
                    res["user"]["email"] = userEmail;
                    res["user"]["role"] = userRole;

                    auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
                    resp->setStatusCode(drogon::k201Created);
                    callback(resp);
                },
                [callback](const drogon::orm::DrogonDbException& e) {
                    Json::Value err;
                    err["success"] = false;
                    err["error"] = std::string("Database error: ") + e.base().what();
                    auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
                    resp->setStatusCode(drogon::k500InternalServerError);
                    callback(resp);
                },
                name, email, passwordHash, role, phone, address
            );
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            Json::Value err;
            err["success"] = false;
            err["error"] = std::string("Database query error: ") + e.base().what();
            auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
        },
        email
    );
}

void AuthController::loginUser(const drogon::HttpRequestPtr& req,
                               std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto json = req->getJsonObject();
    if (!json) {
        Json::Value err;
        err["success"] = false;
        err["error"] = "Invalid JSON payload in request.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    std::string email = (*json)["email"].asString();
    std::string password = (*json)["password"].asString();

    if (email.empty() || password.empty()) {
        Json::Value err;
        err["success"] = false;
        err["error"] = "Email and password are required.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    auto dbClient = drogon::app().getDbClient();
    dbClient->execSqlAsync(
        "SELECT id, name, email, password_hash, role, phone, address FROM users WHERE email = $1",
        [password, callback](const drogon::orm::Result& result) {
            if (result.empty()) {
                Json::Value err;
                err["success"] = false;
                err["error"] = "Invalid email or password credentials.";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
                resp->setStatusCode(drogon::k401Unauthorized);
                callback(resp);
                return;
            }

            std::string storedHash = result[0]["password_hash"].as<std::string>();
            if (!utils::PasswordUtil::verifyPassword(password, storedHash)) {
                Json::Value err;
                err["success"] = false;
                err["error"] = "Invalid email or password credentials.";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
                resp->setStatusCode(drogon::k401Unauthorized);
                callback(resp);
                return;
            }

            int userId = result[0]["id"].as<int>();
            std::string userName = result[0]["name"].as<std::string>();
            std::string userEmail = result[0]["email"].as<std::string>();
            std::string userRole = result[0]["role"].as<std::string>();

            std::string token = utils::JwtUtil::generateToken(userId, userEmail, userName, userRole);

            Json::Value res;
            res["success"] = true;
            res["message"] = "Login successful. Welcome back to SanghaviMart!";
            res["token"] = token;
            res["user"]["id"] = userId;
            res["user"]["name"] = userName;
            res["user"]["email"] = userEmail;
            res["user"]["role"] = userRole;
            res["user"]["phone"] = result[0]["phone"].isNull() ? "" : result[0]["phone"].as<std::string>();
            res["user"]["address"] = result[0]["address"].isNull() ? "" : result[0]["address"].as<std::string>();

            auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
            resp->setStatusCode(drogon::k200OK);
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            Json::Value err;
            err["success"] = false;
            err["error"] = std::string("Database error during authentication: ") + e.base().what();
            auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
        },
        email
    );
}

void AuthController::logoutUser(const drogon::HttpRequestPtr&,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    Json::Value res;
    res["success"] = true;
    res["message"] = "Logged out successfully. Client should discard token.";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
    resp->setStatusCode(drogon::k200OK);
    callback(resp);
}

void AuthController::getCurrentUser(const drogon::HttpRequestPtr& req,
                                   std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    int userId = req->attributes()->get<int>("user_id");
    auto dbClient = drogon::app().getDbClient();

    dbClient->execSqlAsync(
        "SELECT id, name, email, role, phone, address, created_at FROM users WHERE id = $1",
        [callback](const drogon::orm::Result& result) {
            if (result.empty()) {
                Json::Value err;
                err["success"] = false;
                err["error"] = "User profile not found.";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
                resp->setStatusCode(drogon::k404NotFound);
                callback(resp);
                return;
            }

            Json::Value res;
            res["success"] = true;
            res["user"]["id"] = result[0]["id"].as<int>();
            res["user"]["name"] = result[0]["name"].as<std::string>();
            res["user"]["email"] = result[0]["email"].as<std::string>();
            res["user"]["role"] = result[0]["role"].as<std::string>();
            res["user"]["phone"] = result[0]["phone"].isNull() ? "" : result[0]["phone"].as<std::string>();
            res["user"]["address"] = result[0]["address"].isNull() ? "" : result[0]["address"].as<std::string>();

            auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            Json::Value err;
            err["success"] = false;
            err["error"] = std::string("Database error: ") + e.base().what();
            auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
        },
        userId
    );
}

} // namespace sanghavimart::controllers
