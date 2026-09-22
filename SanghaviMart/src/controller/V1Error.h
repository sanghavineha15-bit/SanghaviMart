#pragma once
#include <drogon/HttpResponse.h>
#include <functional>
#include "../exception/AppException.h"
#include "../util/JsonUtil.h"

namespace sanghavimart::controller {

/// Maps AppException hierarchy to {success,data,error} + HTTP status.
/// Unexpected exceptions become generic 500 (details go to server log only).
inline void SendAppError(std::function<void(const drogon::HttpResponsePtr&)>& cb,
                         const exception::AppException& e) {
    drogon::HttpStatusCode st = drogon::k500InternalServerError;
    const std::string& c = e.Code();
    if (c == "VALIDATION_ERROR") st = drogon::k400BadRequest;
    if (c == "UNAUTHORIZED") st = drogon::k401Unauthorized;
    if (c == "FORBIDDEN") st = drogon::k403Forbidden;
    if (c == "NOT_FOUND") st = drogon::k404NotFound;
    if (c == "CONFLICT") st = drogon::k409Conflict;
    auto r = drogon::HttpResponse::newHttpJsonResponse(util::JsonUtil::Error(c, e.PublicMessage()));
    r->setStatusCode(st);
    cb(r);
}

inline void SendInternal(std::function<void(const drogon::HttpResponsePtr&)>& cb,
                         const char* where, const std::exception& e) {
    LOG_ERROR << where << " failed: " << e.what();
    auto r = drogon::HttpResponse::newHttpJsonResponse(
        util::JsonUtil::Error("INTERNAL_ERROR", "Internal server error."));
    r->setStatusCode(drogon::k500InternalServerError);
    cb(r);
}

}  // namespace sanghavimart::controller
