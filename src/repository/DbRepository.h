#pragma once
// Concrete Drogon-ORM implementations. One API for both PostgreSQL and SQLite.
#include <drogon/orm/DbClient.h>
#include "repository/Repository.h"
namespace sanghavi::sanghavimart {
class DbUserRepository : public IUserRepository {
 public:
  explicit DbUserRepository(drogon::orm::DbClientPtr db) : db_(std::move(db)) {}
  User Create(const User& u) override;
  std::optional<User> FindById(std::int64_t id) override;
  std::optional<User> FindByEmail(const std::string& email) override;
  std::vector<User> ListAll() override;
 private:
  drogon::orm::DbClientPtr db_;
};
class DbProductRepository : public IProductRepository {
 public:
  explicit DbProductRepository(drogon::orm::DbClientPtr db) : db_(std::move(db)) {}
  Product Create(const Product& p) override;
  std::optional<Product> FindById(std::int64_t id) override;
  std::vector<Product> Search(const std::string& kw, const std::string& cat, int lim, int off) override;
  std::vector<Product> ListBySeller(std::int64_t sid) override;
  void Update(const Product& p) override;
  void Remove(std::int64_t id) override;
 private:
  drogon::orm::DbClientPtr db_;
};
class DbCartRepository : public ICartRepository {
 public:
  explicit DbCartRepository(drogon::orm::DbClientPtr db) : db_(std::move(db)) {}
  void Upsert(std::int64_t uid, std::int64_t pid, std::int64_t qty) override;
  void RemoveItem(std::int64_t uid, std::int64_t pid) override;
  void Clear(std::int64_t uid) override;
  std::vector<CartItem> List(std::int64_t uid) override;
 private:
  drogon::orm::DbClientPtr db_;
};
class DbOrderRepository : public IOrderRepository {
 public:
  explicit DbOrderRepository(drogon::orm::DbClientPtr db) : db_(std::move(db)) {}
  Order CreateOrder(const Order& o, const std::vector<OrderItem>& items) override;
  std::optional<Order> FindById(std::int64_t id) override;
  std::vector<Order> ListByBuyer(std::int64_t bid) override;
  std::vector<Order> ListAll() override;
  std::vector<OrderItem> ItemsOf(std::int64_t oid) override;
  std::vector<Order> IncomingForSeller(std::int64_t sid) override;
  void UpdateStatus(std::int64_t oid, OrderStatus s) override;
  std::int64_t SalesCountForSeller(std::int64_t sid) override;
  std::int64_t RevenueCentsForSeller(std::int64_t sid) override;
 private:
  drogon::orm::DbClientPtr db_;
};
class DbReviewRepository : public IReviewRepository {
 public:
  explicit DbReviewRepository(drogon::orm::DbClientPtr db) : db_(std::move(db)) {}
  Review Create(const Review& r) override;
  std::vector<Review> ListByProduct(std::int64_t pid) override;
  bool HasUserOrderedProduct(std::int64_t uid, std::int64_t pid) override;
  double AvgRating(std::int64_t pid) override;
 private:
  drogon::orm::DbClientPtr db_;
};
}  // namespace sanghavi::sanghavimart
