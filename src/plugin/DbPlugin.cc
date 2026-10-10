#include "plugin/DbPlugin.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <spdlog/spdlog.h>
#include "controller/Controllers.h"
namespace sanghavi::sanghavimart {
void DbPlugin::initAndStart(const Json::Value& config) {
  std::string rdbms = config.get("rdbms", "sqlite3").asString();
  std::string conn = config.get("connection_string", "sanghavimart.db").asString();
  size_t conns = config.get("connection_number", 4).asUInt();
  if (config.isMember("migrations_dir")) migrations_dir_ = config["migrations_dir"].asString();
  spdlog::info("DbPlugin: rdbms={} conns={}", rdbms, conns);
  if (rdbms == "postgresql")
    client_ = drogon::orm::DbClient::newPgClient(conn, conns);
  else
    client_ = drogon::orm::DbClient::newSqlite3Client(conn, conns);
  RunMigrations();
  std::string provider = "mock", key;
  if (config.isMember("chat_provider")) provider = config["chat_provider"].asString();
  if (const char* k = std::getenv("GEMINI_API_KEY")) key = k;  // server-side env only
  ServiceLocator::Instance().Init(client_, provider, key);
  spdlog::info("DbPlugin: ready (chat_provider={})", provider);
}
void DbPlugin::shutdown() { client_.reset(); }
void DbPlugin::RunMigrations() {
  client_->execSqlSync("CREATE TABLE IF NOT EXISTS schema_migrations(version INTEGER PRIMARY KEY, applied_at TEXT DEFAULT CURRENT_TIMESTAMP)");
  MigrationService m(client_);
  m.ApplyDirectory(migrations_dir_);
}
void MigrationService::ApplyDirectory(const std::string& dir) {
  namespace fs = std::filesystem;
  std::vector<fs::path> files;
  if (!fs::exists(dir)) { spdlog::warn("migrations dir {} missing, skipping", dir); return; }
  for (auto& e : fs::directory_iterator(dir))
    if (e.path().extension() == ".sql") files.push_back(e.path());
  std::sort(files.begin(), files.end());
  for (auto& f : files) {
    int ver = std::stoi(f.filename().string().substr(1, 1));
    auto r = db_->execSqlSync("SELECT version FROM schema_migrations WHERE version=$1", ver);
    if (!r.empty()) continue;
    std::ifstream in(f); std::stringstream ss; ss << in.rdbuf();
    auto tx = db_->newTransaction();
    try {
      tx->execSqlSync(ss.str());
      tx->execSqlSync("INSERT INTO schema_migrations(version) VALUES($1)", ver);
      spdlog::info("migration applied: {}", f.filename().string());
    } catch (const std::exception& e) {
      spdlog::error("migration {} failed: {}", f.filename().string(), e.what());
      throw;
    }
  }
}
}  // namespace sanghavi::sanghavimart
