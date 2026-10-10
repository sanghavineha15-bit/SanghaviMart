#include "service/Services.h"
#include <algorithm>
#include <chrono>
#include <cctype>
#include <curl/curl.h>
#include <spdlog/spdlog.h>
#include "exception/AppException.h"
#include "util/PasswordUtil.h"
#include "util/ValidationUtil.h"

namespace sanghavi::sanghavimart {
namespace {
std::int64_t NowSec() {
  return std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::system_clock::now().time_since_epoch()).count();
}
size_t CurlWrite(char* p, size_t s, size_t n, void* u) {
  auto* str = static_cast<std::string*>(u);
  str->append(p, s * n); return s * n;
}
constexpr const char* kChatPrompt =
  "You are SanghaviMart support. Answer ONLY product/listing/order/shipping/returns "
  "questions about the SanghaviMart marketplace. Refuse anything off-topic in one sentence.";
}  // namespace

AuthService::AuthService(std::shared_ptr<IUserRepository> users, std::string req)
    : users_(std::move(users)), req_(std::move(req)) {}
User AuthService::Register(const std::string& name, const std::string& email,
                           const std::string& password, const std::string& role,
                           const RequestContext& ctx) {
  ValidationUtil::RequireNonEmpty("name", name);
  ValidationUtil::RequireEmail(email);
  ValidationUtil::RequirePassword(password);
  ValidationUtil::RequireRole(role);
  spdlog::info("[{}] register email={}", ctx.request_id, email);
  if (users_->FindByEmail(email).has_value()) throw ConflictException("email already registered");
  User u; u.name = ValidationUtil::Trim(name); u.email = ValidationUtil::Trim(email);
  u.password_hash = PasswordUtil::Hash(password);  // Argon2id, never logged
  u.role = RoleFromString(role);
  return users_->Create(u);
}
User AuthService::Login(const std::string& email, const std::string& password,
                        const RequestContext& ctx) {
  ValidationUtil::RequireEmail(email);
  ValidationUtil::RequireNonEmpty("password", password);
  auto u = users_->FindByEmail(ValidationUtil::Trim(email));
  if (!u.has_value() || !PasswordUtil::Verify(u->password_hash, password)) {
    spdlog::warn("[{}] failed login email={}", ctx.request_id, email);
    throw UnauthorizedException("invalid email or password");
  }
  spdlog::info("[{}] login user_id={}", ctx.request_id, u->id);
  return *u;
}

ProductService::ProductService(std::shared_ptr<IProductRepository> p) : products_(std::move(p)) {}
Product ProductService::Create(std::int64_t seller_id, const std::string& name, const std::string& desc,
                               std::int64_t price_cents, std::int64_t stock, const std::string& cat,
                               const std::string& img, const RequestContext&) {
  ValidationUtil::RequireNonEmpty("name", name);
  ValidationUtil::RequirePriceCents(price_cents);
  ValidationUtil::RequireStockQty(stock);
  Product p; p.seller_id = seller_id; p.name = ValidationUtil::Trim(name); p.description = desc;
  p.price = Money::FromCents(price_cents); p.stock_qty = stock;
  p.category = ValidationUtil::Trim(cat); p.image_url = ValidationUtil::Trim(img);
  return products_->Create(p);
}
Product ProductService::Update(std::int64_t seller_id, std::int64_t id, const std::string& name,
                               const std::string& desc, std::int64_t price_cents, std::int64_t stock,
                               const std::string& cat, const std::string& img, bool is_admin) {
  auto cur = products_->FindById(id);
  if (!cur.has_value()) throw NotFoundException("product not found");
  if (!is_admin && cur->seller_id != seller_id) throw ForbiddenException("not your listing");
  ValidationUtil::RequireNonEmpty("name", name);
  ValidationUtil::RequirePriceCents(price_cents);
  ValidationUtil::RequireStockQty(stock);
  cur->name = ValidationUtil::Trim(name); cur->description = desc;
  cur->price = Money::FromCents(price_cents); cur->stock_qty = stock;
  cur->category = ValidationUtil::Trim(cat); cur->image_url = ValidationUtil::Trim(img);
  products_->Update(*cur); return *cur;
}
void ProductService::Remove(std::int64_t seller_id, std::int64_t id, bool is_admin) {
  auto cur = products_->FindById(id);
  if (!cur.has_value()) throw NotFoundException("product not found");
  if (!is_admin && cur->seller_id != seller_id) throw ForbiddenException("not your listing");
  products_->Remove(id);
}
std::vector<Product> ProductService::Browse(const std::string& kw, const std::string& cat, int lim, int off) {
  lim = std::clamp(lim, 1, 100); off = std::max(0, off);
  return products_->Search(kw, cat, lim, off);
}
std::vector<Product> ProductService::SellerCatalog(std::int64_t sid) {
  return products_->ListBySeller(sid);
}

