#pragma once

#include <atomic>
#include <cstdint>

#include <userver/formats/json_fwd.hpp>
#include <userver/formats/parse/to.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

struct RetryBudgetSettings final {
  float max_tokens{100.0f};
  float token_ratio{0.1f};
  bool enabled{true};
};

/// Class implements the same logic described in
/// https://github.com/grpc/proposal/blob/master/A6-client-retries.md#throttling-configuration
class RetryBudget final {
 public:
  RetryBudget();
  explicit RetryBudget(const RetryBudgetSettings& settings);

  void AccountOk() noexcept;
  void AccountFail() noexcept;
  bool CanRetry() const;

  /// Thread-safe relative to AccountOk/AccountFail/CanRetry.
  /// Not thread-safe relative other SetSettings method calls.
  void SetSettings(const RetryBudgetSettings& settings);

 private:
  std::atomic<std::uint32_t> max_tokens_;
  std::atomic<std::uint32_t> token_ratio_;
  std::atomic<std::int32_t> token_count_;
  std::atomic<bool> enabled_{false};
};

RetryBudgetSettings Parse(const formats::json::Value& elem,
                          formats::parse::To<RetryBudgetSettings>);

}  // namespace redis

USERVER_NAMESPACE_END
