#pragma once
#include <string>
#include <vector>
#include <json/json.h>

namespace sanghavimart::models {

struct OrderItem {
    int id{0};
    int orderId{0};
    int productId{0};
    std::string productName;
    std::string imageUrl;
    int sellerId{0};
    std::string sellerName;
    int quantity{0};
    double unitPrice{0.0};
    double subtotal{0.0};

    Json::Value toJson() const {
        Json::Value val;
        val["id"] = id;
        val["order_id"] = orderId;
        val["product_id"] = productId;
        val["product_name"] = productName;
        val["image_url"] = imageUrl;
        val["seller_id"] = sellerId;
        val["seller_name"] = sellerName;
        val["quantity"] = quantity;
        val["unit_price"] = unitPrice;
        val["subtotal"] = subtotal;
        return val;
    }
};

struct Order {
    int id{0};
    std::string orderNumber;
    int buyerId{0};
    std::string buyerName;
    std::string buyerEmail;
    double totalAmount{0.0};
    std::string status; // "pending", "processing", "shipped", "delivered", "cancelled"
    std::string shippingName;
    std::string shippingPhone;
    std::string shippingAddress;
    std::string shippingCity;
    std::string shippingPostalCode;
    std::string paymentMethod;
    std::string createdAt;
    std::vector<OrderItem> items;

    Json::Value toJson() const {
        Json::Value val;
        val["id"] = id;
        val["order_number"] = orderNumber;
        val["buyer_id"] = buyerId;
        val["buyer_name"] = buyerName;
        val["buyer_email"] = buyerEmail;
        val["total_amount"] = totalAmount;
        val["status"] = status;
        val["shipping_name"] = shippingName;
        val["shipping_phone"] = shippingPhone;
        val["shipping_address"] = shippingAddress;
        val["shipping_city"] = shippingCity;
        val["shipping_postal_code"] = shippingPostalCode;
        val["payment_method"] = paymentMethod;
        val["created_at"] = createdAt;
        Json::Value itemsArr(Json::arrayValue);
        for (const auto& it : items) {
            itemsArr.append(it.toJson());
        }
        val["items"] = itemsArr;
        return val;
    }
};

} // namespace sanghavimart::models
