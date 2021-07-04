#include <userver/congestion_control/controller.hpp>

#include <userver/logging/log.hpp>

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
  p.load_limit_percent = policy["load-limit-percent"].As<int>(0);
  p.load_limit_crit_percent = policy["load-limit-crit-percent"].As<int>(101);
  p.start_limit_factor = policy["start-limit-factor"].As<double>(0.75);
  return p;
}

Controller::Controller(std::string name, Policy policy)
    : name_(std::move(name)), policy_(policy), is_enabled_(true) {}

bool Controller::IsOverloadedNow(const Sensor::Data& data,
                                 const Policy& policy) const {
  // Use on/off limits for anti-flap
  bool overload_limit =
      state_.is_overloaded ? policy.down_count : policy.up_count;

  if (data.overload_events_count <= overload_limit) {
    // spurious noise
    return false;
  }

  if (!IsThresholdReached(data, policy.load_limit_percent)) {
    // there was an overload peak, but now it's gone
    return false;
  }

  // there were overloads during all previous second,
  // it's not a flap, but a serious overloaded state
  return true;
}

bool Controller::IsThresholdReached(const Sensor::Data& data, int percent) {
  /*
   * Transform the equation to eliminate division operations:
   *
   * V * 100 >= (V + N) * p
   *   <=>
   * V / (V + N) >= p / 100  and (V + N) != 0
   */
  return (data.overload_events_count * 100 >=
          (data.overload_events_count + data.no_overload_events_count) *
              percent);
}

size_t Controller::CalcNewLimit(const Sensor::Data& data,
                                const Policy& policy) const {
  if (!state_.current_limit) {
    return std::max<size_t>(
        policy.min_limit,
        std::lround(data.current_load * policy.start_limit_factor));
  }

  // Use current_limit instead of sensor's current load as the limiter
  // might fail to immediatelly affect sensor's levels
  //
  auto current_load = *state_.current_limit;
  if (current_load == 0) current_load = 1;

  if (state_.is_overloaded) {
    return std::max<std::size_t>(
        policy.min_limit,
        std::min<std::size_t>(
            std::floor(current_load * (100 - policy.down_rate_percent) / 100),
            current_load - 1));
  } else {
    const auto max_up =
        std::lround(current_load * (100 + policy.up_rate_percent) / 100);
    const auto up =
        std::min<size_t>(max_up, current_load + state_.max_up_delta);

    /* New RPS is limited by:
     * - current*K and
     * - current+2**N
     * where N is count of seconds in a row w/o overloads.
     */
    return std::max<std::size_t>(up, current_load + 1);
  }
}

void Controller::Feed(const Sensor::Data& data) {
  auto policy = policy_.Lock();

  const auto is_overloaded_pressure = IsOverloadedNow(data, *policy);
  const auto old_overloaded = state_.is_overloaded;

  if (is_overloaded_pressure) {
    state_.times_with_overload++;
    state_.times_wo_overload = 0;

    /* If we're raising RPS, but faced with a random overload, slow down a bit.
     */
    state_.max_up_delta = 1;
  } else {
    state_.times_with_overload = 0;
    state_.times_wo_overload++;

    // avoid overlfow
    if (state_.max_up_delta < 10000) {
      // If we're raising RPS and have no overflows, speed up the rise
      state_.max_up_delta *= 2;
    }
  }

  if (state_.is_overloaded) {
    if (is_overloaded_pressure) {
      state_.current_limit = CalcNewLimit(data, *policy);

      stats_.overload_pressure++;
      stats_.last_overload_pressure =
          std::chrono::duration_cast<std::chrono::seconds>(
              std::chrono::steady_clock::now().time_since_epoch());
      stats_.current_state = 4;
    } else {
      if (state_.times_wo_overload > policy->overload_off) {
        state_.is_overloaded = false;
      }

      stats_.overload_no_pressure++;
      stats_.current_state = 3;
    }
  } else {
    if (!is_overloaded_pressure) {
      if (state_.current_limit) {
        state_.current_limit = CalcNewLimit(data, *policy);

        stats_.not_overload_no_pressure++;
        stats_.current_state = 1;
      } else {
        stats_.no_limit++;
        stats_.current_state = 0;
      }
    } else {
      if (state_.times_with_overload > policy->overload_on ||
          IsThresholdReached(data, policy->load_limit_crit_percent)) {
        state_.is_overloaded = true;
      }

      stats_.not_overload_pressure++;
      stats_.current_state = 2;
    }
  }

  if (!state_.is_overloaded &&
      state_.times_wo_overload > policy->no_limit_count)
    state_.current_limit = std::nullopt;

  auto log_level = state_.is_overloaded
                       ? logging::Level::kError
                       : (state_.current_limit ? logging::Level::kWarning
                                               : logging::Level::kInfo);
  std::string log_suffix;

  if (!is_enabled_) {
    log_level = logging::Level::kInfo;
    log_suffix = " (running in fake mode, RPS limit is not forced)";
  }

  if (old_overloaded || state_.is_overloaded) {
    if (!old_overloaded)
      LOG(log_level) << "congestion_control '" << name_ << "' is activated";
    if (!state_.is_overloaded)
      LOG(log_level) << "congestion_control '" << name_ << "' is deactivated";
  }

  auto load_prc = data.GetLoadPercent();

  // Log if:
  // - CC is already enabled
  // - there is some overloads noise
  // - CC was enabled recently
  if (log_level > logging::Level::kInfo || load_prc > 0.01 ||
      state_.current_limit) {
    auto load_prc_str = fmt::format(" ({:.2f}%)", load_prc);
    LOG(log_level) << "congestion control '" << name_
                   << "' state: input load=" << data.current_load
                   << " input overloads=" << data.overload_events_count
                   << load_prc_str
                   << " => is_overloaded=" << state_.is_overloaded
                   << " current_limit=" << state_.current_limit
                   << " times_w=" << state_.times_with_overload
                   << " times_wo=" << state_.times_wo_overload
                   << " max_up_delta=" << state_.max_up_delta << log_suffix;
  }
  limit_.load_limit = state_.current_limit;
}

Limit Controller::GetLimit() const {
  if (is_enabled_.load())
    return GetLimitRaw();
  else
    return {};
}

Limit Controller::GetLimitRaw() const { return limit_; }

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

bool Controller::IsEnabled() const { return is_enabled_; }

const Stats& Controller::GetStats() const { return stats_; }

}  // namespace congestion_control
