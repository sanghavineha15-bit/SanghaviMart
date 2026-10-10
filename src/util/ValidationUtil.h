#pragma once
// ValidationUtil — pure functions; called at the TOP of every service method
// before any repository call (spec section 11.5).
#include <cstdint>
#include <string>
namespace sanghavi::sanghavimart {
class ValidationUtil {
 public:
  ValidationUtil() = delete;
  static void RequireNonEmpty(const std::string& field, const std::string& value);
  static void RequireEmail(const std::string& email);
  static void RequirePassword(const std::string& password);
  static void RequireRole(const std::string& role);
  static void RequirePriceCents(std::int64_t cents);
  static void RequireStockQty(std::int64_t qty);
  static void RequireQuantity(std::int64_t qty);
  static void RequireRating(int rating);
  static std::string Trim(const std::string& s);
};
}  // namespace sanghavi::sanghavimart
