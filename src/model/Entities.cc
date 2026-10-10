#include "model/Entities.h"
#include <stdexcept>

namespace sanghavi::sanghavimart {

std::string ToString(UserRole role) {
  switch (role) {
    case UserRole::kBuyer: return "BUYER";
    case UserRole::kSeller: return "SELLER";
    case UserRole::kAdmin: return "ADMIN";
  }
  return "BUYER";
}

UserRole RoleFromString(const std::string& s) {
  if (s == "SELLER") return UserRole::kSeller;
  if (s == "ADMIN") return UserRole::kAdmin;
  return UserRole::kBuyer;
}

std::string ToString(OrderStatus s) {
  switch (s) {
    case OrderStatus::kPending: return "PENDING";
    case OrderStatus::kConfirmed: return "CONFIRMED";
    case OrderStatus::kShipped: return "SHIPPED";
    case OrderStatus::kDelivered: return "DELIVERED";
    case OrderStatus::kCancelled: return "CANCELLED";
  }
  return "PENDING";
}

OrderStatus StatusFromString(const std::string& s) {
  if (s == "CONFIRMED") return OrderStatus::kConfirmed;
  if (s == "SHIPPED") return OrderStatus::kShipped;
  if (s == "DELIVERED") return OrderStatus::kDelivered;
  if (s == "CANCELLED") return OrderStatus::kCancelled;
  return OrderStatus::kPending;
}

bool IsValidStatusTransition(OrderStatus from, OrderStatus to) {
  if (from == to) return false;
  switch (from) {
    case OrderStatus::kPending:
      return to == OrderStatus::kConfirmed || to == OrderStatus::kCancelled;
    case OrderStatus::kConfirmed:
      return to == OrderStatus::kShipped || to == OrderStatus::kCancelled;
    case OrderStatus::kShipped:
      return to == OrderStatus::kDelivered;
    case OrderStatus::kDelivered:
    case OrderStatus::kCancelled:
      return false;
  }
  return false;
}

}  // namespace sanghavi::sanghavimart
