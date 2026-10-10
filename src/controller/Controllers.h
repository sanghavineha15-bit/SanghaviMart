#pragma once
// SanghaviMart Controllers — thin HTTP orchestration only (no SQL, no business logic).
// Routes versioned under /api/v1. Front Controller = Drogon router + BaseController.
#include <drogon/HttpController.h>
#include <memory>
#include "service/Services.h"
#include "repository/Repository.h"

namespace sanghavi::sanghavimart {

/// Shared accessors: per-request DbClient, services, session user.
class ServiceLocator {
 public:
  static ServiceLocator& Instance();
  void Init(drogon::orm::DbClientPtr db, std::string chat_provider, std::string gemini_key);
  drogon::orm::DbClientPtr Db() const { return db_; }
  std::string RequestId(const drogon::HttpRequestPtr& req) const;
  std::int64_t SessionUser(const drogon::HttpRequestPtr& req) const;
  std::string SessionRole(const drogon::HttpRequestPtr& req) const;
  std::shared_ptr<AuthService> Auth();
  std::shared_ptr<ProductService> Products();
  std::shared_ptr<CartService> Cart();
  std::shared_ptr<OrderService> Orders();
  std::shared_ptr<ReviewService> Reviews();
  std::shared_ptr<IUserRepository> Users();
  std::shared_ptr<IOrderRepository> OrderRepo();
  ChatService* Chat();
 private:
  ServiceLocator() = default;
  drogon::orm::DbClientPtr db_;
  std::shared_ptr<IUserRepository> users_; std::shared_ptr<IProductRepository> products_;
  std::shared_ptr<ICartRepository> cart_; std::shared_ptr<IOrderRepository> orders_;
  std::shared_ptr<IReviewRepository> reviews_;
  std::shared_ptr<AuthService> auth_; std::shared_ptr<ProductService> prod_;
  std::shared_ptr<CartService> cartsvc_; std::shared_ptr<OrderService> ordsvc_;
  std::shared_ptr<ReviewService> revsvc_; std::unique_ptr<ChatService> chat_;
};

class AuthController : public drogon::HttpController<AuthController> {
 public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(AuthController::Register, "/api/v1/auth/register", drogon::Post);
  ADD_METHOD_TO(AuthController::Login, "/api/v1/auth/login", drogon::Post);
  ADD_METHOD_TO(AuthController::Logout, "/api/v1/auth/logout", drogon::Post);
  ADD_METHOD_TO(AuthController::Me, "/api/v1/auth/me", drogon::Get);
  METHOD_LIST_END
  void Register(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb);
  void Login(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb);
  void Logout(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb);
  void Me(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb);
};

class ProductController : public drogon::HttpController<ProductController> {
 public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(ProductController::List, "/api/v1/products", drogon::Get);
  ADD_METHOD_TO(ProductController::Get, "/api/v1/products/{id}", drogon::Get);
  ADD_METHOD_TO(ProductController::Create, "/api/v1/products", drogon::Post);
  ADD_METHOD_TO(ProductController::Update, "/api/v1/products/{id}", drogon::Put);
  ADD_METHOD_TO(ProductController::Remove, "/api/v1/products/{id}", drogon::Delete);
  ADD_METHOD_TO(ProductController::Mine, "/api/v1/seller/products", drogon::Get);
  ADD_METHOD_TO(ProductController::Dashboard, "/api/v1/seller/dashboard", drogon::Get);
  METHOD_LIST_END
  void List(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb);
  void Get(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, std::int64_t id);
  void Create(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb);
  void Update(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, std::int64_t id);
  void Remove(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, std::int64_t id);
  void Mine(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb);
  void Dashboard(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb);
};

class CartController : public drogon::HttpController<CartController> {
 public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(CartController::View, "/api/v1/cart", drogon::Get);
  ADD_METHOD_TO(CartController::Add, "/api/v1/cart", drogon::Post);
  ADD_METHOD_TO(CartController::Update, "/api/v1/cart/{id}", drogon::Put);
  ADD_METHOD_TO(CartController::Remove, "/api/v1/cart/{id}", drogon::Delete);
  METHOD_LIST_END
  void View(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb);
  void Add(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb);
  void Update(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, std::int64_t id);
  void Remove(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, std::int64_t id);
};

class OrderController : public drogon::HttpController<OrderController> {
 public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(OrderController::Checkout, "/api/v1/orders/checkout", drogon::Post);
  ADD_METHOD_TO(OrderController::Mine, "/api/v1/orders", drogon::Get);
  ADD_METHOD_TO(OrderController::Incoming, "/api/v1/seller/orders", drogon::Get);
  ADD_METHOD_TO(OrderController::Advance, "/api/v1/orders/{id}/status", drogon::Put);
  METHOD_LIST_END
  void Checkout(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb);
  void Mine(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb);
  void Incoming(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb);
  void Advance(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, std::int64_t id);
};

class ReviewController : public drogon::HttpController<ReviewController> {
 public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(ReviewController::List, "/api/v1/products/{id}/reviews", drogon::Get);
  ADD_METHOD_TO(ReviewController::Add, "/api/v1/products/{id}/reviews", drogon::Post);
  METHOD_LIST_END
  void List(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, std::int64_t id);
  void Add(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, std::int64_t id);
};

class AdminController : public drogon::HttpController<AdminController> {
 public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(AdminController::Users, "/api/v1/admin/users", drogon::Get);
  ADD_METHOD_TO(AdminController::Orders, "/api/v1/admin/orders", drogon::Get);
  ADD_METHOD_TO(AdminController::RemoveListing, "/api/v1/admin/products/{id}", drogon::Delete);
  METHOD_LIST_END
  void Users(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb);
  void Orders(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb);
  void RemoveListing(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, std::int64_t id);
};

class ChatController : public drogon::HttpController<ChatController> {
 public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(ChatController::Chat, "/api/v1/chat", drogon::Post);
  METHOD_LIST_END
  void Chat(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb);
};

class HealthController : public drogon::HttpController<HealthController> {
 public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(HealthController::Health, "/api/v1/health", drogon::Get);
  METHOD_LIST_END
  void Health(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb);
};

}  // namespace sanghavi::sanghavimart
