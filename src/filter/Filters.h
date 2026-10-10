#pragma once
// SanghaviMart filters: AuthFilter (session check), LoggingFilter (request id),
// CORSFilter. All extend drogon::HttpFilter.
#include <drogon/HttpFilter.h>
namespace sanghavi::sanghavimart {
class AuthFilter : public drogon::HttpFilter<AuthFilter> {
 public:
  void doFilter(const drogon::HttpRequestPtr& req, drogon::FilterCallback&& cb,
                drogon::FilterChainCallback&& chain) override;
};
class LoggingFilter : public drogon::HttpFilter<LoggingFilter> {
 public:
  void doFilter(const drogon::HttpRequestPtr& req, drogon::FilterCallback&& cb,
                drogon::FilterChainCallback&& chain) override;
};
class CORSFilter : public drogon::HttpFilter<CORSFilter> {
 public:
  void doFilter(const drogon::HttpRequestPtr& req, drogon::FilterCallback&& cb,
                drogon::FilterChainCallback&& chain) override;
};
}  // namespace sanghavi::sanghavimart
