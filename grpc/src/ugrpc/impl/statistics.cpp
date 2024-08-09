#include <userver/ugrpc/impl/statistics.hpp>

#include <userver/utils/enumerate.hpp>
#include <userver/utils/impl/internal_tag.hpp>
#include <userver/utils/statistics/striped_rate_counter.hpp>
#include <userver/utils/statistics/writer.hpp>

#include <ugrpc/impl/rate_and_gauge.hpp>
#include <userver/ugrpc/status_codes.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

MethodStatistics::MethodStatistics(
    StatisticsDomain domain,
    utils::statistics::StripedRateCounter& global_started)
    : domain_(domain), global_started_(global_started) {}

void MethodStatistics::AccountStarted() noexcept {
  ++started_;
  ++global_started_;
}

void MethodStatistics::AccountStatus(grpc::StatusCode code) noexcept {
  status_codes_.Account(code);
}

void MethodStatistics::AccountTiming(
    std::chrono::milliseconds timing) noexcept {
  timings_.GetCurrentCounter().Account(timing.count());
}

void MethodStatistics::AccountNetworkError() noexcept { ++network_errors_; }

void MethodStatistics::AccountInternalError() noexcept { ++internal_errors_; }

void MethodStatistics::AccountCancelledByDeadlinePropagation() noexcept {
  ++deadline_cancelled_;
}

void MethodStatistics::AccountDeadlinePropagated() noexcept {
  ++deadline_updated_;
}

void MethodStatistics::AccountCancelled() noexcept { ++cancelled_; }

void DumpMetric(utils::statistics::Writer& writer,
                const MethodStatisticsSnapshot& stats) {
  if (stats.domain == StatisticsDomain::kClient && !stats.started) {
    // Don't write client metrics for methods, for which the current service has
    // not performed any requests since the start.
    //
    // gRPC service clients may have tens of methods, of which only a few are
    // actually used by the current service. Not writing extra metrics saves
    // metrics quota and mirrors the behavior for HTTP destinations.
    return;
  }

  writer["timings"] = stats.timings;

  utils::statistics::Rate total_requests{0};
  utils::statistics::Rate error_requests{0};

  {
    auto cnt = stats.status_codes.DumpMetricHelper(writer);
    total_requests += cnt.total_requests;
    error_requests += cnt.error_requests;
  }

  const auto network_errors_value = stats.network_errors;
  // 'abandoned_errors' need not be accounted in eps or rps, because such
  // requests also separately report the error that occurred during the
  // automatic request termination.
  const auto abandoned_errors_value = stats.internal_errors;
  const auto deadline_cancelled_value = stats.deadline_cancelled;
  const auto cancelled_value = stats.cancelled;
  const auto started_renamed = stats.started_renamed;

  // 'total_requests' and 'error_requests' originally only count RPCs that
  // finished with a status code. 'network_errors', 'deadline_cancelled' and
  // 'cancelled' are RPCs that finished abruptly and didn't produce a status
  // code. But these RPCs still need to be included in the totals.

  // Network errors are not considered to be server errors, because either the
  // client is responsible for the server dropping the request (`TryCancel`,
  // deadline), or it is truly a network error, in which case it's typically
  // helpful for troubleshooting to say that there are issues not with the
  // uservice process itself, but with the infrastructure.
  total_requests += network_errors_value;

  // Deadline propagation is considered a client-side error: the client likely
  // caused the error by giving the server an insufficient deadline.
  total_requests += deadline_cancelled_value;

  // Cancellation is triggered by deadline propagation or by the client
  // dropping the RPC, it is not a server-side error.
  total_requests += cancelled_value;

  // "active" is not a rate metric. Also, beware of overflow
  writer["active"] = static_cast<std::int64_t>(stats.started.value) -
                     static_cast<std::int64_t>(started_renamed.value) -
                     static_cast<std::int64_t>(total_requests.value);

  writer["rps"] = AsRateAndGauge{total_requests};
  writer["eps"] = error_requests;

  writer["network-error"] = AsRateAndGauge{network_errors_value};
  writer["abandoned-error"] = AsRateAndGauge{abandoned_errors_value};
  writer["cancelled"] = AsRateAndGauge{cancelled_value};

  writer["deadline-propagated"] = AsRateAndGauge{stats.deadline_updated};
  writer["cancelled-by-deadline-propagation"] =
      AsRateAndGauge{deadline_cancelled_value};
}

