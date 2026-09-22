#include "AuthV1Controller.h"
#include <cctype>
#include <drogon/drogon.h>
#include "../repository/PostgresRepositories.h"
#include "../service/Services.h"
#include "../util/JsonUtil.h"
#include "../util/PasswordUtil.h"
#include "../util/ValidationUtil.h"

namespace sanghavimart::controller {

static drogon::orm::DbClientPtr Db() { return drogon::app().getDbClient(); }

static void SendError(std::function<void(const drogon::HttpResponsePtr&)> cb,
                      const std::string& code, const std::string& msg,
                      drogon::HttpStatusCode st) {
    auto r = drogon::HttpResponse::newHttpJsonResponse(util::JsonUtil::Error(code, msg));
    r->setStatusCode(st);
    cb(r);
}

void AuthV1Controller::Register(const drogon::HttpRequestPtr& req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    auto j = req->getJsonObject();
    if (!j) return SendError(cb, "VALIDATION_ERROR", "Invalid JSON.", drogon::k400BadRequest);
    try {
        std::string name = (*j).get("name", "").asString();
        std::string email = (*j).get("email", "").asString();
        std::string password = (*j).get("password", "").asString();
        std::string role = (*j).get("role", "BUYER").asString();
        for (auto& c : role) c = (char)toupper(c);
        service::AuthService svc2(std::shared_ptr<repository::IUserRepository>(
            new repository::PostgresUserRepository(Db())));
        auto u = svc2.Register(name, email, password, role);
        // Fresh session (fixation protection): drop pre-auth data + rotate ID.
        req->session()->clear();
        req->session()->insert("user_id", u.id);
        req->session()->insert("user_role", u.role);
        req->session()->changeSessionIdToClient();
        Json::Value d;
        d["id"] = u.id;
        d["name"] = u.name;
        d["email"] = u.email;
        d["role"] = u.role;
        auto r = drogon::HttpResponse::newHttpJsonResponse(util::JsonUtil::Success(d));
        r->setStatusCode(drogon::k201Created);
        cb(r);
    } catch (const exception::ValidationException& e) {
        SendError(cb, e.Code(), e.PublicMessage(), drogon::k400BadRequest);
    } catch (const exception::ConflictException& e) {
        SendError(cb, e.Code(), e.PublicMessage(), drogon::k409Conflict);
    } catch (const std::exception& e) {
        LOG_ERROR << "register failed: " << e.what();
        SendError(cb, "INTERNAL_ERROR", "Registration failed.", drogon::k500InternalServerError);
    }
}

void AuthV1Controller::Login(const drogon::HttpRequestPtr& req,
                             std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    auto j = req->getJsonObject();
    if (!j) return SendError(cb, "VALIDATION_ERROR", "Invalid JSON.", drogon::k400BadRequest);
    try {
        std::string email = (*j).get("email", "").asString();
        std::string password = (*j).get("password", "").asString();
        service::AuthService svc(std::shared_ptr<repository::IUserRepository>(
            new repository::PostgresUserRepository(Db())));
        auto u = svc.Authenticate(email, password);
        req->session()->clear();  // discard pre-auth session
        req->session()->insert("user_id", u.id);
        req->session()->insert("user_role", u.role);
        req->session()->changeSessionIdToClient();  // rotate session ID
        Json::Value d;
        d["id"] = u.id;
        d["name"] = u.name;
        d["email"] = u.email;
        d["role"] = u.role;
        cb(drogon::HttpResponse::newHttpJsonResponse(util::JsonUtil::Success(d)));
    } catch (const exception::UnauthorizedException& e) {
        SendError(cb, e.Code(), e.PublicMessage(), drogon::k401Unauthorized);
    } catch (const std::exception& e) {
        LOG_ERROR << "login failed: " << e.what();
        SendError(cb, "INTERNAL_ERROR", "Login failed.", drogon::k500InternalServerError);
    }
}

void AuthV1Controller::Logout(const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    req->session()->clear();
    Json::Value d;
    d["message"] = "Logged out.";
    cb(drogon::HttpResponse::newHttpJsonResponse(util::JsonUtil::Success(d)));
}

void AuthV1Controller::Me(const drogon::HttpRequestPtr& req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    if (!req->session()->find("user_id")) {
        return SendError(cb, "UNAUTHORIZED", "Authentication required.",
                         drogon::k401Unauthorized);
    }
    Json::Value d;
    d["id"] = req->session()->get<int>("user_id");
    d["role"] = req->session()->get<std::string>("user_role");
    cb(drogon::HttpResponse::newHttpJsonResponse(util::JsonUtil::Success(d)));
}

}  // namespace sanghavimart::controller
