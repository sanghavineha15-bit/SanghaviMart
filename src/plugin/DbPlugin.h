#pragma once
// DbPlugin — owns the connection-pool lifecycle (Singleton via Plugin).
// Configured once at startup; no ad-hoc connection construction elsewhere.
// Also runs MigrationService (numbered db/migrations/*.sql in order, tracked
// in schema_migrations, each inside a transaction) before serving traffic.
#include <drogon/plugins/Plugin.h>
#include <drogon/orm/DbClient.h>
namespace sanghavi::sanghavimart {
class DbPlugin : public drogon::Plugin<DbPlugin> {
 public:
  void initAndStart(const Json::Value& config) override;
  void shutdown() override;
  drogon::orm::DbClientPtr Client() const { return client_; }
 private:
  drogon::orm::DbClientPtr client_;
  void RunMigrations();
  std::string migrations_dir_{"./db/migrations"};
};
class MigrationService {
 public:
  explicit MigrationService(drogon::orm::DbClientPtr db) : db_(std::move(db)) {}
  void ApplyDirectory(const std::string& dir);
 private:
  drogon::orm::DbClientPtr db_;
};
}  // namespace sanghavi::sanghavimart
