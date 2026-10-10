#include <gtest/gtest.h>
#include "util/ValidationUtil.h"
#include "exception/AppException.h"
using sanghavi::sanghavimart::ValidationUtil;
using sanghavi::sanghavimart::ValidationException;
TEST(Validation, Email) {
  EXPECT_THROW(ValidationUtil::RequireEmail("not-an-email"), ValidationException);
  EXPECT_NO_THROW(ValidationUtil::RequireEmail("buyer@sanghavimart.local"));
}
TEST(Validation, StatusTransitions) {
  using sanghavi::sanghavimart::IsValidStatusTransition;
  using sanghavi::sanghavimart::OrderStatus;
  EXPECT_TRUE(IsValidStatusTransition(OrderStatus::kPending, OrderStatus::kConfirmed));
  EXPECT_TRUE(IsValidStatusTransition(OrderStatus::kConfirmed, OrderStatus::kShipped));
  EXPECT_TRUE(IsValidStatusTransition(OrderStatus::kShipped, OrderStatus::kDelivered));
  EXPECT_FALSE(IsValidStatusTransition(OrderStatus::kPending, OrderStatus::kShipped));
  EXPECT_FALSE(IsValidStatusTransition(OrderStatus::kDelivered, OrderStatus::kCancelled));
}