CartService::CartService(std::shared_ptr<ICartRepository> c, std::shared_ptr<IProductRepository> p)
    : cart_(std::move(c)), products_(std::move(p)) {}
void CartService::Add(std::int64_t uid, std::int64_t pid, std::int64_t qty) {
  ValidationUtil::RequireQuantity(qty);
  auto p = products_->FindById(pid);
  if (!p.has_value()) throw NotFoundException("product not found");
  if (p->stock_qty < qty) throw ConflictException("insufficient stock");
  cart_->Upsert(uid, pid, qty);
}
void CartService::UpdateQty(std::int64_t uid, std::int64_t pid, std::int64_t qty) {
  if (qty <= 0) { cart_->RemoveItem(uid, pid); return; }
  Add(uid, pid, qty);
}
void CartService::Remove(std::int64_t uid, std::int64_t pid) { cart_->RemoveItem(uid, pid); }
std::vector<CartItem> CartService::View(std::int64_t uid) { return cart_->List(uid); }
Money CartService::Total(const std::vector<CartItem>& items) {
  Money t = Money::FromCents(0);
  for (const auto& i : items) t = t + (i.unit_price * i.quantity);
  return t;
}

OrderService::OrderService(std::shared_ptr<IOrderRepository> o, std::shared_ptr<ICartRepository> c,
                           std::shared_ptr<IProductRepository> p, std::shared_ptr<IPaymentChannel> pay)
    : orders_(std::move(o)), cart_(std::move(c)), products_(std::move(p)), pay_(std::move(pay)) {}
Order OrderService::Checkout(std::int64_t buyer_id, const RequestContext& ctx) {
  auto items = cart_->List(buyer_id);
  if (items.empty()) throw ConflictException("cart is empty");
  Money total = Money::FromCents(0);
  std::vector<OrderItem> lines;
  for (const auto& c : items) {
    auto p = products_->FindById(c.product_id);
    if (!p.has_value()) throw NotFoundException("product in cart no longer exists");
    if (p->stock_qty < c.quantity) throw ConflictException("insufficient stock for " + p->name);
    total = total + (p->price * c.quantity);
    OrderItem l; l.product_id = c.product_id; l.quantity = c.quantity; l.unit_price = p->price;
    lines.push_back(l);
  }
  spdlog::info("[{}] mock-payment confirm buyer={} total_cents={}", ctx.request_id, buyer_id, total.Cents());
  if (!pay_->Confirm(total)) throw ConflictException("mock payment declined");
  Order o; o.buyer_id = buyer_id; o.status = OrderStatus::kPending; o.total = total;
  Order created = orders_->CreateOrder(o, lines);
  spdlog::info("[{}] order placed id={}", ctx.request_id, created.id);
  return created;
}
std::vector<Order> OrderService::BuyerHistory(std::int64_t bid) { return orders_->ListByBuyer(bid); }
std::vector<Order> OrderService::SellerIncoming(std::int64_t sid) { return orders_->IncomingForSeller(sid); }
void OrderService::AdvanceStatus(std::int64_t actor_id, bool is_admin, std::int64_t oid,
                                 OrderStatus to, const RequestContext& ctx) {
  auto o = orders_->FindById(oid);
  if (!o.has_value()) throw NotFoundException("order not found");
  if (!is_admin) {
    bool owns = false;
    for (const auto& sord : orders_->IncomingForSeller(actor_id))
      if (sord.id == oid) { owns = true; break; }
    if (o->buyer_id != actor_id && !owns) throw ForbiddenException("no access to order");
  }
  if (!IsValidStatusTransition(o->status, to))
    throw ConflictException("illegal status transition " + ToString(o->status) + " -> " + ToString(to));
  spdlog::info("[{}] order {} {} -> {}", ctx.request_id, oid, ToString(o->status), ToString(to));
  orders_->UpdateStatus(oid, to);
}

ReviewService::ReviewService(std::shared_ptr<IReviewRepository> r) : reviews_(std::move(r)) {}
Review ReviewService::Add(std::int64_t uid, std::int64_t pid, int rating, const std::string& comment) {
  ValidationUtil::RequireRating(rating);
  ValidationUtil::RequireNonEmpty("comment", comment);
  if (!reviews_->HasUserOrderedProduct(uid, pid))
    throw ForbiddenException("reviews allowed only on completed orders of this product");
  Review r; r.user_id = uid; r.product_id = pid; r.rating = rating; r.comment = comment;
  return reviews_->Create(r);
}
std::vector<Review> ReviewService::ForProduct(std::int64_t pid) { return reviews_->ListByProduct(pid); }