void DumpMetric(utils::statistics::Writer& writer,
                const MethodStatistics& stats) {
  writer = MethodStatisticsSnapshot{stats};
}

MethodStatisticsSnapshot::MethodStatisticsSnapshot(
    const StatisticsDomain domain)
    : domain(domain) {}

MethodStatisticsSnapshot::MethodStatisticsSnapshot(
    const MethodStatistics& stats)
    : domain(stats.domain_),
      started_renamed(stats.started_renamed_.Load()),
      status_codes(stats.status_codes_),
      timings(stats.timings_.GetStatsForPeriod()),
      network_errors(stats.network_errors_.Load()),
      internal_errors(stats.internal_errors_.Load()),
      cancelled(stats.cancelled_.Load()),
      deadline_updated(stats.deadline_updated_.Load()),
      deadline_cancelled(stats.deadline_cancelled_.Load()) {
  // For the 'active' metric, it is important to load the 'started' value after
  // loading the 'started_renamed' and 'total_requests' values.
  // More details in DumpMetric for MethodStatisticsSnapshot
  started = stats.started_.Load();
}

void MethodStatisticsSnapshot::Add(const MethodStatisticsSnapshot& other) {
  UASSERT(domain == other.domain);

  started += other.started;
  started_renamed += other.started_renamed;
  status_codes += other.status_codes;
  timings.Add(other.timings);
  network_errors += other.network_errors;
  internal_errors += other.internal_errors;
  cancelled += other.cancelled;
  deadline_updated += other.deadline_updated;
  deadline_cancelled += other.deadline_cancelled;
}

std::uint64_t MethodStatistics::GetStarted() const noexcept {
  return started_.Load().value - started_renamed_.Load().value;
}

void MethodStatistics::MoveStartedTo(MethodStatistics& other) noexcept {
  UASSERT(&global_started_ == &other.global_started_);
  // Any increment order may result in minor logical inconsistencies
  // on the dashboard at times. Oh well.
  ++started_renamed_;
  ++other.started_;
}

ServiceStatistics::~ServiceStatistics() = default;

ServiceStatistics::ServiceStatistics(
    const StaticServiceMetadata& metadata, StatisticsDomain domain,
    utils::statistics::StripedRateCounter& global_started)
    : metadata_(metadata),
      method_statistics_(metadata.method_full_names.size(), domain,
                         global_started) {}

MethodStatistics& ServiceStatistics::GetMethodStatistics(
    std::size_t method_id) {
  return method_statistics_[method_id];
}

const MethodStatistics& ServiceStatistics::GetMethodStatistics(
    std::size_t method_id) const {
  return method_statistics_[method_id];
}

std::uint64_t ServiceStatistics::GetStartedRequests() const {
  std::uint64_t result{0};
  for (const auto& stats : method_statistics_) {
    result += stats.GetStarted();
  }
  return result;
}

const StaticServiceMetadata& ServiceStatistics::GetMetadata() const {
  return metadata_;
}

void ServiceStatistics::DumpAndCountTotal(
    utils::statistics::Writer& writer, MethodStatisticsSnapshot& total) const {
  const auto service_full_name = metadata_.service_full_name;
  for (const auto& [i, method_full_name] :
       utils::enumerate(metadata_.method_full_names)) {
    const auto method_name =
        method_full_name.substr(metadata_.service_full_name.size() + 1);

    writer.WithLabels(
        utils::impl::InternalTag{},
        {
            {"grpc_service", service_full_name},
            {"grpc_method", method_name},
            {"grpc_destination", method_full_name},
        },
        [&total, &method_statistic =
                     method_statistics_[i]](utils::statistics::Writer& writer) {
          const MethodStatisticsSnapshot snapshot{method_statistic};
          writer = snapshot;
          total.Add(snapshot);
        });
  }
}

static_assert(utils::statistics::kHasWriterSupport<MethodStatisticsSnapshot>);
static_assert(utils::statistics::kHasWriterSupport<MethodStatistics>);

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
