#pragma once
#include <drogon/HttpController.h>

namespace sanghavimart::controller {

// Spec §18/20: transactional checkout (mock payment) + role-scoped orders +
// server-validated status workflow PENDING->CONFIRMED->SHIPPED->DELIVERED.
class OrderV1Controller : public drogon::HttpController<OrderV1Controller> {
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(OrderV1Controller::Checkout, "/api/v1/orders/checkout", drogon::Post);
    ADD_METHOD_TO(OrderV1Controller::List, "/api/v1/orders", drogon::Get);
    ADD_METHOD_TO(OrderV1Controller::Get, "/api/v1/orders/{id}", drogon::Get);
    ADD_METHOD_TO(OrderV1Controller::SetStatus, "/api/v1/orders/{id}/status", drogon::Put);
    METHOD_LIST_END

    void Checkout(const drogon::HttpRequestPtr& req,
                  std::function<void(const drogon::HttpResponsePtr&)>&& cb);
    void List(const drogon::HttpRequestPtr& req,
              std::function<void(const drogon::HttpResponsePtr&)>&& cb);
    void Get(const drogon::HttpRequestPtr& req,
             std::function<void(const drogon::HttpResponsePtr&)>&& cb, int id);
    void SetStatus(const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& cb, int id);
};

}  // namespace sanghavimart::controller
