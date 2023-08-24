#include <curl-ev/ratelimit.hpp>

#include <string_view>

#include <curl-ev/url.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/str_icase.hpp>

USERVER_NAMESPACE_BEGIN

namespace curl {
namespace {
const size_t kByHostThrottleLruSize = 128;

struct ThrottleFactors {
  impl::CurlPtr scheme_ptr;
  impl::CurlPtr host_ptr;
};

ThrottleFactors ExtractThrottleFactors(const char* url_str) {
  ThrottleFactors factors{};

  if (!url_str) {
    LOG_WARNING() << "Empty URL in throttle check";
    UASSERT_MSG(false, "Empty URL in throttle check");
    return factors;
  }

  std::error_code ec;
  curl::url url;
  url.SetUrl(url_str, ec);
  if (ec) {
    LOG_WARNING() << "Cannot parse URL in throttle check: " << ec;
    UASSERT_MSG(false, "Cannot parse URL in throttle check: " + ec.message());
    return factors;
  }

  factors.scheme_ptr = url.GetSchemePtr(ec);
  if (ec || !factors.scheme_ptr) {
    LOG_INFO() << "Cannot retrieve scheme from the URL: " << ec;
    UASSERT_MSG(false, "Cannot retrieve scheme from URL: " + ec.message());
  }

  factors.host_ptr = url.GetHostPtr(ec);
  if (ec || !factors.host_ptr) {
    LOG_INFO() << "Cannot retrieve host from the URL: " << ec;
    UASSERT_MSG(false, "Cannot retrieve host from URL: " + ec.message());
  }

  return factors;
}

}  // namespace

ConnectRateLimiter::ConnectRateLimiter()
    : global_http_(utils::TokenBucket::MakeUnbounded()),
      global_https_(utils::TokenBucket::MakeUnbounded()),
      per_host_limit_(-1UL),
      per_host_rate_(utils::TokenBucket::Duration::zero()),
      by_host_(kByHostThrottleLruSize) {
  static_assert(decltype(per_host_limit_)::is_always_lock_free,
                "limit type is not lock-free atomic");
  static_assert(decltype(per_host_rate_)::is_always_lock_free,
                "rate type is not lock-free atomic");
}

void ConnectRateLimiter::SetGlobalHttpLimits(
    size_t limit, utils::TokenBucket::Duration rate) {
  global_http_.SetMaxSize(limit);
  global_http_.SetRefillPolicy({1, rate});
}

void ConnectRateLimiter::SetGlobalHttpsLimits(
    size_t limit, utils::TokenBucket::Duration rate) {
  global_https_.SetMaxSize(limit);
  global_https_.SetRefillPolicy({1, rate});
}

void ConnectRateLimiter::SetPerHostLimits(size_t limit,
                                          utils::TokenBucket::Duration rate) {
  per_host_limit_.store(limit, std::memory_order_relaxed);
  per_host_rate_.store(rate, std::memory_order_relaxed);
}

void ConnectRateLimiter::Check(const char* url_str, std::error_code& ec) {
  static constexpr std::string_view kHttpsScheme = "https";

  bool may_acquire = true;
  const auto factors = ExtractThrottleFactors(url_str);

  if (factors.host_ptr) {
    const auto current_per_host_limit =
        per_host_limit_.load(std::memory_order_relaxed);
    const auto current_per_host_rate =
        per_host_rate_.load(std::memory_order_relaxed);

    auto locked = by_host_.UniqueLock();
    auto* local_throttle = locked->Emplace(
        factors.host_ptr.get(), current_per_host_limit,
        utils::TokenBucket::RefillPolicy{1, current_per_host_rate});

    local_throttle->SetMaxSize(current_per_host_limit);
    local_throttle->SetRefillPolicy({1, current_per_host_rate});
    may_acquire = local_throttle->Obtain();
  }

  if (!may_acquire) {
    ec = errc::RateLimitErrorCode::kPerHostSocketLimit;
    return;
  }

  auto& global_token_bucket =
      (factors.scheme_ptr &&
       utils::StrIcaseEqual{}(factors.scheme_ptr.get(), kHttpsScheme))
          ? global_https_
          : global_http_;

  may_acquire = global_token_bucket.Obtain();
  if (!may_acquire) {
    ec = errc::RateLimitErrorCode::kGlobalSocketLimit;
    return;
  }

  ec = errc::RateLimitErrorCode::kSuccess;
}

}  // namespace curl

USERVER_NAMESPACE_END
