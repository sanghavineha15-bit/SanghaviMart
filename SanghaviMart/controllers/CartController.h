#pragma once
#include <drogon/HttpController.h>

namespace sanghavimart::controllers {

class CartController : public drogon::HttpController<CartController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(CartController::getCart, "/api/cart", drogon::Get, "sanghavimart::filters::BuyerFilter");
    ADD_METHOD_TO(CartController::addToCart, "/api/cart", drogon::Post, "sanghavimart::filters::BuyerFilter");
    ADD_METHOD_TO(CartController::updateCartItem, "/api/cart/{id}", drogon::Put, "sanghavimart::filters::BuyerFilter");
    ADD_METHOD_TO(CartController::removeCartItem, "/api/cart/{id}", drogon::Delete, "sanghavimart::filters::BuyerFilter");
    ADD_METHOD_TO(CartController::clearCart, "/api/cart", drogon::Delete, "sanghavimart::filters::BuyerFilter");
    METHOD_LIST_END

    void getCart(const drogon::HttpRequestPtr& req,
                 std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void addToCart(const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void updateCartItem(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                        int id);

    void removeCartItem(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                        int id);

    void clearCart(const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

} // namespace sanghavimart::controllers
