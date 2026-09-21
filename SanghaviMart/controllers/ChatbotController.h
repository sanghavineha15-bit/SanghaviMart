#pragma once
#include <drogon/HttpController.h>

namespace sanghavimart::controllers {

class ChatbotController : public drogon::HttpController<ChatbotController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ChatbotController::handleChatMessage, "/api/chat", drogon::Post);
    METHOD_LIST_END

    void handleChatMessage(const drogon::HttpRequestPtr& req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

} // namespace sanghavimart::controllers
