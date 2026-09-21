#pragma once
#include <drogon/HttpController.h>

namespace sanghavimart::controllers {

class ProductController : public drogon::HttpController<ProductController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ProductController::getAllProducts, "/api/products", drogon::Get);
    ADD_METHOD_TO(ProductController::getProductById, "/api/products/{id}", drogon::Get);
    ADD_METHOD_TO(ProductController::addProduct, "/api/products", drogon::Post, "sanghavimart::filters::SellerFilter");
    ADD_METHOD_TO(ProductController::updateProduct, "/api/products/{id}", drogon::Put, "sanghavimart::filters::SellerFilter");
    ADD_METHOD_TO(ProductController::deleteProduct, "/api/products/{id}", drogon::Delete, "sanghavimart::filters::AuthFilter");
    ADD_METHOD_TO(ProductController::getCategories, "/api/categories", drogon::Get);
    METHOD_LIST_END

    void getAllProducts(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void getProductById(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                        int id);

    void addProduct(const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void updateProduct(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                       int id);

    void deleteProduct(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                       int id);

    void getCategories(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

} // namespace sanghavimart::controllers
