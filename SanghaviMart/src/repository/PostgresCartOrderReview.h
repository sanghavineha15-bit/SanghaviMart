#pragma once
#include <drogon/orm/DbClient.h>
#include "Interfaces.h"

namespace sanghavimart::repository {

// Cart / Order / Review PostgreSQL implementations (parameterized only).
class PostgresCartRepository : public ICartRepository {
  public:
    explicit PostgresCartRepository(drogon::orm::DbClientPtr db) : db_(std::move(db)) {}
    void Upsert(int user_id, int product_id, int qty) override {
        db_->execSqlSync(
            "INSERT INTO cart_items (user_id,product_id,quantity) VALUES ($1,$2,$3)"
            " ON CONFLICT (user_id,product_id) DO UPDATE SET quantity=cart_items.quantity+$3",
            user_id, product_id, qty);
    }
    void SetQuantity(int user_id, int product_id, int qty) override {
        db_->execSqlSync("UPDATE cart_items SET quantity=$1 WHERE user_id=$2 AND product_id=$3",
                         qty, user_id, product_id);
    }
    void Remove(int user_id, int product_id) override {
        db_->execSqlSync("DELETE FROM cart_items WHERE user_id=$1 AND product_id=$2", user_id,
                         product_id);
    }
    void Clear(int user_id) override {
        db_->execSqlSync("DELETE FROM cart_items WHERE user_id=$1", user_id);
    }
    std::vector<model::CartItem> ListForUser(int user_id) override {
        auto r = db_->execSqlSync(
            "SELECT id,user_id,product_id,quantity FROM cart_items WHERE user_id=$1", user_id);
        std::vector<model::CartItem> out;
        for (auto& row : r) {
            model::CartItem c;
            c.id = row["id"].as<int>();
            c.user_id = row["user_id"].as<int>();
            c.product_id = row["product_id"].as<int>();
            c.quantity = row["quantity"].as<int>();
            out.push_back(c);
        }
        return out;
    }

  private:
    drogon::orm::DbClientPtr db_;
};

class PostgresOrderRepository : public IOrderRepository {
  public:
    explicit PostgresOrderRepository(drogon::orm::DbClientPtr db) : db_(std::move(db)) {}

    // Transaction-safe checkout with row-level locking (prevents oversell).
    int CheckoutTransaction(int buyer_id) override {
        auto txn = db_->newTransaction();
        try {
            // Lock cart rows + product rows in deterministic order.
            auto cart = txn->execSqlSync(
                "SELECT ci.product_id, ci.quantity, p.price_cents, p.stock_qty"
                " FROM cart_items ci JOIN products p ON p.id=ci.product_id"
                " WHERE ci.user_id=$1 ORDER BY ci.product_id FOR UPDATE",
                buyer_id);
            if (cart.empty()) throw std::runtime_error("cart empty");
            int64_t total = 0;
            for (auto& row : cart) {
                int qty = row["quantity"].as<int>();
                int stock = row["stock_qty"].as<int>();
                if (qty > stock) throw std::runtime_error("insufficient stock");
                total += row["price_cents"].as<int64_t>() * qty;
            }
            auto o = txn->execSqlSync(
                "INSERT INTO orders (buyer_id,status,total_amount_cents)"
                " VALUES ($1,'PENDING',$2) RETURNING id",
                buyer_id, total);
            int order_id = o[0]["id"].as<int>();
            for (auto& row : cart) {
                int pid = row["product_id"].as<int>();
                int qty = row["quantity"].as<int>();
                int64_t unit = row["price_cents"].as<int64_t>();
                txn->execSqlSync(
                    "INSERT INTO order_items (order_id,product_id,quantity,unit_price_cents)"
                    " VALUES ($1,$2,$3,$4)",
                    order_id, pid, qty, unit);
                auto upd = txn->execSqlSync(
                    "UPDATE products SET stock_qty=stock_qty-$1"
                    " WHERE id=$2 AND stock_qty>=$1",
                    qty, pid);
                if (upd.affectedRows() == 0) throw std::runtime_error("race: stock changed");
            }
            txn->execSqlSync("DELETE FROM cart_items WHERE user_id=$1", buyer_id);
            return order_id;
        } catch (...) {
            txn->rollback();
            throw;
        }
    }

