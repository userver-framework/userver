#include <clients/http/statistics.hpp>

#include <utility>

#include <curl-ev/error_code.hpp>

#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/enumerate.hpp>
#include <userver/utils/statistics/common.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

namespace {

template <typename T, typename U>
T SumToMean(T sum, U count) {
  if (count == 0) return 0;
  return sum / static_cast<T>(count);
}

}  // namespace

RequestStats::RequestStats(Statistics& stats) : stats_(&stats) {
  stats_->easy_handles_++;
}

RequestStats::~RequestStats() {
  if (stats_) {
    stats_->easy_handles_--;
  }
}

RequestStats::RequestStats(RequestStats&& other) noexcept
    : stats_{std::exchange(other.stats_, nullptr)} {}

void RequestStats::Start() { start_time_ = std::chrono::steady_clock::now(); }

void RequestStats::FinishOk(int code, unsigned int attempts) noexcept {
  UASSERT(stats_);
  stats_->AccountError(Statistics::ErrorGroup::kOk);
  stats_->AccountStatus(code);
  if (attempts > 1) stats_->retries_ += utils::statistics::Rate{attempts - 1};
  StoreTiming();
}

void RequestStats::FinishEc(std::error_code ec,
                            unsigned int attempts) noexcept {
  UASSERT(stats_);
  stats_->AccountError(Statistics::ErrorCodeToGroup(ec));
  if (attempts > 1) stats_->retries_ += utils::statistics::Rate{attempts - 1};
  StoreTiming();
}

void RequestStats::StoreTiming() noexcept {
  UASSERT(stats_);
  auto now = std::chrono::steady_clock::now();
  auto diff = now - start_time_;
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();
  stats_->timings_percentile_.GetCurrentCounter().Account(ms);
}

void RequestStats::StoreTimeToStart(
    std::chrono::microseconds micro_seconds) noexcept {
  UASSERT(stats_);
  /* There is a race between multiple easy handles, we don't care which of them
   * writes its time_to_start. If boost::asio pool is full, we'll see big
   * numbers anyway. */
  stats_->last_time_to_start_us_ = micro_seconds.count();
}

void RequestStats::AccountOpenSockets(size_t sockets) noexcept {
  UASSERT(stats_);
  stats_->socket_open_ += utils::statistics::Rate{sockets};
}

void RequestStats::AccountTimeoutUpdatedByDeadline() noexcept {
  UASSERT(stats_);
  ++stats_->timeout_updated_by_deadline_;
}

void RequestStats::AccountCancelledByDeadline() noexcept {
  UASSERT(stats_);
  ++stats_->cancelled_by_deadline_;
}

Statistics::ErrorGroup Statistics::ErrorCodeToGroup(std::error_code ec) {
  using ErrorCode = curl::errc::EasyErrorCode;

  if (ec == std::errc::operation_canceled) {
    return ErrorGroup::kCancelled;
  }

  if (ec.category() != curl::errc::GetEasyCategory()) {
    return ErrorGroup::kUnknown;
  }

  switch (static_cast<ErrorCode>(ec.value())) {
    case ErrorCode::kCouldNotResolveHost:
      return ErrorGroup::kHostResolutionFailed;

    case ErrorCode::kOperationTimedout:
      return ErrorGroup::kTimeout;

    case ErrorCode::kSslConnectError:
    case ErrorCode::kPeerFailedVerification:
    case ErrorCode::kSslCipher:
    case ErrorCode::kSslCertproblem:
    case ErrorCode::kSslCacertBadfile:
    case ErrorCode::kSslIssuerError:
    case ErrorCode::kSslCrlBadfile:
      return ErrorGroup::kSslError;

    case ErrorCode::kTooManyRedirects:
      return ErrorGroup::kTooManyRedirects;

    case ErrorCode::kSendError:
    case ErrorCode::kRecvError:
    case ErrorCode::kCouldNotConnect:
      return ErrorGroup::kSocketError;

    default:
      return ErrorGroup::kUnknown;
  }
}

const char* Statistics::ToString(ErrorGroup error) {
  switch (error) {
    case ErrorGroup::kOk:
      return "ok";
    case ErrorGroup::kHostResolutionFailed:
      return "host-resolution-failed";
    case ErrorGroup::kSocketError:
      return "socket-error";
    case ErrorGroup::kTimeout:
      return "timeout";
    case ErrorGroup::kSslError:
      return "ssl-error";
    case ErrorGroup::kTooManyRedirects:
      return "too-many-redirects";
    case ErrorGroup::kCancelled:
      return "cancelled";
    case ErrorGroup::kUnknown:
    case ErrorGroup::kCount:
      break;
  }
  return "unknown-error";
}

