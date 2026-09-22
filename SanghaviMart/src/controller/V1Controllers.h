#pragma once
#include <drogon/HttpController.h>
#include "../util/JsonUtil.h"

namespace sanghavimart::controller {

// GET /api/v1/health — runs SELECT 1.
class HealthController : public drogon::HttpController<HealthController> {
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(HealthController::Check, "/api/v1/health", drogon::Get);
    METHOD_LIST_END
    void Check(const drogon::HttpRequestPtr&,
               std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
        try {
            auto db = drogon::app().getDbClient();
            db->execSqlSync("SELECT 1");
            Json::Value d;
            d["status"] = "UP";
            d["db"] = "UP";
            cb(drogon::HttpResponse::newHttpJsonResponse(util::JsonUtil::Success(d)));
        } catch (...) {
            Json::Value d;
            d["status"] = "DOWN";
            d["db"] = "DOWN";
            auto r = drogon::HttpResponse::newHttpJsonResponse(
                util::JsonUtil::Error("DB_UNAVAILABLE", "Database unavailable."));
            r->setStatusCode(drogon::k503ServiceUnavailable);
            cb(r);
        }
    }
};

// POST /api/v1/chat — rate-limited marketplace assistant.
class ChatV1Controller : public drogon::HttpController<ChatV1Controller> {
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ChatV1Controller::Chat, "/api/v1/chat", drogon::Post);
    METHOD_LIST_END
    void Chat(const drogon::HttpRequestPtr& req,
              std::function<void(const drogon::HttpResponsePtr&)>&& cb);
};

}  // namespace sanghavimart::controller
