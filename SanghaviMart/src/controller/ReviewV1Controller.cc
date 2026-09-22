#include "ReviewV1Controller.h"
#include <drogon/drogon.h>
#include "../dto/Dtos.h"
#include "../repository/PostgresCartOrderReview.h"
#include "../repository/PostgresRepositories.h"
#include "../util/JsonUtil.h"
#include "../util/ValidationUtil.h"
#include "SessionAuth.h"
#include "V1Error.h"

namespace sanghavimart::controller {

static std::shared_ptr<repository::IReviewRepository> Reviews() {
    return std::make_shared<repository::PostgresReviewRepository>(
        drogon::app().getDbClient());
}

void ReviewV1Controller::Add(const drogon::HttpRequestPtr& req,
                             std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    try {
        auto user = RequireSession(req, true, false, false);
        auto j = req->getJsonObject();
        if (!j) throw exception::ValidationException("Invalid JSON body.");
        int pid = (*j).get("product_id", 0).asInt();
        int rating = (*j).get("rating", 0).asInt();
        std::string comment = (*j).get("comment", "").asString();
        if (pid <= 0) throw exception::ValidationException("Invalid product id.");
        util::ValidationUtil::ValidateRating(rating);
        util::ValidationUtil::ValidateComment(comment);
        auto repos = Reviews();
        if (!repos->HasPurchased(user.id, pid))
            throw exception::ForbiddenException("Only buyers who purchased this product may review it.");
        model::Review r;
        r.product_id = pid;
        r.user_id = user.id;
        r.rating = rating;
        r.comment = comment;
        int id;
        try {
            id = repos->Add(r);
        } catch (const std::exception&) {
            throw exception::ConflictException("You have already reviewed this product.");
        }
        r.id = id;
        Json::Value d;
        d["review"] = dto::ReviewResponse::From(r);
        auto resp = drogon::HttpResponse::newHttpJsonResponse(util::JsonUtil::Success(d));
        resp->setStatusCode(drogon::k201Created);
        cb(resp);
    } catch (const exception::AppException& e) {
        SendAppError(cb, e);
    } catch (const std::exception& e) {
        SendInternal(cb, "ReviewV1::Add", e);
    }
}

void ReviewV1Controller::List(const drogon::HttpRequestPtr&,
                              std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                              int id) {
    try {
        if (id <= 0) throw exception::ValidationException("Invalid product id.");
        auto v = Reviews()->ForProduct(id);
        Json::Value arr(Json::arrayValue);
        for (auto& r : v) arr.append(dto::ReviewResponse::From(r));
        Json::Value d;
        d["reviews"] = arr;
        d["count"] = (int)v.size();
        cb(drogon::HttpResponse::newHttpJsonResponse(util::JsonUtil::Success(d)));
    } catch (const exception::AppException& e) {
        SendAppError(cb, e);
    } catch (const std::exception& e) {
        SendInternal(cb, "ReviewV1::List", e);
    }
}

}  // namespace sanghavimart::controller