void Statistics::AccountError(ErrorGroup error) {
  ++error_count_[static_cast<int>(error)];
}

void Statistics::AccountStatus(int code) { reply_status_.Account(code); }

void DumpMetric(utils::statistics::Writer& writer,
                const DestinationStatisticsView& view) {
  const auto& stats = view.stats;

  writer["timings"] = stats.timings_percentile;

  for (std::size_t i = 0; i < Statistics::kErrorGroupCount; i++) {
    const auto error_group = static_cast<Statistics::ErrorGroup>(i);
    writer["errors"].ValueWithLabels(
        stats.error_count[i],
        {"http_error", Statistics::ToString(error_group)});
  }

  writer["reply-statuses"] = stats.reply_status;

  writer["retries"] = stats.retries;
  writer["pending-requests"] = stats.easy_handles;

  writer["timeout-updated-by-deadline"] = stats.timeout_updated_by_deadline;
  writer["cancelled-by-deadline"] = stats.cancelled_by_deadline;

  writer["sockets"]["open"] = stats.multi.socket_open;
}

void DumpMetric(utils::statistics::Writer& writer,
                const InstanceStatistics& stats) {
  writer = DestinationStatisticsView{stats};

  writer["last-time-to-start-us"] =
      SumToMean(stats.last_time_to_start_us, stats.instances_aggregated);
  writer["event-loop-load"][utils::statistics::DurationToString(
      utils::statistics::kDefaultMaxPeriod)] =
      SumToMean(stats.multi.current_load, stats.instances_aggregated);

  // Destinations may reuse sockets from other destination,
  // it is very unjust to account active/closed sockets
  writer["sockets"]["close"] = stats.multi.socket_close;
  writer["sockets"]["throttled"] = stats.multi.socket_ratelimit;
  writer["sockets"]["active"] = utils::statistics::Rate{
      stats.multi.socket_open.value - stats.multi.socket_close.value};
}

void DumpMetric(utils::statistics::Writer& writer,
                const PoolStatistics& stats) {
  InstanceStatistics sum_stats;

  for (const auto& stat : stats.multi) {
    sum_stats += stat;
  }

  writer.ValueWithLabels(sum_stats, {"version", "2"});
}

InstanceStatistics::InstanceStatistics(const Statistics& other)
    : easy_handles(other.easy_handles_.load()),
      last_time_to_start_us(other.last_time_to_start_us_.load()),
      timings_percentile(other.timings_percentile_.GetStatsForPeriod()),
      retries(other.retries_.Load()),
      timeout_updated_by_deadline(other.timeout_updated_by_deadline_.Load()),
      cancelled_by_deadline(other.cancelled_by_deadline_.Load()),
      reply_status(other.reply_status_) {
  for (size_t i = 0; i < error_count.size(); i++)
    error_count[i] = other.error_count_[i].Load();
  multi.socket_open = other.socket_open_.Load();
}

uint64_t InstanceStatistics::GetNotOkErrorCount() const {
  uint64_t result{0};

  for (size_t i = 0; i < Statistics::kErrorGroupCount; i++) {
    auto error_group = static_cast<Statistics::ErrorGroup>(i);
    if (error_group == ErrorGroup::kOk) continue;

    result += error_count[i].value;
  }
  return result;
}

InstanceStatistics& InstanceStatistics::operator+=(
    const InstanceStatistics& stat) {
  instances_aggregated += stat.instances_aggregated;

  easy_handles += stat.easy_handles;
  last_time_to_start_us += stat.last_time_to_start_us;

  timings_percentile.Add(stat.timings_percentile);

  for (size_t i = 0; i < Statistics::kErrorGroupCount; i++) {
    error_count[i] += stat.error_count[i];
  }
  retries += stat.retries;

  timeout_updated_by_deadline += stat.timeout_updated_by_deadline;
  cancelled_by_deadline += stat.cancelled_by_deadline;
  reply_status += stat.reply_status;

  multi += stat.multi;
  return *this;
}

}  // namespace clients::http

USERVER_NAMESPACE_END
