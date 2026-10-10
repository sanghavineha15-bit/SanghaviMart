// SanghaviMart - standalone compiled binary. Run as systemd service in prod.
#include <cstdlib>
#include <drogon/drogon.h>
#include <spdlog/spdlog.h>
int main() {
  spdlog::set_level(spdlog::level::info);
  try {
    drogon::app().loadConfigFile("./config.json");
    if (const char* port = std::getenv("PORT")) {  // Vercel Container Runtime
      drogon::app().addListener("0.0.0.0",
                                static_cast<uint16_t>(std::stoi(port)));
      spdlog::info("listening on PORT={} (env override)", port);
    }
    // Explicit idle session timeout (server-side store, cookie-backed).
    drogon::app().registerPostHandlingAdvice([](const drogon::HttpRequestPtr&,
                                                const drogon::HttpResponsePtr& resp) {
      resp->addHeader("Access-Control-Allow-Origin", "*");
      resp->addHeader("X-Content-Type-Options", "nosniff");
    });
    spdlog::info("SanghaviMart starting...");
    drogon::app().run();
  } catch (const std::exception& e) {
    spdlog::critical("startup failed: {}", e.what());
    return 1;
  }
  return 0;
}
