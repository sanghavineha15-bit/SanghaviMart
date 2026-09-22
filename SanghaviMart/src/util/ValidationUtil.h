#pragma once
#include <regex>
#include <string>
#include "../exception/AppException.h"

namespace sanghavimart::util {

/// Service-layer validation. Throws ValidationException on failure.
class ValidationUtil {
  public:
    static void RequireNonEmpty(const std::string& v, const std::string& field) {
        if (v.empty()) throw exception::ValidationException(field + " is required.");
    }
    static void ValidateEmail(const std::string& email) {
        static const std::regex kRe(R"(^[\w\.-]+@[\w\.-]+\.\w+$)");
        if (!std::regex_match(email, kRe))
            throw exception::ValidationException("Invalid email address.");
    }
    static void ValidatePassword(const std::string& pw) {
        if (pw.size() < 8)
            throw exception::ValidationException("Password must be at least 8 characters.");
    }
    static void ValidateRole(const std::string& role) {
        if (role != "BUYER" && role != "SELLER")
            throw exception::ValidationException("Role must be BUYER or SELLER.");
    }
    static void ValidatePriceCents(int64_t cents) {
        if (cents < 0 || cents > 1000000000LL)
            throw exception::ValidationException("Invalid price.");
    }
    static void ValidateQuantity(int q) {
        if (q <= 0 || q > 1000) throw exception::ValidationException("Invalid quantity.");
    }
    static void ValidateRating(int r) {
        if (r < 1 || r > 5) throw exception::ValidationException("Rating must be 1-5.");
    }
    static void ValidateComment(const std::string& c) {
        if (c.empty() || c.size() > 2000)
            throw exception::ValidationException("Comment must be 1-2000 chars.");
    }
    static void ValidateName(const std::string& n, const std::string& field = "Name") {
        if (n.size() < 2 || n.size() > 200)
            throw exception::ValidationException(field + " must be 2-200 chars.");
    }
    static void ValidateChatMessage(const std::string& m) {
        if (m.empty() || m.size() > 1000)
            throw exception::ValidationException("Message must be 1-1000 chars.");
    }
};

}  // namespace sanghavimart::util
