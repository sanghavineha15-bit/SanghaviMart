#pragma once
#include <drogon/HttpController.h>

namespace sanghavimart::controller {

// Session-based auth (/api/v1/auth/*). Admin cannot register.
class AuthV1Controller : public drogon::HttpController<AuthV1Controller> {
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AuthV1Controller::Register, "/api/v1/auth/register", drogon::Post, "sanghavimart::filter::LoggingFilter");
    ADD_METHOD_TO(AuthV1Controller::Login, "/api/v1/auth/login", drogon::Post, "sanghavimart::filter::LoggingFilter");
    ADD_METHOD_TO(AuthV1Controller::Logout, "/api/v1/auth/logout", drogon::Post, "sanghavimart::filter::LoggingFilter");
    ADD_METHOD_TO(AuthV1Controller::Me, "/api/v1/auth/me", drogon::Get, "sanghavimart::filter::LoggingFilter");
    METHOD_LIST_END

    void Register(const drogon::HttpRequestPtr& req,
                  std::function<void(const drogon::HttpResponsePtr&)>&& cb);
    void Login(const drogon::HttpRequestPtr& req,
               std::function<void(const drogon::HttpResponsePtr&)>&& cb);
    void Logout(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& cb);
    void Me(const drogon::HttpRequestPtr& req,
            std::function<void(const drogon::HttpResponsePtr&)>&& cb);
};

}  // namespace sanghavimart::controller
