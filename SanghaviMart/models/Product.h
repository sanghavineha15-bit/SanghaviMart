#pragma once
#include <string>
#include <json/json.h>

namespace sanghavimart::models {

struct Product {
    int id{0};
    int sellerId{0};
    std::string sellerName;
    int categoryId{0};
    std::string categoryName;
    std::string name;
    std::string description;
    double price{0.0};
    int stockQuantity{0};
    std::string imageUrl;
    bool isActive{true};
    double averageRating{0.0};
    int reviewCount{0};
    std::string createdAt;
    std::string updatedAt;

    Json::Value toJson() const {
        Json::Value val;
        val["id"] = id;
        val["seller_id"] = sellerId;
        val["seller_name"] = sellerName;
        val["category_id"] = categoryId;
        val["category_name"] = categoryName;
        val["name"] = name;
        val["description"] = description;
        val["price"] = price;
        val["stock_quantity"] = stockQuantity;
        val["image_url"] = imageUrl;
        val["is_active"] = isActive;
        val["average_rating"] = averageRating;
        val["review_count"] = reviewCount;
        val["created_at"] = createdAt;
        return val;
    }
};

} // namespace sanghavimart::models
