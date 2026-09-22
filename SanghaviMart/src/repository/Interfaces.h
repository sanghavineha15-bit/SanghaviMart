#pragma once
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "../model/Entities.h"

namespace sanghavimart::repository {

struct IUserRepository {
    virtual ~IUserRepository() = default;
    virtual int Create(const std::string& name, const std::string& email,
                       const std::string& password_hash, const std::string& role) = 0;
    virtual std::optional<model::User> FindByEmail(const std::string& email) = 0;
    virtual std::optional<std::string> PasswordHashFor(const std::string& email) = 0;
    virtual std::vector<model::User> ListAll(const std::string& role_filter) = 0;
};

struct IProductRepository {
    virtual ~IProductRepository() = default;
    virtual int Create(const model::Product& p) = 0;
    virtual std::optional<model::Product> FindById(int id) = 0;
    virtual std::vector<model::Product> Search(const std::string& keyword,
                                              const std::string& category, int limit = 50) = 0;
    virtual void Update(const model::Product& p) = 0;
    virtual void Remove(int id) = 0;
};

struct ICartRepository {
    virtual ~ICartRepository() = default;
    virtual void Upsert(int user_id, int product_id, int qty) = 0;
    virtual void SetQuantity(int user_id, int product_id, int qty) = 0;
    virtual void Remove(int user_id, int product_id) = 0;
    virtual void Clear(int user_id) = 0;
    virtual std::vector<model::CartItem> ListForUser(int user_id) = 0;
};

struct IOrderRepository {
    virtual ~IOrderRepository() = default;
    // Transaction-safe checkout is implemented in CheckoutService using
    // SELECT ... FOR UPDATE + single DB transaction. Returns new order id.
    virtual int CheckoutTransaction(int buyer_id) = 0;
    virtual std::optional<model::Order> FindById(int order_id) = 0;
    virtual std::vector<model::Order> OrdersForBuyer(int buyer_id) = 0;
    virtual std::vector<model::Order> OrdersForSeller(int seller_id) = 0;
    virtual std::vector<model::Order> AllOrders() = 0;
    virtual void SetStatus(int order_id, const std::string& status) = 0;
};

struct IReviewRepository {
    virtual ~IReviewRepository() = default;
    virtual int Add(const model::Review& r) = 0;
    virtual bool HasPurchased(int user_id, int product_id) = 0;
    virtual std::vector<model::Review> ForProduct(int product_id) = 0;
};

}  // namespace sanghavimart::repository
