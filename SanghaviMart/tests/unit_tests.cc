#include <gtest/gtest.h>
#include "../SanghaviMart/src/model/Money.h"
#include "../SanghaviMart/src/util/HtmlEscape.h"
#include "../SanghaviMart/src/util/ValidationUtil.h"
#include "../SanghaviMart/src/service/Services.h"
#include "../SanghaviMart/src/chat/Chat.h"

using namespace sanghavimart;

TEST(Money, MinorUnitsNoFloat) {
    model::Money m = model::Money::FromMinor(99950);
    EXPECT_EQ(m.MinorUnits(), 99950);
    EXPECT_EQ((m * 2).MinorUnits(), 199900);
    EXPECT_EQ((m + model::Money::FromMinor(50)).MinorUnits(), 100000);
}

TEST(Validation, EmailPassword) {
    EXPECT_THROW(util::ValidationUtil::ValidateEmail("bad"), exception::ValidationException);
    EXPECT_NO_THROW(util::ValidationUtil::ValidateEmail("a@b.com"));
    EXPECT_THROW(util::ValidationUtil::ValidatePassword("short"), exception::ValidationException);
    EXPECT_THROW(util::ValidationUtil::ValidateRating(0), exception::ValidationException);
    EXPECT_THROW(util::ValidationUtil::ValidateQuantity(0), exception::ValidationException);
    EXPECT_THROW(util::ValidationUtil::ValidateRole("ADMIN"), exception::ValidationException);
}

TEST(Orders, StateMachine) {
    EXPECT_TRUE(service::IsValidTransition("PENDING", "CONFIRMED"));
    EXPECT_TRUE(service::IsValidTransition("CONFIRMED", "SHIPPED"));
    EXPECT_TRUE(service::IsValidTransition("SHIPPED", "DELIVERED"));
    EXPECT_TRUE(service::IsValidTransition("PENDING", "CANCELLED"));
    EXPECT_FALSE(service::IsValidTransition("PENDING", "SHIPPED"));
    EXPECT_FALSE(service::IsValidTransition("DELIVERED", "CANCELLED"));
}

TEST(Chat, RateLimitAndCache) {
    chat::ChatService svc(chat::MakeProvider("mock"));
    for (int i = 0; i < 10; ++i) EXPECT_NO_THROW(svc.Chat("s1", "msg" + std::to_string(i)));
    EXPECT_NE(svc.Chat("s1", "overflow").find("Rate limit"), std::string::npos);
    EXPECT_EQ(svc.Chat("s2", "hello"), svc.Chat("s2", "hello"));  // cached
}

TEST(Security, HtmlEscapeNeutralizesXss) {
    EXPECT_EQ(util::HtmlEscape("<script>alert(1)</script>"),
              "&lt;script&gt;alert(1)&lt;/script&gt;");
    EXPECT_EQ(util::HtmlEscape("\"'><img src=x onerror=y>"), "&quot;&#39;&gt;&lt;img src=x onerror=y&gt;");
    EXPECT_EQ(util::HtmlEscape("plain text & price"), "plain text &amp; price");
}

TEST(Money, CentsRoundTrip) {
    // price_cents is the API/DB unit; majors are display-only.
    auto m = model::Money::FromMinor(19999);
    EXPECT_EQ(m.MinorUnits(), 19999);
    EXPECT_DOUBLE_EQ(m.ToMajor(), 199.99);
}

// NOTE: Repository + HTTP integration tests require PostgreSQL.
// Run with POSTGRES_TEST_DB set; see tests/integration_readme.md.
// They exercise real SQL against PostgreSQL (no SQLite).
