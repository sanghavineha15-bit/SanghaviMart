#pragma once
#include <drogon/HttpRequest.h>
#include <string>
#include "../exception/AppException.h"

namespace sanghavimart::controller {

struct SessionUser {
    int id{0};
    std::string role;
};

/// @brief Session guard for /api/v1. Throws 401 when logged out, 403 when the
/// role is not in the allowed set. Never trusts frontend role claims.
inline SessionUser RequireSession(const drogon::HttpRequestPtr& req,
                                  bool allow_buyer = true, bool allow_seller = false,
                                  bool allow_admin = false) {
    if (!req->session()->find("user_id"))
        throw exception::UnauthorizedException("Authentication required.");
    SessionUser u;
    u.id = req->session()->get<int>("user_id");
    u.role = req->session()->get<std::string>("user_role");
    bool ok = (allow_buyer && u.role == "BUYER") || (allow_seller && u.role == "SELLER") ||
              (allow_admin && u.role == "ADMIN");
    if (!ok) throw exception::ForbiddenException("Insufficient role.");
    return u;
}

}  // namespace sanghavimart::controller
