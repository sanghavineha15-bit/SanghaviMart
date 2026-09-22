#pragma once
#include <drogon/HttpController.h>

namespace sanghavimart::controller {

// Spec §17: buyer cart. Server validates product/stock; never trusts prices.
class CartV1Controller : public drogon::HttpController<CartV1Controller> {
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(CartV1Controller::Show, "/api/v1/cart", drogon::Get);
    ADD_METHOD_TO(CartV1Controller::Add, "/api/v1/cart", drogon::Post);
    ADD_METHOD_TO(CartV1Controller::SetQty, "/api/v1/cart/{productId}", drogon::Put);
    ADD_METHOD_TO(CartV1Controller::Remove, "/api/v1/cart/{productId}", drogon::Delete);
    ADD_METHOD_TO(CartV1Controller::Clear, "/api/v1/cart", drogon::Delete);
    METHOD_LIST_END

    void Show(const drogon::HttpRequestPtr& req,
              std::function<void(const drogon::HttpResponsePtr&)>&& cb);
    void Add(const drogon::HttpRequestPtr& req,
             std::function<void(const drogon::HttpResponsePtr&)>&& cb);
    void SetQty(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& cb, int productId);
    void Remove(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& cb, int productId);
    void Clear(const drogon::HttpRequestPtr& req,
               std::function<void(const drogon::HttpResponsePtr&)>&& cb);
};

}  // namespace sanghavimart::controller