    std::vector<model::Order> OrdersForBuyer(int buyer_id) override {
        auto r = db_->execSqlSync(
            "SELECT id,buyer_id,status,total_amount_cents FROM orders WHERE buyer_id=$1 ORDER BY id DESC",
            buyer_id);
        return RowsToOrders(r);
    }
    std::optional<model::Order> FindById(int order_id) override {
        auto r = db_->execSqlSync(
            "SELECT id,buyer_id,status,total_amount_cents FROM orders WHERE id=$1", order_id);
        if (r.empty()) return std::nullopt;
        return RowsToOrders(r).front();
    }
    std::vector<model::Order> OrdersForSeller(int seller_id) override {
        auto r = db_->execSqlSync(
            "SELECT DISTINCT o.id,o.buyer_id,o.status,o.total_amount_cents FROM orders o"
            " JOIN order_items oi ON oi.order_id=o.id JOIN products p ON p.id=oi.product_id"
            " WHERE p.seller_id=$1 ORDER BY o.id DESC",
            seller_id);
        return RowsToOrders(r);
    }
    std::vector<model::Order> AllOrders() override {
        auto r = db_->execSqlSync(
            "SELECT id,buyer_id,status,total_amount_cents FROM orders ORDER BY id DESC");
        return RowsToOrders(r);
    }
    void SetStatus(int order_id, const std::string& status) override {
        // Server validates transitions; DB enforces CHECK.
        db_->execSqlSync("UPDATE orders SET status=$1 WHERE id=$2", status, order_id);
    }

  private:
    static std::vector<model::Order> RowsToOrders(const drogon::orm::Result& r) {
        std::vector<model::Order> out;
        for (auto& row : r) {
            model::Order o;
            o.id = row["id"].as<int>();
            o.buyer_id = row["buyer_id"].as<int>();
            o.status = row["status"].as<std::string>();
            o.total = model::Money::FromMinor(row["total_amount_cents"].as<int64_t>());
            out.push_back(o);
        }
        return out;
    }
    drogon::orm::DbClientPtr db_;
};

class PostgresReviewRepository : public IReviewRepository {
  public:
    explicit PostgresReviewRepository(drogon::orm::DbClientPtr db) : db_(std::move(db)) {}
    int Add(const model::Review& r) override {
        auto res = db_->execSqlSync(
            "INSERT INTO reviews (product_id,user_id,rating,comment) VALUES ($1,$2,$3,$4)"
            " RETURNING id",
            r.product_id, r.user_id, r.rating, r.comment);
        return res[0]["id"].as<int>();
    }
    bool HasPurchased(int user_id, int product_id) override {
        auto r = db_->execSqlSync(
            "SELECT 1 FROM order_items oi JOIN orders o ON o.id=oi.order_id"
            " WHERE o.buyer_id=$1 AND oi.product_id=$2"
            " AND o.status IN ('CONFIRMED','SHIPPED','DELIVERED') LIMIT 1",
            user_id, product_id);
        return !r.empty();
    }
    std::vector<model::Review> ForProduct(int product_id) override {
        auto r = db_->execSqlSync(
            "SELECT id,product_id,user_id,rating,comment FROM reviews WHERE product_id=$1 ORDER BY id DESC",
            product_id);
        std::vector<model::Review> out;
        for (auto& row : r) {
            model::Review v;
            v.id = row["id"].as<int>();
            v.product_id = row["product_id"].as<int>();
            v.user_id = row["user_id"].as<int>();
            v.rating = row["rating"].as<int>();
            v.comment = row["comment"].as<std::string>();
            out.push_back(v);
        }
        return out;
    }

  private:
    drogon::orm::DbClientPtr db_;
};

}  // namespace sanghavimart::repository
