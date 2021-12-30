#include <userver/tracing/scope_time.hpp>

#include <userver/tracing/span.hpp>

#include <tracing/time_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

ScopeTime::ScopeTime()
    : ScopeTime(tracing::Span::CurrentSpan().GetTimeStorage()) {}

ScopeTime::ScopeTime(std::string scope_name)
    : ScopeTime(tracing::Span::CurrentSpan().GetTimeStorage(),
                std::move(scope_name)) {}

ScopeTime::ScopeTime(impl::TimeStorage& ts) : ts_(ts) {}

ScopeTime::ScopeTime(impl::TimeStorage& ts, std::string scope_name)
    : ScopeTime(ts) {
  Reset(std::move(scope_name));
}

ScopeTime::~ScopeTime() { Reset(); }

ScopeTime::Duration ScopeTime::Reset() {
  if (scope_name_.empty()) return ScopeTime::Duration(0);

  const auto duration = std::chrono::steady_clock::now() - start_;
  ts_.PushLap(scope_name_, duration);
  scope_name_.clear();
  return duration;
}

ScopeTime::Duration ScopeTime::Reset(std::string scope_name) {
  auto result = Reset();
  scope_name_ = std::move(scope_name);
  start_ = std::chrono::steady_clock::now();
  return result;
}

void ScopeTime::Discard() { scope_name_.clear(); }

ScopeTime::Duration ScopeTime::DurationSinceReset() const {
  if (scope_name_.empty()) return ScopeTime::Duration{0};
  return std::chrono::steady_clock::now() - start_;
}

ScopeTime::Duration ScopeTime::DurationTotal(
    const std::string& scope_name) const {
  const auto duration = ts_.DurationTotal(scope_name);
  return scope_name_ == scope_name ? duration + DurationSinceReset() : duration;
}

ScopeTime::Duration ScopeTime::DurationTotal() const {
  if (scope_name_.empty()) {
    return Duration{0};
  }

  return ts_.DurationTotal(scope_name_) + DurationSinceReset();
}

ScopeTime::DurationMillis ScopeTime::ElapsedSinceReset() const {
  return std::chrono::duration_cast<DurationMillis>(DurationSinceReset());
}

ScopeTime::DurationMillis ScopeTime::ElapsedTotal(
    const std::string& scope_name) const {
  return std::chrono::duration_cast<DurationMillis>(DurationTotal(scope_name));
}

ScopeTime::DurationMillis ScopeTime::ElapsedTotal() const {
  return std::chrono::duration_cast<DurationMillis>(DurationTotal());
}

}  // namespace tracing

USERVER_NAMESPACE_END
