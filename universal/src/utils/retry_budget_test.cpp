#include <userver/utils/retry_budget.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

TEST(RetryBudget, base) {
  const auto kMaxTokens = 10;
  const auto kTokenRatio = 0.1f;
  auto budget = utils::RetryBudget(
      utils::RetryBudgetSettings{kMaxTokens, kTokenRatio, true});

  EXPECT_TRUE(budget.CanRetry());

  for (size_t i = 0; i < kMaxTokens / 2 - 1; ++i) {
    budget.AccountFail();
  }
  /// Still you can do retries
  EXPECT_TRUE(budget.CanRetry());

  budget.AccountFail();
  EXPECT_FALSE(budget.CanRetry());
}

TEST(RetryBudget, replenish) {
  const auto kMaxTokens = 10;
  const auto kTokenRatio = 0.1f;
  auto budget = utils::RetryBudget(
      utils::RetryBudgetSettings{kMaxTokens, kTokenRatio, true});

  /// Empty budget
  for (size_t i = 0; i < kMaxTokens; ++i) {
    budget.AccountFail();
  }
  EXPECT_FALSE(budget.CanRetry());

  /// replenish half of all max_tokens
  for (size_t i = 0; i < 50; ++i) {
    budget.AccountOk();
  }
  EXPECT_FALSE(budget.CanRetry());

  budget.AccountOk();
  EXPECT_TRUE(budget.CanRetry());
}

USERVER_NAMESPACE_END
