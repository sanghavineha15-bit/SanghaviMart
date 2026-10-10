#include "controller/Controllers.h"
#include <random>
#include <sstream>
#include "exception/AppException.h"
#include "util/JsonUtil.h"
#include "util/ValidationUtil.h"

namespace sanghavi::sanghavimart {

ServiceLocator& ServiceLocator::Instance() { static ServiceLocator s; return s; }
void ServiceLocator::Init(drogon::orm::DbClientPtr db, std::string chat_provider, std::string gemini_key) {
  db_ = std::move(db);
  users_ = std::shared_ptr<IUserRepository>(RepositoryFactory::CreateUsers(db_));
  auto prods = std::shared_ptr<IProductRepository>(RepositoryFactory::CreateProducts(db_));
  auto cart = std::shared_ptr<ICartRepository>(RepositoryFactory::CreateCart(db_));
  auto ords = std::shared_ptr<IOrderRepository>(RepositoryFactory::CreateOrders(db_));
  auto revs = std::shared_ptr<IReviewRepository>(RepositoryFactory::CreateReviews(db_));
  products_ = prods; cart_ = cart; orders_ = ords; reviews_ = revs;
  auth_ = std::make_shared<AuthService>(users_);
  prod_ = std::make_shared<ProductService>(prods);
  cartsvc_ = std::make_shared<CartService>(cart, prods);
  auto pay = std::make_shared<MockPaymentChannel>();  // Strategy selected by config
  ordsvc_ = std::make_shared<OrderService>(ords, cart, prods, pay);
  revsvc_ = std::make_shared<ReviewService>(revs);
  chat_ = std::make_unique<ChatService>(ChatProviderFactory::Create(chat_provider, gemini_key));
}
std::string ServiceLocator::RequestId(const drogon::HttpRequestPtr& req) const {
  auto v = req->getAttributes()->get<std::string>("request_id");
  return v.empty() ? "-" : v;
}
std::int64_t ServiceLocator::SessionUser(const drogon::HttpRequestPtr& req) const {
  return req->session()->get<std::int64_t>("user_id");
}
std::string ServiceLocator::SessionRole(const drogon::HttpRequestPtr& req) const {
  return req->session()->get<std::string>("role");
}
std::shared_ptr<AuthService> ServiceLocator::Auth() { return auth_; }
std::shared_ptr<ProductService> ServiceLocator::Products() { return prod_; }
std::shared_ptr<CartService> ServiceLocator::Cart() { return cartsvc_; }
std::shared_ptr<OrderService> ServiceLocator::Orders() { return ordsvc_; }
std::shared_ptr<ReviewService> ServiceLocator::Reviews() { return revsvc_; }
std::shared_ptr<IUserRepository> ServiceLocator::Users() { return users_; }
std::shared_ptr<IOrderRepository> ServiceLocator::OrderRepo() { return orders_; }
ChatService* ServiceLocator::Chat() { return chat_.get(); }

namespace {
Json::Value UserJson(const User& u) {
  Json::Value v; v["id"] = Json::Int64(u.id); v["name"] = u.name;
  v["email"] = u.email; v["role"] = ToString(u.role); return v;  // never password_hash
}
Json::Value ProductJson(const Product& p, double avg = 0.0) {
  Json::Value v; v["id"] = Json::Int64(p.id); v["seller_id"] = Json::Int64(p.seller_id);
  v["name"] = p.name; v["description"] = p.description;
  v["price_cents"] = Json::Int64(p.price.Cents()); v["stock_qty"] = Json::Int64(p.stock_qty);
  v["category"] = p.category; v["image_url"] = p.image_url; v["avg_rating"] = avg; return v;
}
void SendOk(std::function<void(const drogon::HttpResponsePtr&)>&& cb, const Json::Value& data,
            drogon::HttpStatusCode code = drogon::k200OK) {
  auto r = drogon::HttpResponse::newHttpJsonResponse(JsonUtil::Ok(data));
  r->setStatusCode(code); cb(r);
}
void SendErr(std::function<void(const drogon::HttpResponsePtr&)>&& cb, const std::exception& e) {
  cb(ToErrorResponse(e));
}
RequestContext Ctx(const drogon::HttpRequestPtr& req) {
  return RequestContext{ServiceLocator::Instance().RequestId(req)};
}
void RequireRole(const drogon::HttpRequestPtr& req, const std::string& role) {
  if (ServiceLocator::Instance().SessionRole(req) != role) {
    if (ServiceLocator::Instance().SessionUser(req) == 0) throw UnauthorizedException();
    throw ForbiddenException("requires role " + role);
  }
}
}  // namespace

void AuthController::Register(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
  try {
    auto j = *req->getJsonObject();
    User u = ServiceLocator::Instance().Auth()->Register(
        JsonUtil::Str(j, "name"), JsonUtil::Str(j, "email"),
        JsonUtil::Str(j, "password"), JsonUtil::Str(j, "role"), Ctx(req));
    SendOk(std::move(cb), UserJson(u), drogon::k201Created);
  } catch (const std::exception& e) { SendErr(std::move(cb), e); }
}
void AuthController::Login(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
  try {
    auto j = *req->getJsonObject();
    User u = ServiceLocator::Instance().Auth()->Login(
        JsonUtil::Str(j, "email"), JsonUtil::Str(j, "password"), Ctx(req));
    req->session()->erase("user_id");  // discard pre-auth session, issue fresh one
    req->session()->insert("user_id", u.id);
    req->session()->insert("role", ToString(u.role));
    SendOk(std::move(cb), UserJson(u));
  } catch (const std::exception& e) { SendErr(std::move(cb), e); }
}
void AuthController::Logout(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
  req->session()->clear();
  Json::Value v; v["ok"] = true; SendOk(std::move(cb), v);
}
void AuthController::Me(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
  try {
    auto id = ServiceLocator::Instance().SessionUser(req);
    if (id == 0) throw UnauthorizedException();
    auto u = ServiceLocator::Instance().Users()->FindById(id);
    if (!u.has_value()) throw UnauthorizedException("session invalid");
    SendOk(std::move(cb), UserJson(*u));
  } catch (const std::exception& e) { SendErr(std::move(cb), e); }
}

void ProductController::List(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
  try {
    auto q = req->getParameters();
    std::string kw = q.count("q") ? q.at("q") : "";
    std::string cat = q.count("category") ? q.at("category") : "";
    auto items = ServiceLocator::Instance().Products()->Browse(kw, cat);
    Json::Value arr(Json::arrayValue);
    for (const auto& p : items) arr.append(ProductJson(p));
    SendOk(std::move(cb), arr);
  } catch (const std::exception& e) { SendErr(std::move(cb), e); }
}
void ProductController::Get(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, std::int64_t id) {
  try {
    (void)req;
    auto repo = ServiceLocator::Instance().Products();
    auto items = repo->Browse("", "", 1000000, 0);
    for (const auto& p : items) if (p.id == id) { SendOk(std::move(cb), ProductJson(p)); return; }
    throw NotFoundException("product not found");
  } catch (const std::exception& e) { SendErr(std::move(cb), e); }
}
void ProductController::Create(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
  try {
    RequireRole(req, "SELLER");
    auto j = *req->getJsonObject();
    Product p = ServiceLocator::Instance().Products()->Create(
        ServiceLocator::Instance().SessionUser(req), JsonUtil::Str(j, "name"),
        JsonUtil::Str(j, "description", false), JsonUtil::Int64(j, "price_cents"),
        JsonUtil::Int64(j, "stock_qty"), JsonUtil::Str(j, "category", false),
        JsonUtil::Str(j, "image_url", false), Ctx(req));
    SendOk(std::move(cb), ProductJson(p), drogon::k201Created);
  } catch (const std::exception& e) { SendErr(std::move(cb), e); }
}
void ProductController::Update(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, std::int64_t id) {
  try {
    RequireRole(req, "SELLER");
    auto j = *req->getJsonObject();
    Product p = ServiceLocator::Instance().Products()->Update(
        ServiceLocator::Instance().SessionUser(req), id, JsonUtil::Str(j, "name"),
        JsonUtil::Str(j, "description", false), JsonUtil::Int64(j, "price_cents"),
        JsonUtil::Int64(j, "stock_qty"), JsonUtil::Str(j, "category", false),
        JsonUtil::Str(j, "image_url", false));
    SendOk(std::move(cb), ProductJson(p));
  } catch (const std::exception& e) { SendErr(std::move(cb), e); }
}
void ProductController::Remove(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, std::int64_t id) {
  try {
    RequireRole(req, "SELLER");
    ServiceLocator::Instance().Products()->Remove(ServiceLocator::Instance().SessionUser(req), id);
    Json::Value v; v["ok"] = true; SendOk(std::move(cb), v);
  } catch (const std::exception& e) { SendErr(std::move(cb), e); }
}
void ProductController::Mine(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
  try {
    RequireRole(req, "SELLER");
    auto items = ServiceLocator::Instance().Products()->SellerCatalog(ServiceLocator::Instance().SessionUser(req));
    Json::Value arr(Json::arrayValue);
    for (const auto& p : items) arr.append(ProductJson(p));
    SendOk(std::move(cb), arr);
  } catch (const std::exception& e) { SendErr(std::move(cb), e); }
}
void ProductController::Dashboard(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
  try {
    RequireRole(req, "SELLER");
    auto sid = ServiceLocator::Instance().SessionUser(req);
    auto repo = ServiceLocator::Instance().OrderRepo();
    Json::Value v;
    v["order_count"] = Json::Int64(repo->SalesCountForSeller(sid));
    v["revenue_cents"] = Json::Int64(repo->RevenueCentsForSeller(sid));
    SendOk(std::move(cb), v);
  } catch (const std::exception& e) { SendErr(std::move(cb), e); }
}

void CartController::View(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
  try {
    auto uid = ServiceLocator::Instance().SessionUser(req);
    if (uid == 0) throw UnauthorizedException();
    auto items = ServiceLocator::Instance().Cart()->View(uid);
    Json::Value arr(Json::arrayValue);
    for (const auto& c : items) {
      Json::Value v; v["product_id"] = Json::Int64(c.product_id); v["name"] = c.product_name;
      v["quantity"] = Json::Int64(c.quantity); v["unit_price_cents"] = Json::Int64(c.unit_price.Cents());
      v["line_total_cents"] = Json::Int64((c.unit_price * c.quantity).Cents()); arr.append(v);
    }
    Json::Value body; body["items"] = arr;
    body["total_cents"] = Json::Int64(ServiceLocator::Instance().Cart()->Total(items).Cents());
    SendOk(std::move(cb), body);
  } catch (const std::exception& e) { SendErr(std::move(cb), e); }
}
void CartController::Add(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
  try {
    auto uid = ServiceLocator::Instance().SessionUser(req);
    if (uid == 0) throw UnauthorizedException();
    auto j = *req->getJsonObject();
    ServiceLocator::Instance().Cart()->Add(uid, JsonUtil::Int64(j, "product_id"), JsonUtil::Int64(j, "quantity"));
    Json::Value v; v["ok"] = true; SendOk(std::move(cb), v, drogon::k201Created);
  } catch (const std::exception& e) { SendErr(std::move(cb), e); }
}
void CartController::Update(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, std::int64_t id) {
  try {
    auto uid = ServiceLocator::Instance().SessionUser(req);
    if (uid == 0) throw UnauthorizedException();
    auto j = *req->getJsonObject();
    ServiceLocator::Instance().Cart()->UpdateQty(uid, id, JsonUtil::Int64(j, "quantity"));
    Json::Value v; v["ok"] = true; SendOk(std::move(cb), v);
  } catch (const std::exception& e) { SendErr(std::move(cb), e); }
}
void CartController::Remove(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, std::int64_t id) {
  try {
    auto uid = ServiceLocator::Instance().SessionUser(req);
    if (uid == 0) throw UnauthorizedException();
    ServiceLocator::Instance().Cart()->Remove(uid, id);
    Json::Value v; v["ok"] = true; SendOk(std::move(cb), v);
  } catch (const std::exception& e) { SendErr(std::move(cb), e); }
}

void OrderController::Checkout(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
  try {
    auto uid = ServiceLocator::Instance().SessionUser(req);
    if (uid == 0) throw UnauthorizedException();
    Order o = ServiceLocator::Instance().Orders()->Checkout(uid, Ctx(req));
    Json::Value v; v["id"] = Json::Int64(o.id); v["status"] = ToString(o.status);
    v["total_amount_cents"] = Json::Int64(o.total.Cents()); SendOk(std::move(cb), v, drogon::k201Created);
  } catch (const std::exception& e) { SendErr(std::move(cb), e); }
}
void OrderController::Mine(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
  try {
    auto uid = ServiceLocator::Instance().SessionUser(req);
    if (uid == 0) throw UnauthorizedException();
    Json::Value arr(Json::arrayValue);
    for (const auto& o : ServiceLocator::Instance().Orders()->BuyerHistory(uid)) {
      Json::Value v; v["id"] = Json::Int64(o.id); v["status"] = ToString(o.status);
      v["total_amount_cents"] = Json::Int64(o.total.Cents()); v["created_at"] = o.created_at; arr.append(v);
    }
    SendOk(std::move(cb), arr);
  } catch (const std::exception& e) { SendErr(std::move(cb), e); }
}
void OrderController::Incoming(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
  try {
    RequireRole(req, "SELLER");
    Json::Value arr(Json::arrayValue);
    for (const auto& o : ServiceLocator::Instance().Orders()->SellerIncoming(ServiceLocator::Instance().SessionUser(req))) {
      Json::Value v; v["id"] = Json::Int64(o.id); v["status"] = ToString(o.status);
      v["total_amount_cents"] = Json::Int64(o.total.Cents()); v["created_at"] = o.created_at; arr.append(v);
    }
    SendOk(std::move(cb), arr);
  } catch (const std::exception& e) { SendErr(std::move(cb), e); }
}
void OrderController::Advance(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, std::int64_t id) {
  try {
    auto uid = ServiceLocator::Instance().SessionUser(req);
    if (uid == 0) throw UnauthorizedException();
    bool admin = ServiceLocator::Instance().SessionRole(req) == "ADMIN";
    auto j = *req->getJsonObject();
    ServiceLocator::Instance().Orders()->AdvanceStatus(uid, admin, id, StatusFromString(JsonUtil::Str(j, "status")), Ctx(req));
    Json::Value v; v["ok"] = true; SendOk(std::move(cb), v);
  } catch (const std::exception& e) { SendErr(std::move(cb), e); }
}

void ReviewController::List(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, std::int64_t id) {
  try {
    (void)req;
    Json::Value arr(Json::arrayValue);
    for (const auto& r : ServiceLocator::Instance().Reviews()->ForProduct(id)) {
      Json::Value v; v["id"] = Json::Int64(r.id); v["rating"] = r.rating; v["comment"] = r.comment;
      v["created_at"] = r.created_at; arr.append(v);
    }
    SendOk(std::move(cb), arr);
  } catch (const std::exception& e) { SendErr(std::move(cb), e); }
}
void ReviewController::Add(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, std::int64_t id) {
  try {
    auto uid = ServiceLocator::Instance().SessionUser(req);
    if (uid == 0) throw UnauthorizedException();
    auto j = *req->getJsonObject();
    Review r = ServiceLocator::Instance().Reviews()->Add(uid, id, JsonUtil::Int(j, "rating"), JsonUtil::Str(j, "comment"));
    Json::Value v; v["id"] = Json::Int64(r.id); SendOk(std::move(cb), v, drogon::k201Created);
  } catch (const std::exception& e) { SendErr(std::move(cb), e); }
}

void AdminController::Users(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
  try {
    RequireRole(req, "ADMIN");
    Json::Value arr(Json::arrayValue);
    for (const auto& u : ServiceLocator::Instance().Users()->ListAll()) arr.append(UserJson(u));
    SendOk(std::move(cb), arr);
  } catch (const std::exception& e) { SendErr(std::move(cb), e); }
}
void AdminController::Orders(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
  try {
    RequireRole(req, "ADMIN");
    Json::Value arr(Json::arrayValue);
    for (const auto& o : ServiceLocator::Instance().OrderRepo()->ListAll()) {
      Json::Value v; v["id"] = Json::Int64(o.id); v["buyer_id"] = Json::Int64(o.buyer_id);
      v["status"] = ToString(o.status); v["total_amount_cents"] = Json::Int64(o.total.Cents()); arr.append(v);
    }
    SendOk(std::move(cb), arr);
  } catch (const std::exception& e) { SendErr(std::move(cb), e); }
}
void AdminController::RemoveListing(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb, std::int64_t id) {
  try {
    RequireRole(req, "ADMIN");
    ServiceLocator::Instance().Products()->Remove(0, id, true);
    Json::Value v; v["ok"] = true; SendOk(std::move(cb), v);
  } catch (const std::exception& e) { SendErr(std::move(cb), e); }
}

void ChatController::Chat(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
  try {
    auto j = *req->getJsonObject();
    std::string reply = ServiceLocator::Instance().Chat()->Reply(
        req->session()->get<std::string>("session_id"), JsonUtil::Str(j, "message"));
    Json::Value v; v["reply"] = reply; SendOk(std::move(cb), v);
  } catch (const std::exception& e) { SendErr(std::move(cb), e); }
}

void HealthController::Health(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
  (void)req;
  try {
    ServiceLocator::Instance().Db()->execSqlSync("SELECT 1");
    Json::Value v; v["status"] = "UP"; v["db"] = "UP"; SendOk(std::move(cb), v);
  } catch (...) {
    Json::Value v; v["status"] = "DOWN"; v["db"] = "DOWN";
    auto r = drogon::HttpResponse::newHttpJsonResponse(JsonUtil::Ok(v));
    r->setStatusCode(drogon::k503ServiceUnavailable); cb(r);
  }
}

}  // namespace sanghavi::sanghavimart
