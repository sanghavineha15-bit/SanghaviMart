#include <fstream>
#include <sstream>
#include <string>
#include <gtest/gtest.h>
#include <drogon/orm/DbClient.h>
#include "repository/DbRepository.h"
using namespace sanghavi::sanghavimart;
class SqliteRepoTest : public ::testing::Test {
 protected:
  drogon::orm::DbClientPtr db_;
  void SetUp() override {
    db_ = drogon::orm::DbClient::newSqlite3Client(":memory:", 1);
    std::ifstream in("./db/schema.sql");
    if (!in) in = std::ifstream("../db/schema.sql");
    std::stringstream ss; ss << in.rdbuf();
    std::string stmt;
    for (std::string line; std::getline(ss, line);) {
      stmt += line + "\n";
      if (line.find('"'"';'"'"') != std::string::npos) {
        try { db_->execSqlSync(stmt); } catch (...) {}
        stmt.clear();
      }
    }
  }
};
TEST_F(SqliteRepoTest, UserRoundTrip) {
  DbUserRepository users(db_);
  User u; u.name = "Buyer"; u.email = "b@t.local"; u.password_hash = "h"; u.role = UserRole::kBuyer;
  auto c = users.Create(u);
  EXPECT_GT(c.id, 0);
  EXPECT_THROW({ User d = u; users.Create(d); }, std::exception);
  EXPECT_EQ(users.FindByEmail("b@t.local")->name, "Buyer");
}
TEST_F(SqliteRepoTest, ProductSearchIsParameterized) {
  DbProductRepository prods(db_);
  Product p; p.seller_id = 1; p.name = "Bottle"; p.price = Money::FromCents(100); p.stock_qty = 5;
  prods.Create(p);
  auto r = prods.Search("'"'"' OR '"'"'1'"'"'='"'"'1", "", 10, 0);
  EXPECT_TRUE(r.empty());
}
