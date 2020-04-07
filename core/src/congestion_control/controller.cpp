#include <congestion_control/controller.hpp>

#include <logging/log.hpp>

namespace congestion_control {

namespace {

void ValidatePercent(double value, const std::string& name) {
  if (value < 0 || value > 100)
    throw std::runtime_error(
        fmt::format("Validation 0 < x < 100 failed for {} ({})", name, value));
}

const auto kUpRatePercent = "up-rate-percent";
const auto kDownRatePercent = "down-rate-percent";
}  // namespace

Policy MakePolicy(formats::json::Value policy) {
  Policy p;
  p.min_limit = policy["min-limit"].As<int>();
  if (p.min_limit < 0)
    throw std::runtime_error(
        fmt::format("'min-limit' must be non-negative ({})", p.min_limit));

  p.up_rate_percent = policy[kUpRatePercent].As<double>();
  ValidatePercent(p.up_rate_percent, kUpRatePercent);

  p.down_rate_percent = policy[kDownRatePercent].As<double>();
  ValidatePercent(p.down_rate_percent, kDownRatePercent);

  p.overload_on = policy["overload-on-seconds"].As<int>();
  p.overload_off = policy["overload-off-seconds"].As<int>();
  p.up_count = policy["up-level"].As<int>();
  p.down_count = policy["down-level"].As<int>();
  p.no_limit_count = policy["no-limit-seconds"].As<int>();
  return p;
}

Controller::Controller(std::string name, Policy policy)
    : name_(std::move(name)), policy_(policy), is_enabled_(true) {}

bool Controller::IsOverloadedNow(const Sensor::Data& data,
                                 const Policy& policy) const {
  // Use on/off limits for anti-flap
  bool overload_limit =
      state_.is_overloaded ? policy.overload_off : policy.overload_on;
  return data.overload_events_count > overload_limit;
}

size_t Controller::CalcNewLimit(const Sensor::Data& data,
                                const Policy& policy) const {
  if (!state_.current_limit)
    return std::max(policy.min_limit, data.current_load);

  // Use current_limit instead of sensor's current load as the limiter
  // might fail to immediatelly affect sensor's levels
  //
  auto current_load = state_.is_overloaded
                          ? std::min(data.current_load, *state_.current_limit)
                          : *state_.current_limit;
  if (current_load == 0) current_load = 1;

  if (state_.is_overloaded) {
    return std::max<std::size_t>(
        policy.min_limit,
        std::min<std::size_t>(
            std::floor(current_load * (100 - policy.down_rate_percent) / 100),
            current_load - 1));
  } else {
    return std::max<std::size_t>(
        std::floor(current_load * (100 + policy.up_rate_percent) / 100),
        current_load + 1);
  }
}

void Controller::Feed(const Sensor::Data& data) {
  auto policy = policy_.Lock();

  const auto is_overloaded_now = IsOverloadedNow(data, *policy);
  const auto old_overloaded = state_.is_overloaded;

  if (is_overloaded_now) {
    state_.times_with_overload++;
    state_.times_wo_overload = 0;
  } else {
    state_.times_with_overload = 0;
    state_.times_wo_overload++;
  }

  if (state_.is_overloaded) {
    if (is_overloaded_now) {
      state_.current_limit = CalcNewLimit(data, *policy);
    } else if (state_.times_wo_overload > policy->down_count) {
      state_.is_overloaded = false;
    }
  } else {
    if (!is_overloaded_now) {
      if (state_.current_limit)
        state_.current_limit = CalcNewLimit(data, *policy);
    } else if (state_.times_with_overload > policy->up_count) {
      state_.is_overloaded = true;
    }
  }

  if (!state_.is_overloaded &&
      state_.times_wo_overload > policy->no_limit_count)
    state_.current_limit = std::nullopt;

  if (old_overloaded || state_.is_overloaded) {
    if (!old_overloaded)
      LOG_ERROR() << "congestion_control '" << name_ << "' is activated";
    if (!state_.is_overloaded)
      LOG_ERROR() << "congestion_control '" << name_ << "' is deactivated";

    LOG_ERROR() << "congestion control '" << name_
                << "' state: input load=" << data.current_load
                << " input overloads=" << data.overload_events_count
                << " => is_overloaded=" << state_.is_overloaded
                << " current_limit=" << state_.current_limit
                << " times_w=" << state_.times_with_overload
                << " times_wo=" << state_.times_wo_overload;
  }
  limit_.load_limit = state_.current_limit;
}

Limit Controller::GetLimit() const {
  if (is_enabled_.load())
    return limit_;
  else
    return {};
}

void Controller::SetPolicy(const Policy& new_policy) {
  auto policy = policy_.Lock();
  *policy = new_policy;
}

void Controller::SetEnabled(bool enabled) {
  if (enabled != is_enabled_)
    LOG_WARNING() << "congestion control for '" << name_ << "' is "
                  << (enabled ? "enabled" : "disabled");
  is_enabled_ = enabled;
}

}  // namespace congestion_control
