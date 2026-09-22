#pragma once
#include <chrono>
#include <curl/curl.h>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace sanghavimart::chat {

/// ChatProvider interface (Factory pattern).
class ChatProvider {
  public:
    virtual ~ChatProvider() = default;
    virtual std::string GetReply(const std::string& user_message,
                                 const std::string& context) = 0;
};

/// Safe static fallback (never crashes server).
class MockChatProvider : public ChatProvider {
  public:
    std::string GetReply(const std::string& msg, const std::string&) override {
        std::string l = msg;
        for (auto& c : l) c = (char)tolower(c);
        if (l.find("order") != std::string::npos)
            return "Track orders under Order History: PENDING -> CONFIRMED -> SHIPPED -> DELIVERED.";
        if (l.find("cart") != std::string::npos || l.find("checkout") != std::string::npos)
            return "Add to cart, review quantities, checkout. Stock is validated server-side in a transaction.";
        if (l.find("seller") != std::string::npos)
            return "Sellers manage listings in Seller Dashboard; only own products are editable.";
        return "Welcome to SanghaviMart! Ask about products, cart, checkout, orders, or seller features.";
    }
};

static size_t CurlWrite(void* p, size_t s, size_t n, void* u) {
    ((std::string*)u)->append((char*)p, s * n);
    return s * n;
}

/// Gemini HTTP provider (libcurl, timeout, server-side key only).
class GeminiChatProvider : public ChatProvider {
  public:
    GeminiChatProvider(std::string api_key, std::string model, long timeout_ms)
        : key_(std::move(api_key)), model_(std::move(model)), timeout_ms_(timeout_ms) {}

    std::string GetReply(const std::string& user_message,
                         const std::string& context) override {
        static const char* kSystem =
            "You are SanghaviMart assistant. ONLY answer marketplace/product/listing/order "
            "questions. Refuse off-topic requests briefly.";
        try {
            nlohmann::json body = {
                {"system_instruction", {{"parts", {{{"text", kSystem}}}}}},
                {"contents", {{{"parts", {{{"text", context + "\nUser: " + user_message}}}}}}}};
            std::string payload = body.dump();
            std::string url =
                "https://generativelanguage.googleapis.com/v1beta/models/" + model_ +
                ":generateContent?key=" + key_;
            CURL* curl = curl_easy_init();
            if (!curl) throw std::runtime_error("curl init failed");
            std::string resp;
            struct curl_slist* hdrs = nullptr;
            hdrs = curl_slist_append(hdrs, "Content-Type: application/json");
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_ms_);
            CURLcode rc = curl_easy_perform(curl);
            curl_slist_free_all(hdrs);
            curl_easy_cleanup(curl);
            if (rc != CURLE_OK) throw std::runtime_error(curl_easy_strerror(rc));
            auto j = nlohmann::json::parse(resp);
            return j["candidates"][0]["content"]["parts"][0]["text"].get<std::string>();
        } catch (const std::exception& e) {
            spdlog::warn("gemini failed: {}", e.what());
            return MockChatProvider().GetReply(user_message, context);
        }
    }

  private:
    std::string key_, model_;
    long timeout_ms_;
};

inline std::unique_ptr<ChatProvider> MakeProvider(const std::string& name) {
    const char* key = std::getenv("GEMINI_API_KEY");
    if (name == "gemini" && key && std::string(key).size() > 8) {
        const char* m = std::getenv("GEMINI_MODEL");
        return std::make_unique<GeminiChatProvider>(key, m ? m : "gemini-2.0-flash", 8000);
    }
    return std::make_unique<MockChatProvider>();
}

/// Rate-limited (10/min/session) + per-session cache chat service.
class ChatService {
  public:
    explicit ChatService(std::unique_ptr<ChatProvider> p) : provider_(std::move(p)) {}

    std::string Chat(const std::string& session_id, const std::string& message) {
        if (message.empty() || message.size() > 1000) return Fallback();
        std::lock_guard<std::mutex> g(mu_);
        auto now = std::chrono::steady_clock::now();
        auto& times = hits_[session_id];
        while (!times.empty() &&
               now - times.front() > std::chrono::minutes(1))
            times.erase(times.begin());
        if (times.size() >= 10) return "Rate limit: max 10 messages/minute. Please wait.";
        auto key = session_id + "|" + message;
        auto it = cache_.find(key);
        if (it != cache_.end()) return it->second;
        times.push_back(now);
        std::string reply;
        try {
            reply = provider_->GetReply(message, "SanghaviMart marketplace context.");
        } catch (...) {
            reply = Fallback();
        }
        cache_[key] = reply;
        if (cache_.size() > 500) cache_.erase(cache_.begin());
        return reply;
    }

  private:
    static std::string Fallback() {
        return "I can help with products, cart, checkout, orders, and seller features.";
    }
    std::unique_ptr<ChatProvider> provider_;
    std::mutex mu_;
    std::map<std::string, std::vector<std::chrono::steady_clock::time_point>> hits_;
    std::map<std::string, std::string> cache_;
};

}  // namespace sanghavimart::chat
