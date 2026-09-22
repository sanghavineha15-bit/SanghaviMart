#pragma once
#include <string>
#include "Money.h"

namespace sanghavimart::model {

/// Domain entities (PostgreSQL-mapped, spec vocabulary).
struct User {
    int id{0};
    std::string name;
    std::string email;
    std::string role;  // BUYER|SELLER|ADMIN
    std::string created_at;
};

struct Product {
    int id{0};
    int seller_id{0};
    std::string name;
    std::string description;
    Money price{Money(0)};
    int stock_qty{0};
    std::string category{"General"};
    std::string image_url;
};

struct CartItem {
    int id{0};
    int user_id{0};
    int product_id{0};
    int quantity{0};
};

struct Order {
    int id{0};
    int buyer_id{0};
    std::string status{"PENDING"};  // PENDING|CONFIRMED|SHIPPED|DELIVERED|CANCELLED
    Money total{Money(0)};
};

struct OrderItem {
    int id{0};
    int order_id{0};
    int product_id{0};
    int quantity{0};
    Money unit_price{Money(0)};
};

struct Review {
    int id{0};
    int product_id{0};
    int user_id{0};
    int rating{0};
    std::string comment;
};

}  // namespace sanghavimart::model
