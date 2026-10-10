// Service tests with hand fakes (GoogleMock-style; fakes mirror repository interfaces).
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "service/Services.h"
using namespace sanghavi::sanghavimart;
using ::testing::_;
using ::testing::Return;
class FakeProducts : public IProductRepository {
 public:
  MOCK_METHOD(Product, Create, (const Product&), (override));
  MOCK_METHOD((std::optional<Product>), FindById, (std::int64_t), (override));
  MOCK_METHOD((std::vector<Product>), Search, (const std::string&, const std::string&, int, int), (override));
  MOCK_METHOD((std::vector<Product>), ListBySeller, (std::int64_t), (override));
  MOCK_METHOD(void, Update, (const Product&), (override));
  MOCK_METHOD(void, Remove, (std::int64_t), (override));
};
TEST(ProductService, RejectsNonPositivePrice) {
  auto f = std::make_shared<FakeProducts>();
  ProductService s(f);
  EXPECT_THROW(s.Create(1, "x", "d", 0, 5, "c", "", {}), ValidationException);
  EXPECT_THROW(s.Create(1, "x", "d", -50, 5, "c", "", {}), ValidationException);
}
TEST(ChatService, RateLimitAndCache) {
  ChatService c(ChatProviderFactory::Create("mock", ""));
  for (int i = 0; i < 10; ++i) EXPECT_NO_THROW(c.Reply("s1", std::string("shipping q") + std::to_string(i)));
  EXPECT_THROW(c.Reply("s1", "one more"), ConflictException);
  ChatService c2(ChatProviderFactory::Create("mock", ""));
  auto a = c2.Reply("s2", "shipping?"); auto b = c2.Reply("s2", "SHIPPING?");
  EXPECT_EQ(a, b);  // per-session repeat cache
}
TEST(ChatService, MockFaq) {
  ChatService c(ChatProviderFactory::Create("mock", ""));
  EXPECT_NE(c.Reply("s9", "how do returns work?").find("return"), std::string::npos);
}
