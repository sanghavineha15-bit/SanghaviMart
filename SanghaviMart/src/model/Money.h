#pragma once
#include <cstdint>
#include <string>

namespace sanghavimart::model {

/// Money in integer minor units (paise/cents). BIGINT in PostgreSQL.
/// Never use float/double for currency.
class Money {
  public:
    explicit Money(int64_t minor_units = 0) : minor_units_(minor_units) {}

    static Money FromMajor(double major) {
        return Money(static_cast<int64_t>(major * 100.0 + 0.5));
    }
    static Money FromMinor(int64_t minor) { return Money(minor); }

    int64_t MinorUnits() const { return minor_units_; }
    double ToMajor() const { return minor_units_ / 100.0; }

    Money operator+(const Money& o) const { return Money(minor_units_ + o.minor_units_); }
    Money operator*(int64_t qty) const { return Money(minor_units_ * qty); }
    bool operator<(const Money& o) const { return minor_units_ < o.minor_units_; }

  private:
    int64_t minor_units_{0};
};

inline std::string FormatMoneyMinor(int64_t minor) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%lld.%02lld", (long long)(minor / 100),
             (long long)(minor % 100 < 0 ? -(minor % 100) : (minor % 100)));
    return std::string(buf);
}

}  // namespace sanghavimart::model
