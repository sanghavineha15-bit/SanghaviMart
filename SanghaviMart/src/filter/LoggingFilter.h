#pragma once
#include <drogon/HttpFilter.h>
#include <spdlog/spdlog.h>
#include <atomic>

namespace sanghavimart::filter {

/// Every request gets an ID; never logs secrets.
class LoggingFilter : public drogon::HttpFilter<LoggingFilter> {
  public:
    void doFilter(const drogon::HttpRequestPtr& req, drogon::FilterCallback&& fcb,
                  drogon::FilterChainCallback&& fccb) override {
        static std::atomic<uint64_t> s_seq{0};
        uint64_t id = ++s_seq;
        req->attributes()->insert("request_id", std::to_string(id));
        spdlog::info("[{}] {} {}", id, req->methodString(), req->path());
        fccb();
    }
};

}  // namespace sanghavimart::filter
