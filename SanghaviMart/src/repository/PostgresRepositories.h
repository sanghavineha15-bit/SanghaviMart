#pragma once
#include <drogon/orm/DbClient.h>
#include <spdlog/spdlog.h>
#include "Interfaces.h"

namespace sanghavimart::repository {

/// PostgreSQL repositories via Drogon DbClient. ALL SQL PARAMETERIZED ($1..).
/// No string concatenation of user input. LIKE patterns passed as bound params.
class PostgresUserRepository : public IUserRepository {
  public:
    explicit PostgresUserRepository(drogon::orm::DbClientPtr db) : db_(std::move(db)) {}

    int Create(const std::string& name, const std::string& email,
               const std::string& password_hash, const std::string& role) override {
        auto r = db_->execSqlSync(
            "INSERT INTO users (name,email,password_hash,role) VALUES ($1,$2,$3,$4) RETURNING id",
            name, email, password_hash, role);
        return r[0]["id"].as<int>();
    }

    std::optional<model::User> FindByEmail(const std::string& email) override {
        auto r = db_->execSqlSync(
            "SELECT id,name,email,role FROM users WHERE email=$1", email);
        if (r.empty()) return std::nullopt;
        model::User u;
        u.id = r[0]["id"].as<int>();
        u.name = r[0]["name"].as<std::string>();
        u.email = r[0]["email"].as<std::string>();
        u.role = r[0]["role"].as<std::string>();
        return u;
    }

    std::optional<std::string> PasswordHashFor(const std::string& email) override {
        auto r = db_->execSqlSync("SELECT password_hash FROM users WHERE email=$1", email);
        if (r.empty()) return std::nullopt;
        return r[0]["password_hash"].as<std::string>();
    }

    std::vector<model::User> ListAll(const std::string& role_filter) override {
        drogon::orm::Result r;
        if (role_filter == "BUYER" || role_filter == "SELLER" || role_filter == "ADMIN") {
            r = db_->execSqlSync(
                "SELECT id,name,email,role FROM users WHERE role=$1 ORDER BY id", role_filter);
        } else {
            r = db_->execSqlSync("SELECT id,name,email,role FROM users ORDER BY id");
        }
        std::vector<model::User> out;
        for (auto& row : r) {
            model::User u;
            u.id = row["id"].as<int>();
            u.name = row["name"].as<std::string>();
            u.email = row["email"].as<std::string>();
            u.role = row["role"].as<std::string>();
            out.push_back(std::move(u));
        }
        return out;
    }

  private:
    drogon::orm::DbClientPtr db_;
};

class PostgresProductRepository : public IProductRepository {
  public:
    explicit PostgresProductRepository(drogon::orm::DbClientPtr db) : db_(std::move(db)) {}

    int Create(const model::Product& p) override {
        auto r = db_->execSqlSync(
            "INSERT INTO products (seller_id,name,description,price_cents,stock_qty,category,image_url)"
            " VALUES ($1,$2,$3,$4,$5,$6,$7) RETURNING id",
            p.seller_id, p.name, p.description, p.price.MinorUnits(), p.stock_qty,
            p.category, p.image_url);
        return r[0]["id"].as<int>();
    }

    std::optional<model::Product> FindById(int id) override {
        auto r = db_->execSqlSync(
            "SELECT id,seller_id,name,description,price_cents,stock_qty,category,image_url"
            " FROM products WHERE id=$1",
            id);
        if (r.empty()) return std::nullopt;
        return RowToProduct(r[0]);
    }

    std::vector<model::Product> Search(const std::string& keyword, const std::string& category,
                                      int limit = 50) override {
        // Bound ILIKE patterns — never interpolated.
        std::string kw = "%" + keyword + "%";
        std::string cat = "%" + category + "%";
        auto r = db_->execSqlSync(
            "SELECT id,seller_id,name,description,price_cents,stock_qty,category,image_url"
            " FROM products WHERE ($1='' OR name ILIKE $2 OR description ILIKE $2)"
            " AND ($3='' OR category ILIKE $4) ORDER BY id DESC LIMIT $5",
            keyword, kw, category, cat, limit);
        std::vector<model::Product> out;
        for (auto& row : r) out.push_back(RowToProduct(row));
        return out;
    }

    void Update(const model::Product& p) override {
        db_->execSqlSync(
            "UPDATE products SET name=$1,description=$2,price_cents=$3,stock_qty=$4,"
            "category=$5,image_url=$6 WHERE id=$7 AND seller_id=$8",
            p.name, p.description, p.price.MinorUnits(), p.stock_qty, p.category,
            p.image_url, p.id, p.seller_id);
    }

    void Remove(int id) override {
        db_->execSqlSync("DELETE FROM products WHERE id=$1", id);
    }

  private:
    static model::Product RowToProduct(const drogon::orm::Row& row) {
        model::Product p;
        p.id = row["id"].as<int>();
        p.seller_id = row["seller_id"].as<int>();
        p.name = row["name"].as<std::string>();
        p.description = row["description"].as<std::string>();
        p.price = model::Money::FromMinor(row["price_cents"].as<int64_t>());
        p.stock_qty = row["stock_qty"].as<int>();
        p.category = row["category"].as<std::string>();
        p.image_url = row["image_url"].as<std::string>();
        return p;
    }
    drogon::orm::DbClientPtr db_;
};

}  // namespace sanghavimart::repository
