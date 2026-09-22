#include "ReviewController.h"

namespace sanghavimart::controllers {

void ReviewController::addReview(const drogon::HttpRequestPtr& req,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    int buyerId = req->attributes()->get<int>("user_id");
    auto json = req->getJsonObject();
    if (!json) {
        Json::Value err;
        err["success"] = false;
        err["error"] = "Invalid JSON body.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    int productId = (*json).get("product_id", 0).asInt();
    int rating = (*json).get("rating", 0).asInt();
    std::string comment = (*json).get("comment", "").asString();

    if (productId <= 0 || rating < 1 || rating > 5 || comment.empty()) {
        Json::Value err;
        err["success"] = false;
        err["error"] = "product_id, rating (between 1 and 5), and review comment are required.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    auto dbClient = drogon::app().getDbClient();

    // Verification check: Buyer must have purchased this product in at least one order
    dbClient->execSqlAsync(
        "SELECT oi.order_id "
        "FROM order_items oi "
        "JOIN orders o ON oi.order_id = o.id "
        "WHERE oi.product_id = $1 AND o.buyer_id = $2 "
        "ORDER BY o.id DESC LIMIT 1",
        [dbClient, buyerId, productId, rating, comment, callback](const drogon::orm::Result& orderMatch) {
            if (orderMatch.empty()) {
                Json::Value err;
                err["success"] = false;
                err["error"] = "Review rejected: You can only review products you have purchased on SanghaviMart.";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
                resp->setStatusCode(drogon::k403Forbidden);
                callback(resp);
                return;
            }

            int orderId = orderMatch[0]["order_id"].as<int>();

            // Insert or update review
            dbClient->execSqlAsync(
                "INSERT INTO reviews (product_id, buyer_id, order_id, rating, comment) "
                "VALUES ($1, $2, $3, $4, $5) "
                "ON CONFLICT (product_id, buyer_id, order_id) "
                "DO UPDATE SET rating = $4, comment = $5, created_at = CURRENT_TIMESTAMP "
                "RETURNING id, rating, comment, created_at",
                [callback](const drogon::orm::Result& revRes) {
                    Json::Value res;
                    res["success"] = true;
                    res["message"] = "Your verified review has been submitted successfully!";
                    res["review"]["id"] = revRes[0]["id"].as<int>();
                    res["review"]["rating"] = revRes[0]["rating"].as<int>();
                    res["review"]["comment"] = revRes[0]["comment"].as<std::string>();
                    res["review"]["created_at"] = revRes[0]["created_at"].as<std::string>();

                    auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
                    resp->setStatusCode(drogon::k201Created);
                    callback(resp);
                },
                [callback](const drogon::orm::DrogonDbException& e) {
                    Json::Value err;
                    err["success"] = false;
                    LOG_ERROR << "db error: " << e.base().what();
                    err["error"] = "Internal server error. Please try again.";
                    auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
                    resp->setStatusCode(drogon::k500InternalServerError);
                    callback(resp);
                },
                productId, buyerId, orderId, rating, comment
            );
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            Json::Value err;
            err["success"] = false;
            LOG_ERROR << "db error: " << e.base().what();
            err["error"] = "Internal server error. Please try again.";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
        },
        productId, buyerId
    );
}

void ReviewController::getProductReviews(const drogon::HttpRequestPtr&,
                                        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                        int id) {
    auto dbClient = drogon::app().getDbClient();
    dbClient->execSqlAsync(
        "SELECT r.id, r.product_id, r.buyer_id, u.name AS buyer_name, r.rating, r.comment, r.created_at "
        "FROM reviews r "
        "JOIN users u ON r.buyer_id = u.id "
        "WHERE r.product_id = $1 "
        "ORDER BY r.id DESC",
        [callback](const drogon::orm::Result& result) {
            Json::Value res;
            res["success"] = true;
            res["count"] = static_cast<int>(result.size());

            double sum = 0.0;
            Json::Value reviews(Json::arrayValue);
            for (const auto& row : result) {
                Json::Value r;
                r["id"] = row["id"].as<int>();
                r["buyer_name"] = row["buyer_name"].as<std::string>();
                int rating = row["rating"].as<int>();
                r["rating"] = rating;
                r["comment"] = row["comment"].as<std::string>();
                r["created_at"] = row["created_at"].as<std::string>();
                sum += rating;
                reviews.append(r);
            }

            res["average_rating"] = result.empty() ? 0.0 : (sum / result.size());
            res["reviews"] = reviews;

            auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            Json::Value err;
            err["success"] = false;
            LOG_ERROR << "db error: " << e.base().what();
            err["error"] = "Internal server error. Please try again.";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
        },
        id
    );
}

} // namespace sanghavimart::controllers
