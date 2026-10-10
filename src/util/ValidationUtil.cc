#include "util/ValidationUtil.h"
#include "exception/AppException.h"
#include <regex>
namespace sanghavi::sanghavimart {
void ValidationUtil::RequireNonEmpty(const std::string& field, const std::string& value) {
  if (Trim(value).empty()) throw ValidationException(field + " must not be empty");
}
void ValidationUtil::RequireEmail(const std::string& email) {
  static const std::regex kRe(R"(^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$)");
  if (!std::regex_match(email, kRe)) throw ValidationException("invalid email format");
}
void ValidationUtil::RequirePassword(const std::string& password) {
  if (password.size() < 8) throw ValidationException("password must be at least 8 characters");
}
void ValidationUtil::RequireRole(const std::string& role) {
  if (role != "BUYER" && role != "SELLER") throw ValidationException("role must be BUYER or SELLER");
}
void ValidationUtil::RequirePriceCents(std::int64_t cents) {
  if (cents <= 0) throw ValidationException("price must be positive (cents)");
  if (cents > 1000000000LL) throw ValidationException("price exceeds maximum");
}
void ValidationUtil::RequireStockQty(std::int64_t qty) {
  if (qty < 0) throw ValidationException("stock quantity cannot be negative");
}
void ValidationUtil::RequireQuantity(std::int64_t qty) {
  if (qty <= 0) throw ValidationException("quantity must be >= 1");
  if (qty > 1000) throw ValidationException("quantity exceeds maximum (1000)");
}
void ValidationUtil::RequireRating(int rating) {
  if (rating < 1 || rating > 5) throw ValidationException("rating must be 1..5");
}
std::string ValidationUtil::Trim(const std::string& s) {
  const auto b = s.find_first_not_of(" \t\r\n");
  if (b == std::string::npos) return "";
  const auto e = s.find_last_not_of(" \t\r\n");
  return s.substr(b, e - b + 1);
}
}  // namespace sanghavi::sanghavimart
