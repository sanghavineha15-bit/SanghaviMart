#pragma once
#include <drogon/plugins/Plugin.h>

namespace sanghavimart::plugin {

/// @brief Application lifecycle plugin: owns PostgreSQL startup duties.
///
/// On `initAndStart` it runs pending `db/migrations/V*.sql` through
/// service::MigrationService (each in its own transaction, recorded in
/// schema_migrations) and verifies `SELECT 1`. Any failure is logged via
/// spdlog and boot continues — the server must never crash at startup;
/// /api/v1/health reports DB state at runtime.
/// Registered in config.json "plugins".
class DatabasePlugin : public drogon::Plugin<DatabasePlugin> {
  public:
    void initAndStart(const Json::Value& config) override;
    void shutdown() override;
};

}  // namespace sanghavimart::plugin
