#pragma once
// SanghaviMart entities (POD structs). Doxygen on every public type.
#include <cstdint>
#include <string>
#include "model/Money.h"

namespace sanghavi::sanghavimart {

/// User roles. ADMIN is only created via seed migration, never via signup.
enum class UserRole { kBuyer, kSeller, kAdmin };

/// Converts role <-> string stored in DB CHECK(role IN ('BUYER','SELLER','ADMIN')).
std::string ToString(UserRole role);
UserRole RoleFromString(const std::string& s);

/// Row of `users` table. Never serialise password_hash to clients (see dto/).
struct User {
  std::int64_t id{0};
  std::string name;
  std::string email;
  std::string password_hash;  ///< Argon2id string from libsodium, never logged.
  UserRole role{UserRole::kBuyer};
  std::string created_at;
};

/// Row of `products` table.
struct Product {
  std::int64_t id{0};
  std::int64_t seller_id{0};
  std::string name;
  std::string description;
  Money price{Money::FromCents(0)};
  std::int64_t stock_qty{0};
  std::string category;
  std::string image_url;
  std::string created_at;
};

/// Order status workflow: PENDING -> CONFIRMED -> SHIPPED -> DELIVERED (+CANCELLED).
enum class OrderStatus { kPending, kConfirmed, kShipped, kDelivered, kCancelled };
std::string ToString(OrderStatus s);
OrderStatus StatusFromString(const std::string& s);
/// Returns true if transition from->to is legal.
bool IsValidStatusTransition(OrderStatus from, OrderStatus to);

struct Order {
  std::int64_t id{0};
  std::int64_t buyer_id{0};
  OrderStatus status{OrderStatus::kPending};
  Money total{Money::FromCents(0)};
  std::string created_at;
};

struct OrderItem {
  std::int64_t id{0};
  std::int64_t order_id{0};
  std::int64_t product_id{0};
  std::int64_t quantity{0};
  Money unit_price{Money::FromCents(0)};
};

struct CartItem {
  std::int64_t id{0};
  std::int64_t user_id{0};
  std::int64_t product_id{0};
  std::int64_t quantity{0};
  Money unit_price{Money::FromCents(0)};  ///< denormalised snapshot for totals.
  std::string product_name;
};

struct Review {
  std::int64_t id{0};
  std::int64_t product_id{0};
  std::int64_t user_id{0};
  int rating{5};  ///< 1..5
  std::string comment;
  std::string created_at;
};

}  // namespace sanghavi::sanghavimart
