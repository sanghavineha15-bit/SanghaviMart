#include "filter/Filters.h"
#include <random>
#include <spdlog/spdlog.h>
#include "controller/Controllers.h"
namespace sanghavi::sanghavimart {
void AuthFilter::doFilter(const drogon::HttpRequestPtr& req, drogon::FilterCallback&& cb,
                          drogon::FilterChainCallback&& chain) {
  // JWT-first: survives Vercel scale-to-zero; session is the local-dev fallback.
  if (ServiceLocator::Instance().SessionUser(req) == 0) {
    Json::Value b; b["success"] = false; b["data"] = Json::Value::null;
    b["error"]["code"] = "UNAUTHORIZED"; b["error"]["message"] = "Authentication required";
    auto r = drogon::HttpResponse::newHttpJsonResponse(b);
    r->setStatusCode(drogon::k401Unauthorized); cb(r); return;
  }
  chain();
}
void LoggingFilter::doFilter(const drogon::HttpRequestPtr& req, drogon::FilterCallback&& cb,
                             drogon::FilterChainCallback&& chain) {
  static thread_local std::mt19937_64 rng{std::random_device{}()};
  std::string rid = "req-" + std::to_string(rng());
  req->getAttributes()->insert("request_id", rid);
  spdlog::info("[{}] {} {}", rid, req->getMethodString(), req->getPath());
  chain();
  (void)cb;
}
void CORSFilter::doFilter(const drogon::HttpRequestPtr& req, drogon::FilterCallback&& cb,
                          drogon::FilterChainCallback&& chain) {
  (void)req; (void)cb; chain();  // headers applied in main via post-handling advice
}
}  // namespace sanghavi::sanghavimart
