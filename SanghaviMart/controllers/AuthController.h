#pragma once
#include <drogon/HttpController.h>

namespace sanghavimart::controllers {

class AuthController : public drogon::HttpController<AuthController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AuthController::registerUser, "/api/register", drogon::Post);
    ADD_METHOD_TO(AuthController::loginUser, "/api/login", drogon::Post);
    ADD_METHOD_TO(AuthController::logoutUser, "/api/logout", drogon::Post);
    ADD_METHOD_TO(AuthController::getCurrentUser, "/api/auth/me", drogon::Get, "sanghavimart::filters::AuthFilter");
    METHOD_LIST_END

    void registerUser(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void loginUser(const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void logoutUser(const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void getCurrentUser(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

} // namespace sanghavimart::controllers
