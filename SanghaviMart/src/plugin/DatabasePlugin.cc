#include "DatabasePlugin.h"
#include <cstdlib>
#include <spdlog/spdlog.h>
#include "../service/MigrationService.h"

namespace sanghavimart::plugin {

static std::string EnvOr(const char* key, const std::string& fallback) {
    const char* v = std::getenv(key);
    return (v && *v) ? std::string(v) : fallback;
}

void DatabasePlugin::initAndStart(const Json::Value& config) {
    const std::string dir = config.get("migrations_dir", "./db/migrations").asString();
    const bool run = config.get("run_on_startup", true).asBool();
    if (!run) {
        spdlog::info("DatabasePlugin: run_on_startup=false, skipping migrations.");
        return;
    }
    try {
        // Credentials come from the environment only — never from Git.
        const std::string conn =
            "host=" + EnvOr("POSTGRES_HOST", "127.0.0.1") + " port=" +
            EnvOr("POSTGRES_PORT", "5432") + " dbname=" + EnvOr("POSTGRES_DB", "sanghavimart_db") +
            " user=" + EnvOr("POSTGRES_USER", "postgres") +
            " password=" + EnvOr("POSTGRES_PASSWORD", "") + " connect_timeout=5";
        service::MigrationService runner(conn, dir);
        const int applied = runner.Run();
        spdlog::info("DatabasePlugin: startup migrations complete, {} applied.", applied);
    } catch (const std::exception& e) {
        // Never crash boot: /api/v1/health surfaces DB state instead.
        spdlog::error("DatabasePlugin: startup migrations skipped: {}", e.what());
    } catch (...) {
        spdlog::error("DatabasePlugin: startup migrations skipped (unknown error).");
    }
}

void DatabasePlugin::shutdown() {
    spdlog::info("DatabasePlugin: shutdown.");
}

}  // namespace sanghavimart::plugin
