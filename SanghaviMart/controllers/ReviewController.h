#pragma once
#include <drogon/HttpController.h>

namespace sanghavimart::controllers {

class ReviewController : public drogon::HttpController<ReviewController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ReviewController::addReview, "/api/reviews", drogon::Post, "sanghavimart::filters::BuyerFilter");
    ADD_METHOD_TO(ReviewController::getProductReviews, "/api/products/{id}/reviews", drogon::Get);
    METHOD_LIST_END

    void addReview(const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void getProductReviews(const drogon::HttpRequestPtr& req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                           int id);
};

} // namespace sanghavimart::controllers
