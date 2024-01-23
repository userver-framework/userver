#include <userver/utils/retry_budget.hpp>

#include <algorithm>
#include <atomic>

#include <userver/formats/json.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace {
constexpr std::int32_t kMillis = 1000;
}

RetryBudget::RetryBudget() : RetryBudget(RetryBudgetSettings()) {}

RetryBudget::RetryBudget(const RetryBudgetSettings& settings)
    : token_count_(settings.max_tokens * kMillis) {
  SetSettings(settings);
}

void RetryBudget::AccountOk() noexcept {
  if (!enabled_.load(std::memory_order_relaxed)) {
    return;
  }

  const auto max_tokens = max_tokens_.load(std::memory_order_relaxed);
  const auto token_ratio = token_ratio_.load(std::memory_order_relaxed);

  auto expected = token_count_.load(std::memory_order_relaxed);
  while (!token_count_.compare_exchange_weak(
      expected, std::min(max_tokens, expected + token_ratio),
      std::memory_order_relaxed, std::memory_order_relaxed))
    ;
}

void RetryBudget::AccountFail() noexcept {
  if (!enabled_.load(std::memory_order_relaxed)) {
    return;
  }

  auto expected = token_count_.load(std::memory_order_relaxed);
  while (!token_count_.compare_exchange_weak(
      expected, std::max(0, expected - kMillis), std::memory_order_relaxed,
      std::memory_order_relaxed))
    ;
}

bool RetryBudget::CanRetry() const {
  if (!enabled_.load(std::memory_order_relaxed)) {
    return true;
  }

  const auto max_tokens = max_tokens_.load(std::memory_order_relaxed);
  const std::uint32_t tokens = token_count_.load(std::memory_order_relaxed);
  return tokens > max_tokens / 2;
}

void RetryBudget::SetSettings(const RetryBudgetSettings& settings) {
  UASSERT(settings.max_tokens > 0);
  UASSERT(settings.max_tokens <= 1000000);
  UASSERT(settings.token_ratio > 0);
  enabled_.store(settings.enabled, std::memory_order_relaxed);
  max_tokens_.store(settings.max_tokens * kMillis, std::memory_order_relaxed);
  token_ratio_.store(settings.token_ratio * kMillis, std::memory_order_relaxed);
}

RetryBudgetSettings Parse(const formats::json::Value& elem,
                          formats::parse::To<RetryBudgetSettings>) {
  RetryBudgetSettings result;
  result.max_tokens = elem["max-tokens"].As<float>(result.max_tokens);
  result.token_ratio = elem["token-ratio"].As<float>(result.token_ratio);
  result.enabled = elem["enabled"].As<bool>(result.enabled);
  return result;
}

}  // namespace utils

USERVER_NAMESPACE_END
