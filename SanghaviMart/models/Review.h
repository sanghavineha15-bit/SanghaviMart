#pragma once
#include <string>
#include <json/json.h>

namespace sanghavimart::models {

struct Review {
    int id{0};
    int productId{0};
    int buyerId{0};
    std::string buyerName;
    int orderId{0};
    int rating{5};
    std::string comment;
    std::string createdAt;

    Json::Value toJson() const {
        Json::Value val;
        val["id"] = id;
        val["product_id"] = productId;
        val["buyer_id"] = buyerId;
        val["buyer_name"] = buyerName;
        val["order_id"] = orderId;
        val["rating"] = rating;
        val["comment"] = comment;
        val["created_at"] = createdAt;
        return val;
    }
};

} // namespace sanghavimart::models
