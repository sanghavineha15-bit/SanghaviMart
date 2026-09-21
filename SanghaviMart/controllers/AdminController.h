#pragma once
#include <drogon/HttpController.h>

namespace sanghavimart::controllers {

class AdminController : public drogon::HttpController<AdminController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AdminController::getAllUsers, "/api/admin/users", drogon::Get, "sanghavimart::filters::AdminFilter");
    ADD_METHOD_TO(AdminController::getAllProducts, "/api/admin/products", drogon::Get, "sanghavimart::filters::AdminFilter");
    ADD_METHOD_TO(AdminController::getAllOrders, "/api/admin/orders", drogon::Get, "sanghavimart::filters::AdminFilter");
    ADD_METHOD_TO(AdminController::deleteProduct, "/api/admin/products/{id}", drogon::Delete, "sanghavimart::filters::AdminFilter");
    ADD_METHOD_TO(AdminController::getPlatformStats, "/api/admin/stats", drogon::Get, "sanghavimart::filters::AdminFilter");
    METHOD_LIST_END

    void getAllUsers(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void getAllProducts(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void getAllOrders(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void deleteProduct(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                       int id);

    void getPlatformStats(const drogon::HttpRequestPtr& req,
                          std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

} // namespace sanghavimart::controllers
