#pragma once
#include <drogon/HttpController.h>

namespace sanghavimart::controller {

// Spec §15/16: seller CRUD + public browse/search/filter. Prices in
// price_cents (integer minor units). Thin HTTP layer over ProductService.
class ProductV1Controller : public drogon::HttpController<ProductV1Controller> {
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ProductV1Controller::List, "/api/v1/products", drogon::Get, "sanghavimart::filter::LoggingFilter");
    ADD_METHOD_TO(ProductV1Controller::Get, "/api/v1/products/{id}", drogon::Get, "sanghavimart::filter::LoggingFilter");
    ADD_METHOD_TO(ProductV1Controller::Create, "/api/v1/products", drogon::Post, "sanghavimart::filter::LoggingFilter");
    ADD_METHOD_TO(ProductV1Controller::Update, "/api/v1/products/{id}", drogon::Put, "sanghavimart::filter::LoggingFilter");
    ADD_METHOD_TO(ProductV1Controller::Remove, "/api/v1/products/{id}", drogon::Delete, "sanghavimart::filter::LoggingFilter");
    METHOD_LIST_END

    void List(const drogon::HttpRequestPtr& req,
              std::function<void(const drogon::HttpResponsePtr&)>&& cb);
    void Get(const drogon::HttpRequestPtr& req,
             std::function<void(const drogon::HttpResponsePtr&)>&& cb, int id);
    void Create(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& cb);
    void Update(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& cb, int id);
    void Remove(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& cb, int id);
};

}  // namespace sanghavimart::controller
