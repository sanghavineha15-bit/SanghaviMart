#include "repository/DbRepository.h"
#include "exception/AppException.h"
using drogon::orm::Compare;
namespace sanghavi::sanghavimart {
// ---- helpers (sync wrappers over Drogon async client; RAII transactions) ----
static drogon::orm::Result Exec(drogon::orm::DbClientPtr db, const std::string& sql) {
  return db->execSqlSync(sql);
}
template <typename... Args>
static drogon::orm::Result Exec(drogon::orm::DbClientPtr db, const std::string& sql, Args&&... args) {
  return db->execSqlSync(sql, std::forward<Args>(args)...);
}
// ---- users ----
User DbUserRepository::Create(const User& u) {
  auto r = Exec(db_, "INSERT INTO users(name,email,password_hash,role) VALUES($1,$2,$3,$4) RETURNING id, created_at",
                u.name, u.email, u.password_hash, ToString(u.role));
  User out = u; out.id = r[0]["id"].as<std::int64_t>();
  out.created_at = r[0]["created_at"].as<std::string>(); return out;
}
std::optional<User> DbUserRepository::FindById(std::int64_t id) {
  auto r = Exec(db_, "SELECT id,name,email,password_hash,role,created_at FROM users WHERE id=$1", id);
  if (r.empty()) return std::nullopt;
  User u; u.id = r[0]["id"].as<std::int64_t>(); u.name = r[0]["name"].as<std::string>();
  u.email = r[0]["email"].as<std::string>(); u.password_hash = r[0]["password_hash"].as<std::string>();
  u.role = RoleFromString(r[0]["role"].as<std::string>()); u.created_at = r[0]["created_at"].as<std::string>();
  return u;
}
std::optional<User> DbUserRepository::FindByEmail(const std::string& email) {
  auto r = Exec(db_, "SELECT id,name,email,password_hash,role,created_at FROM users WHERE email=$1", email);
  if (r.empty()) return std::nullopt;
  User u; u.id = r[0]["id"].as<std::int64_t>(); u.name = r[0]["name"].as<std::string>();
  u.email = r[0]["email"].as<std::string>(); u.password_hash = r[0]["password_hash"].as<std::string>();
  u.role = RoleFromString(r[0]["role"].as<std::string>()); u.created_at = r[0]["created_at"].as<std::string>();
  return u;
}
std::vector<User> DbUserRepository::ListAll() {
  std::vector<User> out;
  for (auto row : Exec(db_, "SELECT id,name,email,password_hash,role,created_at FROM users ORDER BY id")) {
    User u; u.id = row["id"].as<std::int64_t>(); u.name = row["name"].as<std::string>();
    u.email = row["email"].as<std::string>(); u.password_hash = row["password_hash"].as<std::string>();
    u.role = RoleFromString(row["role"].as<std::string>()); u.created_at = row["created_at"].as<std::string>();
    out.push_back(u);
  }
  return out;
}
// ---- products ----
Product DbProductRepository::Create(const Product& p) {
  auto r = Exec(db_, "INSERT INTO products(seller_id,name,description,price_cents,stock_qty,category,image_url)"
                     " VALUES($1,$2,$3,$4,$5,$6,$7) RETURNING id, created_at",
                p.seller_id, p.name, p.description, p.price.Cents(), p.stock_qty, p.category, p.image_url);
  Product o = p; o.id = r[0]["id"].as<std::int64_t>(); o.created_at = r[0]["created_at"].as<std::string>(); return o;
}
std::optional<Product> DbProductRepository::FindById(std::int64_t id) {
  auto r = Exec(db_, "SELECT id,seller_id,name,description,price_cents,stock_qty,category,image_url,created_at"
                     " FROM products WHERE id=$1", id);
  if (r.empty()) return std::nullopt;
  auto row = r[0]; Product p; p.id = row["id"].as<std::int64_t>(); p.seller_id = row["seller_id"].as<std::int64_t>();
  p.name = row["name"].as<std::string>(); p.description = row["description"].as<std::string>();
  p.price = Money::FromCents(row["price_cents"].as<std::int64_t>()); p.stock_qty = row["stock_qty"].as<std::int64_t>();
  p.category = row["category"].as<std::string>(); p.image_url = row["image_url"].as<std::string>();
  p.created_at = row["created_at"].as<std::string>(); return p;
}
std::vector<Product> DbProductRepository::Search(const std::string& kw, const std::string& cat, int lim, int off) {
  std::string like = "%" + kw + "%";
  drogon::orm::Result r;
  if (cat.empty())
    r = Exec(db_, "SELECT id,seller_id,name,description,price_cents,stock_qty,category,image_url,created_at FROM products"
                  " WHERE ($1 = $2 OR name LIKE $1 OR description LIKE $1) ORDER BY id DESC LIMIT $3 OFFSET $4",
             like, std::string("%%"), lim, off);
  else
    r = Exec(db_, "SELECT id,seller_id,name,description,price_cents,stock_qty,category,image_url,created_at FROM products"
                  " WHERE category=$1 AND ($2 = $3 OR name LIKE $2 OR description LIKE $2) ORDER BY id DESC LIMIT $4 OFFSET $5",
             cat, like, std::string("%%"), lim, off);
  std::vector<Product> out;
  for (auto row : r) {
    Product p; p.id = row["id"].as<std::int64_t>(); p.seller_id = row["seller_id"].as<std::int64_t>();
    p.name = row["name"].as<std::string>(); p.description = row["description"].as<std::string>();
    p.price = Money::FromCents(row["price_cents"].as<std::int64_t>()); p.stock_qty = row["stock_qty"].as<std::int64_t>();
    p.category = row["category"].as<std::string>(); p.image_url = row["image_url"].as<std::string>();
    p.created_at = row["created_at"].as<std::string>(); out.push_back(p);
  }
  return out;
}
std::vector<Product> DbProductRepository::ListBySeller(std::int64_t sid) {
  std::vector<Product> out;
  for (auto row : Exec(db_, "SELECT id,seller_id,name,description,price_cents,stock_qty,category,image_url,created_at"
                             " FROM products WHERE seller_id=$1 ORDER BY id", sid)) {
    Product p; p.id = row["id"].as<std::int64_t>(); p.seller_id = row["seller_id"].as<std::int64_t>();
    p.name = row["name"].as<std::string>(); p.description = row["description"].as<std::string>();
    p.price = Money::FromCents(row["price_cents"].as<std::int64_t>()); p.stock_qty = row["stock_qty"].as<std::int64_t>();
    p.category = row["category"].as<std::string>(); p.image_url = row["image_url"].as<std::string>();
    p.created_at = row["created_at"].as<std::string>(); out.push_back(p);
  }
  return out;
}
void DbProductRepository::Update(const Product& p) {
  Exec(db_, "UPDATE products SET name=$1,description=$2,price_cents=$3,stock_qty=$4,category=$5,image_url=$6 WHERE id=$7",
       p.name, p.description, p.price.Cents(), p.stock_qty, p.category, p.image_url, p.id);
}
void DbProductRepository::Remove(std::int64_t id) {
  auto refs = Exec(db_, "SELECT COUNT(*) AS c FROM order_items WHERE product_id=$1", id);
  if (refs[0]["c"].as<std::int64_t>() > 0)
    throw ConflictException("cannot remove: listing has orders");
  Exec(db_, "DELETE FROM products WHERE id=$1", id);
}
// ---- cart ----
void DbCartRepository::Upsert(std::int64_t uid, std::int64_t pid, std::int64_t qty) {
  auto r = Exec(db_, "SELECT id FROM cart_items WHERE user_id=$1 AND product_id=$2", uid, pid);
  if (r.empty()) Exec(db_, "INSERT INTO cart_items(user_id,product_id,quantity) VALUES($1,$2,$3)", uid, pid, qty);
  else Exec(db_, "UPDATE cart_items SET quantity=$1 WHERE user_id=$2 AND product_id=$3", qty, uid, pid);
}
void DbCartRepository::RemoveItem(std::int64_t uid, std::int64_t pid) {
  Exec(db_, "DELETE FROM cart_items WHERE user_id=$1 AND product_id=$2", uid, pid);
}
void DbCartRepository::Clear(std::int64_t uid) {
  Exec(db_, "DELETE FROM cart_items WHERE user_id=$1", uid);
}
std::vector<CartItem> DbCartRepository::List(std::int64_t uid) {
  std::vector<CartItem> out;
  for (auto row : Exec(db_, "SELECT c.id,c.user_id,c.product_id,c.quantity,p.name,p.price_cents FROM cart_items c"
                             " JOIN products p ON p.id=c.product_id WHERE c.user_id=$1 ORDER BY c.id", uid)) {
    CartItem c; c.id = row["id"].as<std::int64_t>(); c.user_id = row["user_id"].as<std::int64_t>();
    c.product_id = row["product_id"].as<std::int64_t>(); c.quantity = row["quantity"].as<std::int64_t>();
    c.product_name = row["name"].as<std::string>();
    c.unit_price = Money::FromCents(row["price_cents"].as<std::int64_t>()); out.push_back(c);
  }
  return out;
}
// ---- orders (transactional: RAII transaction, auto-rollback on scope exit) ----
Order DbOrderRepository::CreateOrder(const Order& o, const std::vector<OrderItem>& items) {
  auto tx = db_->newTransaction();
  try {
    auto r = tx->execSqlSync("INSERT INTO orders(buyer_id,status,total_amount_cents) VALUES($1,$2,$3) RETURNING id, created_at",
                             o.buyer_id, ToString(o.status), o.total.Cents());
    Order out = o; out.id = r[0]["id"].as<std::int64_t>(); out.created_at = r[0]["created_at"].as<std::string>();
    for (const auto& it : items) {
      tx->execSqlSync("INSERT INTO order_items(order_id,product_id,quantity,unit_price_cents) VALUES($1,$2,$3,$4)",
                      out.id, it.product_id, it.quantity, it.unit_price.Cents());
      tx->execSqlSync("UPDATE products SET stock_qty = stock_qty - $1 WHERE id=$2 AND stock_qty >= $1",
                      it.quantity, it.product_id);
    }
    tx->execSqlSync("DELETE FROM cart_items WHERE user_id=$1", o.buyer_id);
    return out;
  } catch (...) { throw; }
}
std::optional<Order> DbOrderRepository::FindById(std::int64_t id) {
  auto r = Exec(db_, "SELECT id,buyer_id,status,total_amount_cents,created_at FROM orders WHERE id=$1", id);
  if (r.empty()) return std::nullopt;
  Order o; o.id = r[0]["id"].as<std::int64_t>(); o.buyer_id = r[0]["buyer_id"].as<std::int64_t>();
  o.status = StatusFromString(r[0]["status"].as<std::string>());
  o.total = Money::FromCents(r[0]["total_amount_cents"].as<std::int64_t>());
  o.created_at = r[0]["created_at"].as<std::string>(); return o;
}
std::vector<Order> DbOrderRepository::ListByBuyer(std::int64_t bid) {
  std::vector<Order> out;
  for (auto row : Exec(db_, "SELECT id,buyer_id,status,total_amount_cents,created_at FROM orders WHERE buyer_id=$1 ORDER BY id DESC", bid)) {
    Order o; o.id = row["id"].as<std::int64_t>(); o.buyer_id = row["buyer_id"].as<std::int64_t>();
    o.status = StatusFromString(row["status"].as<std::string>());
    o.total = Money::FromCents(row["total_amount_cents"].as<std::int64_t>());
    o.created_at = row["created_at"].as<std::string>(); out.push_back(o);
  }
  return out;
}
std::vector<Order> DbOrderRepository::ListAll() {
  std::vector<Order> out;
  for (auto row : Exec(db_, "SELECT id,buyer_id,status,total_amount_cents,created_at FROM orders ORDER BY id DESC")) {
    Order o; o.id = row["id"].as<std::int64_t>(); o.buyer_id = row["buyer_id"].as<std::int64_t>();
    o.status = StatusFromString(row["status"].as<std::string>());
    o.total = Money::FromCents(row["total_amount_cents"].as<std::int64_t>());
    o.created_at = row["created_at"].as<std::string>(); out.push_back(o);
  }
  return out;
}
std::vector<OrderItem> DbOrderRepository::ItemsOf(std::int64_t oid) {
  std::vector<OrderItem> out;
  for (auto row : Exec(db_, "SELECT id,order_id,product_id,quantity,unit_price_cents FROM order_items WHERE order_id=$1", oid)) {
    OrderItem i; i.id = row["id"].as<std::int64_t>(); i.order_id = row["order_id"].as<std::int64_t>();
    i.product_id = row["product_id"].as<std::int64_t>(); i.quantity = row["quantity"].as<std::int64_t>();
    i.unit_price = Money::FromCents(row["unit_price_cents"].as<std::int64_t>()); out.push_back(i);
  }
  return out;
}
std::vector<Order> DbOrderRepository::IncomingForSeller(std::int64_t sid) {
  std::vector<Order> out;
  for (auto row : Exec(db_, "SELECT DISTINCT o.id,o.buyer_id,o.status,o.total_amount_cents,o.created_at FROM orders o"
                             " JOIN order_items i ON i.order_id=o.id JOIN products p ON p.id=i.product_id"
                             " WHERE p.seller_id=$1 ORDER BY o.id DESC", sid)) {
    Order o; o.id = row["id"].as<std::int64_t>(); o.buyer_id = row["buyer_id"].as<std::int64_t>();
    o.status = StatusFromString(row["status"].as<std::string>());
    o.total = Money::FromCents(row["total_amount_cents"].as<std::int64_t>());
    o.created_at = row["created_at"].as<std::string>(); out.push_back(o);
  }
  return out;
}
void DbOrderRepository::UpdateStatus(std::int64_t oid, OrderStatus s) {
  Exec(db_, "UPDATE orders SET status=$1 WHERE id=$2", ToString(s), oid);
}
std::int64_t DbOrderRepository::SalesCountForSeller(std::int64_t sid) {
  auto r = Exec(db_, "SELECT COUNT(*) AS c FROM order_items i JOIN products p ON p.id=i.product_id WHERE p.seller_id=$1", sid);
  return r[0]["c"].as<std::int64_t>();
}
std::int64_t DbOrderRepository::RevenueCentsForSeller(std::int64_t sid) {
  auto r = Exec(db_, "SELECT COALESCE(SUM(i.quantity*i.unit_price_cents),0) AS s FROM order_items i"
                     " JOIN products p ON p.id=i.product_id WHERE p.seller_id=$1", sid);
  return r[0]["s"].as<std::int64_t>();
}
// ---- reviews ----
Review DbReviewRepository::Create(const Review& r0) {
  auto r = Exec(db_, "INSERT INTO reviews(product_id,user_id,rating,comment) VALUES($1,$2,$3,$4) RETURNING id, created_at",
                r0.product_id, r0.user_id, r0.rating, r0.comment);
  Review o = r0; o.id = r[0]["id"].as<std::int64_t>(); o.created_at = r[0]["created_at"].as<std::string>(); return o;
}
std::vector<Review> DbReviewRepository::ListByProduct(std::int64_t pid) {
  std::vector<Review> out;
  for (auto row : Exec(db_, "SELECT id,product_id,user_id,rating,comment,created_at FROM reviews WHERE product_id=$1 ORDER BY id DESC", pid)) {
    Review r; r.id = row["id"].as<std::int64_t>(); r.product_id = row["product_id"].as<std::int64_t>();
    r.user_id = row["user_id"].as<std::int64_t>(); r.rating = row["rating"].as<int>();
    r.comment = row["comment"].as<std::string>(); r.created_at = row["created_at"].as<std::string>(); out.push_back(r);
  }
  return out;
}
bool DbReviewRepository::HasUserOrderedProduct(std::int64_t uid, std::int64_t pid) {
  auto r = Exec(db_, "SELECT COUNT(*) AS c FROM orders o JOIN order_items i ON i.order_id=o.id"
                     " WHERE o.buyer_id=$1 AND i.product_id=$2 AND o.status != $3", uid, pid, std::string("CANCELLED"));
  return r[0]["c"].as<std::int64_t>() > 0;
}
double DbReviewRepository::AvgRating(std::int64_t pid) {
  auto r = Exec(db_, "SELECT COALESCE(AVG(rating),0) AS a FROM reviews WHERE product_id=$1", pid);
  return r[0]["a"].as<double>();
}
// ---- factory ----
std::unique_ptr<IUserRepository> RepositoryFactory::CreateUsers(drogon::orm::DbClientPtr db) {
  return std::make_unique<DbUserRepository>(std::move(db));
}
std::unique_ptr<IProductRepository> RepositoryFactory::CreateProducts(drogon::orm::DbClientPtr db) {
  return std::make_unique<DbProductRepository>(std::move(db));
}
std::unique_ptr<ICartRepository> RepositoryFactory::CreateCart(drogon::orm::DbClientPtr db) {
  return std::make_unique<DbCartRepository>(std::move(db));
}
std::unique_ptr<IOrderRepository> RepositoryFactory::CreateOrders(drogon::orm::DbClientPtr db) {
  return std::make_unique<DbOrderRepository>(std::move(db));
}
void DbOtpRepository::Issue(const std::string& email, const std::string& code_sha256_hex,
                          std::int64_t expires_at_sec) {
  Exec(db_, "DELETE FROM login_codes WHERE email=$1", email);
  Exec(db_, "INSERT INTO login_codes(email,code_hash,expires_at_sec) VALUES($1,$2,$3)",
       email, code_sha256_hex, expires_at_sec);
}
bool DbOtpRepository::CheckAndCount(const std::string& email,
                                    const std::string& code_sha256_hex,
                                    std::int64_t now_sec) {
  auto r = Exec(db_, "SELECT id,code_hash,expires_at_sec,attempts,used FROM login_codes"
                      " WHERE email=$1 ORDER BY id DESC LIMIT 1", email);
  if (r.empty()) return false;
  auto row = r[0];
  Exec(db_, "UPDATE login_codes SET attempts=attempts+1 WHERE id=$1",
       row["id"].as<std::int64_t>());
  if (row["used"].as<int>() != 0) return false;
  if (row["expires_at_sec"].as<std::int64_t>() < now_sec) return false;
  if (row["attempts"].as<int>() >= 5) return false;
  return row["code_hash"].as<std::string>() == code_sha256_hex;
}
void DbOtpRepository::MarkUsed(const std::string& email) {
  Exec(db_, "UPDATE login_codes SET used=1 WHERE email=$1", email);
}
std::unique_ptr<IReviewRepository> RepositoryFactory::CreateReviews(drogon::orm::DbClientPtr db) {
  return std::make_unique<DbReviewRepository>(std::move(db));
}
std::unique_ptr<IOtpRepository> RepositoryFactory::CreateOtp(drogon::orm::DbClientPtr db) {
  return std::make_unique<DbOtpRepository>(std::move(db));
}
}  // namespace sanghavi::sanghavimart