// ---- chatbot ----
std::string MockChatProvider::GetReply(const std::string& msg, const std::string&) {
  std::string m = msg; std::transform(m.begin(), m.end(), m.begin(), ::tolower);
  if (m.find("return") != std::string::npos) return "SanghaviMart returns: unused items can be returned within 7 days for a full refund to the original mock-payment method.";
  if (m.find("ship") != std::string::npos) return "Orders flow Pending -> Confirmed -> Shipped -> Delivered. Shipping usually takes 3-5 days after confirmation.";
  if (m.find("track") != std::string::npos) return "Track your order from Orders page; each order shows its live status.";
  if (m.find("sell") != std::string::npos) return "Sellers register with role SELLER, then manage listings from the Seller Dashboard (create, edit, delete, view incoming orders and revenue).";
  if (m.find("payment") != std::string::npos || m.find("pay") != std::string::npos) return "Checkout uses a mock payment confirmation step — no real gateway, no card data leaves your browser.";
  if (m.find("refund") != std::string::npos) return "Cancelled orders are refunded instantly to the mock-payment method.";
  if (m.find("discount") != std::string::npos || m.find("coupon") != std::string::npos) return "There are no coupons right now — the catalogue price you see is the price you pay.";
  if (m.find("contact") != std::string::npos || m.find("support") != std::string::npos) return "Reach support at support@sanghavimart.local — median reply time one business day.";
  if (m.find("hello") != std::string::npos || m.find("hi") != std::string::npos) return "Hello! I can help with products, orders, shipping, returns and selling on SanghaviMart. What do you need?";
  return "I can help with SanghaviMart products, orders, shipping, returns and selling. Try asking about shipping times, returns, or how to sell.";
}
GeminiChatProvider::GeminiChatProvider(std::string key, std::string model)
    : api_key_(std::move(key)), model_(std::move(model)) {}
std::string GeminiChatProvider::GetReply(const std::string& msg, const std::string&) {
  if (api_key_.empty()) return MockChatProvider{}.GetReply(msg, "");
  CURL* curl = curl_easy_init();
  if (curl == nullptr) return MockChatProvider{}.GetReply(msg, "");  // degraded static response
  std::string resp_body;
  std::string url = "https://generativelanguage.googleapis.com/v1beta/models/" + model_ + ":generateContent?key=" + api_key_;
  std::string payload = "{\"contents\":[{\"parts\":[{\"text\":\"" + std::string(kChatPrompt) + "\\nUser: " + msg + "\"}]}]}";
  struct curl_slist* hdrs = nullptr;
  hdrs = curl_slist_append(hdrs, "Content-Type: application/json");
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp_body);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);  // explicit timeout per spec
  CURLcode rc = curl_easy_perform(curl);
  curl_slist_free_all(hdrs); curl_easy_cleanup(curl);
  if (rc != CURLE_OK || resp_body.empty()) return MockChatProvider{}.GetReply(msg, "");  // degraded
  auto pos = resp_body.find("\"text\"");
  if (pos == std::string::npos) return MockChatProvider{}.GetReply(msg, "");
  return resp_body.substr(pos, 500);  // keep it simple; mock covers FAQs
}
std::unique_ptr<ChatProvider> ChatProviderFactory::Create(const std::string& name, const std::string& key) {
  if (name == "gemini") return std::make_unique<GeminiChatProvider>(key);
  return std::make_unique<MockChatProvider>();
}
ChatService::ChatService(std::unique_ptr<ChatProvider> p) : provider_(std::move(p)) {}
std::string ChatService::Norm(const std::string& s) {
  std::string o; for (char c : s) o += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return o;
}
std::string ChatService::Reply(const std::string& sid, const std::string& message) {
  if (message.empty() || message.size() > 500) throw ValidationException("message must be 1..500 chars");
  std::lock_guard<std::mutex> lock(mutex_);
  const std::int64_t now = NowSec();
  auto& hits = hits_[sid];
  hits.erase(std::remove_if(hits.begin(), hits.end(), [&](std::int64_t t){ return now - t > 60; }), hits.end());
  if (hits.size() >= 10) throw ConflictException("rate limit: 10 messages/minute");
  hits.push_back(now);
  const std::string key = Norm(message);
  auto sit = cache_.find(sid);
  if (sit != cache_.end()) {
    auto cit = sit->second.find(key);
    if (cit != sit->second.end()) return cit->second;  // per-session repeat cache
  }
  std::string reply = provider_->GetReply(message, kChatPrompt);
  cache_[sid][key] = reply;
  return reply;
}
}  // namespace sanghavi::sanghavimart
