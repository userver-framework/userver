#pragma once

/// @file userver/utils/retry_budget.hpp
/// @brief @copybrief utils::RetryBudget

#include <atomic>
#include <cstdint>

#include <userver/formats/json_fwd.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/utils/statistics/rate_counter.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace statistics {
class Writer;
}

struct RetryBudgetSettings final {
  // Maximum number of tokens in the budget.
  float max_tokens{100.0f};
  // The number of tokens by which the budget will be increased in case of a
  // successful request.
  float token_ratio{0.1f};
  // Enable/disable retry budget.
  bool enabled{true};
};

/// Class implements the same logic described in
/// https://github.com/grpc/proposal/blob/master/A6-client-retries.md#throttling-configuration
class RetryBudget final {
 public:
  RetryBudget();
  explicit RetryBudget(const RetryBudgetSettings& settings);

  /// Call after a request succeeds.
  void AccountOk() noexcept;

  /// Call after a request fails.
  void AccountFail() noexcept;

  /// @brief Call before attempting a retry (but not before the initial
  /// request).
  bool CanRetry() const;

  /// Thread-safe relative to AccountOk/AccountFail/CanRetry.
  /// Not thread-safe relative other SetSettings method calls.
  void SetSettings(const RetryBudgetSettings& settings);

 private:
  friend void DumpMetric(statistics::Writer& writer, const RetryBudget& budget);

  std::atomic<std::uint32_t> max_tokens_;
  std::atomic<std::uint32_t> token_ratio_;
  std::atomic<std::int32_t> token_count_;
  std::atomic<bool> enabled_{false};

  // rate of account oks
  utils::statistics::RateCounter ok_rate_counter_;
  // rate of account fails
  utils::statistics::RateCounter fail_rate_counter_;
};

RetryBudgetSettings Parse(const formats::json::Value& elem,
                          formats::parse::To<RetryBudgetSettings>);

}  // namespace utils

USERVER_NAMESPACE_END
