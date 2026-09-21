#pragma once
#include <drogon/HttpFilter.h>
#include "../utils/JwtUtil.h"

namespace sanghavimart::filters {

class AuthFilter : public drogon::HttpFilter<AuthFilter> {
public:
    AuthFilter() {}
    void doFilter(const drogon::HttpRequestPtr& req,
                  drogon::FilterCallback&& fcb,
                  drogon::FilterChainCallback&& fccb) override;
};

// Role-specific filters
class BuyerFilter : public drogon::HttpFilter<BuyerFilter> {
public:
    void doFilter(const drogon::HttpRequestPtr& req,
                  drogon::FilterCallback&& fcb,
                  drogon::FilterChainCallback&& fccb) override;
};

class SellerFilter : public drogon::HttpFilter<SellerFilter> {
public:
    void doFilter(const drogon::HttpRequestPtr& req,
                  drogon::FilterCallback&& fcb,
                  drogon::FilterChainCallback&& fccb) override;
};

class AdminFilter : public drogon::HttpFilter<AdminFilter> {
public:
    void doFilter(const drogon::HttpRequestPtr& req,
                  drogon::FilterCallback&& fcb,
                  drogon::FilterChainCallback&& fccb) override;
};

} // namespace sanghavimart::filters
