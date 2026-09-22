#pragma once
#include <json/json.h>
#include <string>
#include "../model/Entities.h"

namespace sanghavimart::dto {

// Request DTOs (parsed + validated in services/controllers).
struct RegisterRequest {
    std::string name;
    std::string email;
    std::string password;
    std::string role;  // BUYER|SELLER (uppercased by controller)
};

struct ProductRequest {
    std::string name;
    std::string description;
    int64_t price_cents{0};  // integer minor units — never float
    int stock_qty{0};
    std::string category{"General"};
    std::string image_url;
};

// Response DTOs — NEVER include password_hash or internal fields.
struct UserResponse {
    int id{0};
    std::string name;
    std::string email;
    std::string role;
    Json::Value ToJson() const {
        Json::Value v;
        v["id"] = id;
        v["name"] = name;
        v["email"] = email;
        v["role"] = role;
        return v;
    }
};

struct ProductResponse {
    static Json::Value From(const model::Product& p) {
        Json::Value v;
        v["id"] = p.id;
        v["seller_id"] = p.seller_id;
        v["name"] = p.name;
        v["description"] = p.description;
        v["price_cents"] = (Json::Int64)p.price.MinorUnits();
        v["stock_qty"] = p.stock_qty;
        v["category"] = p.category;
        v["image_url"] = p.image_url;
        return v;
    }
};

struct OrderResponse {
    static Json::Value From(const model::Order& o) {
        Json::Value v;
        v["id"] = o.id;
        v["buyer_id"] = o.buyer_id;
        v["status"] = o.status;
        v["total_amount_cents"] = (Json::Int64)o.total.MinorUnits();
        return v;
    }
};

struct ReviewResponse {
    static Json::Value From(const model::Review& r) {
        Json::Value v;
        v["id"] = r.id;
        v["product_id"] = r.product_id;
        v["user_id"] = r.user_id;
        v["rating"] = r.rating;
        v["comment"] = r.comment;
        return v;
    }
};

}  // namespace sanghavimart::dto
