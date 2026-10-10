#include <gtest/gtest.h>
#include "model/Money.h"
using sanghavi::sanghavimart::Money;
TEST(Money, AddAndMultiply) {
  auto t = Money::FromCents(900) + Money::FromCents(100);
  EXPECT_EQ(t.Cents(), 1000);
  EXPECT_EQ((Money::FromCents(250) * 4).Cents(), 1000);
}
TEST(Money, NegativeRejected) { EXPECT_THROW(Money::FromCents(-1), std::invalid_argument); }
