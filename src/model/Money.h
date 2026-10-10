#pragma once
// SanghaviMart — Money value type. All currency as BIGINT minor units (cents).
// No raw-integer arithmetic may leak into business logic; use this type.
#include <cstdint>
#include <stdexcept>
#include <string>

namespace sanghavi::sanghavimart {

class Money {
 public:
  explicit Money(std::int64_t cents = 0) : cents_(cents) {
    if (cents < 0) throw std::invalid_argument("Money cannot be negative");
  }
  static Money FromCents(std::int64_t cents) { return Money(cents); }
  static Money FromMajor(double major) {
    return Money(static_cast<std::int64_t>(major * 100.0 + 0.5));
  }
  [[nodiscard]] std::int64_t Cents() const noexcept { return cents_; }
  [[nodiscard]] double ToMajor() const noexcept {
    return static_cast<double>(cents_) / 100.0;
  }
  Money operator+(const Money& o) const { return Money(cents_ + o.cents_); }
  Money operator*(std::int64_t qty) const {
    if (qty < 0) throw std::invalid_argument("qty cannot be negative");
    return Money(cents_ * qty);
  }
  bool operator==(const Money& o) const noexcept { return cents_ == o.cents_; }
  bool operator<(const Money& o) const noexcept { return cents_ < o.cents_; }
  std::string ToString() const { return std::to_string(cents_); }

 private:
  std::int64_t cents_{0};
};

}  // namespace sanghavi::sanghavimart
