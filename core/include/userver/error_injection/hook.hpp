#pragma once

#include <userver/engine/deadline.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/error_injection/settings.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/span.hpp>

USERVER_NAMESPACE_BEGIN

/// Artificial errors injection framework
namespace error_injection {

/// Single operation error injection process. Create on the stack,
/// call PreHook(), do operation, call PostHook().
class Hook final {
 public:
  explicit Hook(const Settings& settings, engine::Deadline deadline);

  /// Must be called just before actual operation. May sleep for some time,
  /// throw an exception, or do nothing.
  /// @throws TimeoutException in case of artificial timeout
  /// @throws ErrorException in case of artificial error
  template <typename TimeoutException, typename ErrorException>
  void PreHook() {
    if (verdict_ == Verdict::Skip) {
      // fast path
      return;
    }

    switch (verdict_) {
      case Verdict::Error:
        LOG_ERROR() << "Error injection hook triggered verdict: error";
        throw ErrorException{"error injection"};

      case Verdict::Timeout:
        LOG_ERROR() << "Error injection hook triggered verdict: timeout";
        {
          auto scope_time = tracing::Span::CurrentSpan().CreateScopeTime(
              "error_injection_timeout");
          engine::InterruptibleSleepUntil(deadline_);
        }
        throw TimeoutException{"error injection"};

      case Verdict::MaxDelay:
      case Verdict::RandomDelay:
      case Verdict::Skip:
          // pass
          ;
    }
  }

  /// Must be called just after operation. May sleep for some time,
  /// or do nothing.
  void PostHook() {
    if (verdict_ == Verdict::Skip) {
      // fast path
      return;
    }

    engine::Deadline deadline = CalcPostHookDeadline();
    if (deadline.IsReached()) return;

    LOG_ERROR()
        << "Error injection hook triggered verdict: max-delay / random-delay";
    auto scope_time =
        tracing::Span::CurrentSpan().CreateScopeTime("error_injection_delay");
    engine::InterruptibleSleepUntil(deadline);
  }

 private:
  engine::Deadline CalcPostHookDeadline();

  static Verdict ReturnVerdict(const Settings& settings);

  const Verdict verdict_;
  const engine::Deadline deadline_;
};

}  // namespace error_injection

USERVER_NAMESPACE_END
