#include "AuthFilter.h"
#include <json/json.h>

namespace sanghavimart::filters {

static bool extractAndVerify(const drogon::HttpRequestPtr& req,
                             utils::JwtPayload& payload,
                             drogon::HttpResponsePtr& errResp) {
    std::string authHeader = req->getHeader("Authorization");
    if (authHeader.empty() || authHeader.rfind("Bearer ", 0) != 0) {
        Json::Value err;
        err["success"] = false;
        err["error"] = "Authorization token required. Please provide 'Bearer <token>'.";
        errResp = drogon::HttpResponse::newHttpJsonResponse(err);
        errResp->setStatusCode(drogon::k401Unauthorized);
        return false;
    }

    std::string token = authHeader.substr(7);
    payload = utils::JwtUtil::verifyToken(token);
    if (!payload.valid) {
        Json::Value err;
        err["success"] = false;
        err["error"] = "Invalid or expired token. Please log in again.";
        errResp = drogon::HttpResponse::newHttpJsonResponse(err);
        errResp->setStatusCode(drogon::k401Unauthorized);
        return false;
    }

    req->attributes()->insert("user_id", payload.userId);
    req->attributes()->insert("user_email", payload.email);
    req->attributes()->insert("user_name", payload.name);
    req->attributes()->insert("user_role", payload.role);
    return true;
}

void AuthFilter::doFilter(const drogon::HttpRequestPtr& req,
                          drogon::FilterCallback&& fcb,
                          drogon::FilterChainCallback&& fccb) {
    utils::JwtPayload payload;
    drogon::HttpResponsePtr errResp;
    if (!extractAndVerify(req, payload, errResp)) {
        fcb(errResp);
        return;
    }
    fccb();
}

void BuyerFilter::doFilter(const drogon::HttpRequestPtr& req,
                           drogon::FilterCallback&& fcb,
                           drogon::FilterChainCallback&& fccb) {
    utils::JwtPayload payload;
    drogon::HttpResponsePtr errResp;
    if (!extractAndVerify(req, payload, errResp)) {
        fcb(errResp);
        return;
    }
    if (payload.role != "BUYER" && payload.role != "ADMIN") {
        Json::Value err;
        err["success"] = false;
        err["error"] = "Access restricted to Buyers only.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k403Forbidden);
        fcb(resp);
        return;
    }
    fccb();
}

void SellerFilter::doFilter(const drogon::HttpRequestPtr& req,
                            drogon::FilterCallback&& fcb,
                            drogon::FilterChainCallback&& fccb) {
    utils::JwtPayload payload;
    drogon::HttpResponsePtr errResp;
    if (!extractAndVerify(req, payload, errResp)) {
        fcb(errResp);
        return;
    }
    if (payload.role != "SELLER" && payload.role != "ADMIN") {
        Json::Value err;
        err["success"] = false;
        err["error"] = "Access restricted to registered Sellers only.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k403Forbidden);
        fcb(resp);
        return;
    }
    fccb();
}

void AdminFilter::doFilter(const drogon::HttpRequestPtr& req,
                           drogon::FilterCallback&& fcb,
                           drogon::FilterChainCallback&& fccb) {
    utils::JwtPayload payload;
    drogon::HttpResponsePtr errResp;
    if (!extractAndVerify(req, payload, errResp)) {
        fcb(errResp);
        return;
    }
    if (payload.role != "ADMIN") {
        Json::Value err;
        err["success"] = false;
        err["error"] = "Administrative privileges required.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k403Forbidden);
        fcb(resp);
        return;
    }
    fccb();
}

} // namespace sanghavimart::filters
