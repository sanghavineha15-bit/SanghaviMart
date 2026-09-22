#pragma once
#include <drogon/HttpController.h>

namespace sanghavimart::controller {

// Spec §21: verified-purchase reviews only, rating 1-5, one review per
// user/product (409 on duplicate).
class ReviewV1Controller : public drogon::HttpController<ReviewV1Controller> {
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ReviewV1Controller::Add, "/api/v1/reviews", drogon::Post, "sanghavimart::filter::LoggingFilter");
    ADD_METHOD_TO(ReviewV1Controller::List, "/api/v1/products/{id}/reviews", drogon::Get, "sanghavimart::filter::LoggingFilter");
    METHOD_LIST_END

    void Add(const drogon::HttpRequestPtr& req,
             std::function<void(const drogon::HttpResponsePtr&)>&& cb);
    void List(const drogon::HttpRequestPtr& req,
              std::function<void(const drogon::HttpResponsePtr&)>&& cb, int id);
};

}  // namespace sanghavimart::controller
