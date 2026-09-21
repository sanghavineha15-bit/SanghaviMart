#pragma once
#include <string>
#include <vector>
#include <json/json.h>

namespace sanghavimart::models {

struct CartItem {
    int id{0};
    int cartId{0};
    int productId{0};
    std::string productName;
    std::string imageUrl;
    double unitPrice{0.0};
    int quantity{0};
    int availableStock{0};
    double subtotal{0.0};

    Json::Value toJson() const {
        Json::Value val;
        val["id"] = id;
        val["cart_id"] = cartId;
        val["product_id"] = productId;
        val["product_name"] = productName;
        val["image_url"] = imageUrl;
        val["unit_price"] = unitPrice;
        val["quantity"] = quantity;
        val["available_stock"] = availableStock;
        val["subtotal"] = subtotal;
        return val;
    }
};

struct Cart {
    int id{0};
    int buyerId{0};
    std::vector<CartItem> items;
    double totalPrice{0.0};
    int totalItems{0};

    Json::Value toJson() const {
        Json::Value val;
        val["id"] = id;
        val["buyer_id"] = buyerId;
        val["total_price"] = totalPrice;
        val["total_items"] = totalItems;
        Json::Value itemsArray(Json::arrayValue);
        for (const auto& item : items) {
            itemsArray.append(item.toJson());
        }
        val["items"] = itemsArray;
        return val;
    }
};

} // namespace sanghavimart::models
