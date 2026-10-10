#pragma once
// SanghaviMart Service layer - business logic + validation (pure C++, no DB calls).
// Services depend on repository INTERFACES via constructor-injected shared_ptr.
#include <cstdint>
#include <drogon/orm/DbClient.h>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include "model/Entities.h"
#include "repository/Repository.h"

namespace sanghavi::sanghavimart {

/// Structured-logging request context passed explicitly controller -> service -> repo.
struct RequestContext {
  std::string request_id{"-"};
};

class AuthService {
 public:
  AuthService(std::shared_ptr<IUserRepository> users, std::string req = "-");
  /// Validates input, checks email uniqueness, Argon2id-hashes password.
  User Register(const std::string& name, const std::string& email,
               const std::string& password, const std::string& role,
               const RequestContext& ctx = {});
  /// Verifies credentials; throws UnauthorizedException on failure.
  User Login(const std::string& email, const std::string& password,
            const RequestContext& ctx = {});
 private:
  std::shared_ptr<IUserRepository> users_;
  std::string req_;
};

class ProductService {
 public:
  explicit ProductService(std::shared_ptr<IProductRepository> products);
  Product Create(std::int64_t seller_id, const std::string& name, const std::string& desc,
                 std::int64_t price_cents, std::int64_t stock, const std::string& cat,
                 const std::string& img, const RequestContext& ctx = {});
  Product Update(std::int64_t seller_id, std::int64_t id, const std::string& name,
                 const std::string& desc, std::int64_t price_cents, std::int64_t stock,
                 const std::string& cat, const std::string& img, bool is_admin = false);
  void Remove(std::int64_t seller_id, std::int64_t id, bool is_admin = false);
  std::vector<Product> Browse(const std::string& keyword, const std::string& category,
                              int limit = 50, int offset = 0);
  std::vector<Product> SellerCatalog(std::int64_t seller_id);
 private:
  std::shared_ptr<IProductRepository> products_;
};

/// Strategy: swappable payment-mock channel behind an interface (config-selected).
class IPaymentChannel {
 public:
  virtual ~IPaymentChannel() = default;
  virtual bool Confirm(const Money& total) = 0;  ///< mock confirmation step
};
class MockPaymentChannel : public IPaymentChannel {
 public:
  bool Confirm(const Money&) override { return true; }
};
class FailingPaymentChannel : public IPaymentChannel {
 public:
  bool Confirm(const Money&) override { return false; }
};

class CartService {
 public:
  CartService(std::shared_ptr<ICartRepository> cart, std::shared_ptr<IProductRepository> products);
  void Add(std::int64_t user_id, std::int64_t product_id, std::int64_t qty);
  void UpdateQty(std::int64_t user_id, std::int64_t product_id, std::int64_t qty);
  void Remove(std::int64_t user_id, std::int64_t product_id);
  std::vector<CartItem> View(std::int64_t user_id);
  Money Total(const std::vector<CartItem>& items);
 private:
  std::shared_ptr<ICartRepository> cart_;
  std::shared_ptr<IProductRepository> products_;
};

class OrderService {
 public:
  OrderService(std::shared_ptr<IOrderRepository> orders, std::shared_ptr<ICartRepository> cart,
               std::shared_ptr<IProductRepository> products,
               std::shared_ptr<IPaymentChannel> pay);
  /// Checkout: places an order from cart contents via mock payment confirmation.
  Order Checkout(std::int64_t buyer_id, const RequestContext& ctx = {});
  std::vector<Order> BuyerHistory(std::int64_t buyer_id);
  std::vector<Order> SellerIncoming(std::int64_t seller_id);
  void AdvanceStatus(std::int64_t actor_id, bool is_admin, std::int64_t order_id,
                     OrderStatus to, const RequestContext& ctx = {});
 private:
  std::shared_ptr<IOrderRepository> orders_;
  std::shared_ptr<ICartRepository> cart_;
  std::shared_ptr<IProductRepository> products_;
  std::shared_ptr<IPaymentChannel> pay_;
};

class ReviewService {
 public:
  ReviewService(std::shared_ptr<IReviewRepository> reviews);
  Review Add(std::int64_t user_id, std::int64_t product_id, int rating,
             const std::string& comment);
  std::vector<Review> ForProduct(std::int64_t product_id);
 private:
  std::shared_ptr<IReviewRepository> reviews_;
};

// ---- AI chatbot (Section 9/15): provider interface + factory ----
class ChatProvider {
 public:
  virtual ~ChatProvider() = default;
  virtual std::string GetReply(const std::string& user_message, const std::string& context) = 0;
};
/// Canned FAQ answers, no network. Default provider + degraded fallback.
class MockChatProvider : public ChatProvider {
 public:
  std::string GetReply(const std::string& msg, const std::string& ctx) override;
};
/// Real LLM call server-side via libcurl (API key from env at startup only).
class GeminiChatProvider : public ChatProvider {
 public:
  explicit GeminiChatProvider(std::string api_key, std::string model = "gemini-1.5-flash");
  std::string GetReply(const std::string& msg, const std::string& ctx) override;
 private:
  std::string api_key_; std::string model_;
};
/// Factory: config flag ai.chatbot.provider = gemini|mock.
class ChatProviderFactory {
 public:
  static std::unique_ptr<ChatProvider> Create(const std::string& name, const std::string& api_key);
};

class ChatService {
 public:
  explicit ChatService(std::unique_ptr<ChatProvider> provider);
  /// Postgres-backed store (Vercel-safe). Null store = in-memory (local dev).
  void SetStore(drogon::orm::DbClientPtr db);
  /// Enforces input cap + per-session rate limit (10/min) + per-session cache.
  std::string Reply(const std::string& session_id, const std::string& message);
 private:
  bool AllowStored(const std::string& sid, std::int64_t now);
  std::unique_ptr<ChatProvider> provider_;
  drogon::orm::DbClientPtr store_;
  std::mutex mutex_;
  std::unordered_map<std::string, std::vector<std::int64_t>> hits_;  ///< session -> epoch secs
  std::unordered_map<std::string, std::unordered_map<std::string, std::string>> cache_;
  static std::string Norm(const std::string& s);
};

}  // namespace sanghavi::sanghavimart
