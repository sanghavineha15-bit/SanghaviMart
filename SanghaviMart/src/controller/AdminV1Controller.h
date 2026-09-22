#pragma once
#include <drogon/HttpController.h>

namespace sanghavimart::controller {

// Spec §22: admin-only user/order/listing oversight. Every route 403s
// non-admins via RequireSession(ADMIN).
class AdminV1Controller : public drogon::HttpController<AdminV1Controller> {
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AdminV1Controller::Users, "/api/v1/admin/users", drogon::Get);
    ADD_METHOD_TO(AdminV1Controller::Orders, "/api/v1/admin/orders", drogon::Get);
    ADD_METHOD_TO(AdminV1Controller::Stats, "/api/v1/admin/stats", drogon::Get);
    ADD_METHOD_TO(AdminV1Controller::RemoveProduct, "/api/v1/admin/products/{id}",
                  drogon::Delete);
    METHOD_LIST_END

    void Users(const drogon::HttpRequestPtr& req,
               std::function<void(const drogon::HttpResponsePtr&)>&& cb);
    void Orders(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& cb);
    void Stats(const drogon::HttpRequestPtr& req,
               std::function<void(const drogon::HttpResponsePtr&)>&& cb);
    void RemoveProduct(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& cb, int id);
};

}  // namespace sanghavimart::controller
