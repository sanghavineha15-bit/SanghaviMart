#pragma once
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <pqxx/pqxx.h>
#include <spdlog/spdlog.h>

namespace sanghavimart::service {

/// @brief Numbered PostgreSQL migration runner (transactions + rollback).
class MigrationService {
  public:
    explicit MigrationService(std::string conninfo, std::string migrations_dir)
        : conninfo_(std::move(conninfo)), dir_(std::move(migrations_dir)) {}

    int Run() {
        namespace fs = std::filesystem;
        pqxx::connection c(conninfo_);
        pqxx::work w(c);
        w.exec("CREATE TABLE IF NOT EXISTS schema_migrations"
               " (version VARCHAR(32) PRIMARY KEY, applied_at TIMESTAMPTZ DEFAULT now())");
        w.commit();
        std::vector<std::string> files;
        for (auto& e : fs::directory_iterator(dir_)) {
            if (e.path().extension() == ".sql") files.push_back(e.path().filename().string());
        }
        std::sort(files.begin(), files.end());
        int applied = 0;
        for (auto& f : files) {
            std::string version = f.substr(0, f.find("__"));
            pqxx::work check(c);
            auto r = check.exec_params("SELECT 1 FROM schema_migrations WHERE version=$1", version);
            check.commit();
            if (!r.empty()) continue;
            std::ifstream in(fs::path(dir_) / f);
            std::string sql((std::istreambuf_iterator<char>(in)),
                            std::istreambuf_iterator<char>());
            try {
                pqxx::work txn(c);  // RAII: rollback on failure
                txn.exec(sql);
                txn.exec_params("INSERT INTO schema_migrations (version) VALUES ($1)", version);
                txn.commit();
                spdlog::info("migration applied: {}", f);
                ++applied;
            } catch (const std::exception& e) {
                spdlog::error("migration {} failed: {}", f, e.what());
                throw;
            }
        }
        return applied;
    }

  private:
    std::string conninfo_;
    std::string dir_;
};

}  // namespace sanghavimart::service
