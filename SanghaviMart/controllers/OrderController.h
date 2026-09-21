#pragma once
#include <drogon/HttpController.h>

namespace sanghavimart::controllers {

class OrderController : public drogon::HttpController<OrderController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(OrderController::createOrder, "/api/orders", drogon::Post, "sanghavimart::filters::BuyerFilter");
    ADD_METHOD_TO(OrderController::getOrders, "/api/orders", drogon::Get, "sanghavimart::filters::AuthFilter");
    ADD_METHOD_TO(OrderController::getOrderById, "/api/orders/{id}", drogon::Get, "sanghavimart::filters::AuthFilter");
    ADD_METHOD_TO(OrderController::updateOrderStatus, "/api/orders/{id}/status", drogon::Put, "sanghavimart::filters::AuthFilter");
    METHOD_LIST_END

    void createOrder(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void getOrders(const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void getOrderById(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                      int id);

    void updateOrderStatus(const drogon::HttpRequestPtr& req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                           int id);
};

} // namespace sanghavimart::controllers
