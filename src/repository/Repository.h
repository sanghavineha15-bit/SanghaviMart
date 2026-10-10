#pragma once
// SanghaviMart Repository interfaces (abstract base classes, pure virtual).
// ALL SQL lives in implementations; services depend ONLY on these interfaces
// (dependency inversion via constructor-injected shared_ptr<Interface>).
// Every query is parameterized — no string-concatenated SQL, ever.
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "model/Entities.h"

namespace sanghavi::sanghavimart {

class IUserRepository {
 public:
  virtual ~IUserRepository() = default;
  virtual User Create(const User& u) = 0;
  virtual std::optional<User> FindById(std::int64_t id) = 0;
  virtual std::optional<User> FindByEmail(const std::string& email) = 0;
  virtual std::vector<User> ListAll() = 0;
};

class IProductRepository {
 public:
  virtual ~IProductRepository() = default;
  virtual Product Create(const Product& p) = 0;
  virtual std::optional<Product> FindById(std::int64_t id) = 0;
  virtual std::vector<Product> Search(const std::string& keyword, const std::string& category,
                                      int limit, int offset) = 0;
  virtual std::vector<Product> ListBySeller(std::int64_t seller_id) = 0;
  virtual void Update(const Product& p) = 0;
  virtual void Remove(std::int64_t id) = 0;
};

class ICartRepository {
 public:
  virtual ~ICartRepository() = default;
  virtual void Upsert(std::int64_t user_id, std::int64_t product_id, std::int64_t qty) = 0;
  virtual void RemoveItem(std::int64_t user_id, std::int64_t product_id) = 0;
  virtual void Clear(std::int64_t user_id) = 0;
  virtual std::vector<CartItem> List(std::int64_t user_id) = 0;
};

class IOrderRepository {
 public:
  virtual ~IOrderRepository() = default;
  virtual Order CreateOrder(const Order& o, const std::vector<OrderItem>& items) = 0;
  virtual std::optional<Order> FindById(std::int64_t id) = 0;
  virtual std::vector<Order> ListByBuyer(std::int64_t buyer_id) = 0;
  virtual std::vector<Order> ListAll() = 0;
  virtual std::vector<OrderItem> ItemsOf(std::int64_t order_id) = 0;
  virtual std::vector<Order> IncomingForSeller(std::int64_t seller_id) = 0;
  virtual void UpdateStatus(std::int64_t order_id, OrderStatus s) = 0;
  virtual std::int64_t SalesCountForSeller(std::int64_t seller_id) = 0;
  virtual std::int64_t RevenueCentsForSeller(std::int64_t seller_id) = 0;
};

class IReviewRepository {
 public:
  virtual ~IReviewRepository() = default;
  virtual Review Create(const Review& r) = 0;
  virtual std::vector<Review> ListByProduct(std::int64_t product_id) = 0;
  virtual bool HasUserOrderedProduct(std::int64_t user_id, std::int64_t product_id) = 0;
  virtual double AvgRating(std::int64_t product_id) = 0;
};

/// Factory — instantiates repository implementations, returning unique_ptr<Interface>.
class RepositoryFactory {
 public:
  static std::unique_ptr<IUserRepository> CreateUsers(drogon::orm::DbClientPtr db);
  static std::unique_ptr<IProductRepository> CreateProducts(drogon::orm::DbClientPtr db);
  static std::unique_ptr<ICartRepository> CreateCart(drogon::orm::DbClientPtr db);
  static std::unique_ptr<IOrderRepository> CreateOrders(drogon::orm::DbClientPtr db);
  static std::unique_ptr<IReviewRepository> CreateReviews(drogon::orm::DbClientPtr db);
};

}  // namespace sanghavi::sanghavimart
