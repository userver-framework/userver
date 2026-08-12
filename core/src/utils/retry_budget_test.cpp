#include <userver/utils/retry_budget.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

TEST(RetryBudget, base) {
    const auto max_tokens = 10;
    const auto token_ratio = 0.1f;
    auto budget = utils::RetryBudget(utils::RetryBudgetSettings{max_tokens, token_ratio, true});

    EXPECT_TRUE(budget.CanRetry());

    for (size_t i = 0; i < max_tokens / 2 - 1; ++i) {
        budget.AccountFail();
    }
    /// Still you can do retries
    EXPECT_TRUE(budget.CanRetry());

    budget.AccountFail();
    EXPECT_FALSE(budget.CanRetry());
}

TEST(RetryBudget, replenish) {
    const auto max_tokens = 10;
    const auto token_ratio = 0.1f;
    auto budget = utils::RetryBudget(utils::RetryBudgetSettings{max_tokens, token_ratio, true});

    /// Empty budget
    for (size_t i = 0; i < max_tokens; ++i) {
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
