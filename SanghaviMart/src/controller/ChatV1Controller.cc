#include "V1Controllers.h"
#include <cstdlib>
#include "../chat/Chat.h"
#include "../util/JsonUtil.h"

namespace sanghavimart::controller {

void ChatV1Controller::Chat(const drogon::HttpRequestPtr& req,
                            std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    auto j = req->getJsonObject();
    if (!j) {
        auto r = drogon::HttpResponse::newHttpJsonResponse(
            util::JsonUtil::Error("VALIDATION_ERROR", "Message required."));
        r->setStatusCode(drogon::k400BadRequest);
        cb(r);
        return;
    }
    std::string msg = (*j).get("message", "").asString();
    if (msg.empty() || msg.size() > 1000) {
        auto r = drogon::HttpResponse::newHttpJsonResponse(
            util::JsonUtil::Error("VALIDATION_ERROR", "Message must be 1-1000 chars."));
        r->setStatusCode(drogon::k400BadRequest);
        cb(r);
        return;
    }
    const char* prov = std::getenv("AI_CHATBOT_PROVIDER");
    static chat::ChatService svc(chat::MakeProvider(prov ? prov : "mock"));
    std::string sid = req->getHeader("X-Session-Id");
    if (sid.empty()) sid = req->getPeerAddr().toIp();
    Json::Value d;
    d["reply"] = svc.Chat(sid, msg);
    cb(drogon::HttpResponse::newHttpJsonResponse(util::JsonUtil::Success(d)));
}

}  // namespace sanghavimart::controller
