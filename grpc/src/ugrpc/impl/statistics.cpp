#include <userver/ugrpc/impl/statistics.hpp>

#include <userver/logging/log.hpp>
#include <userver/utils/enumerate.hpp>
#include <userver/utils/statistics/writer.hpp>
#include <userver/utils/underlying_value.hpp>

#include <userver/ugrpc/status_codes.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

namespace {

// See https://opentelemetry.io/docs/specs/semconv/rpc/grpc/
// Except that we don't mark DEADLINE_EXCEEDED as a server error.
bool IsServerError(grpc::StatusCode status) {
  switch (status) {
    case grpc::StatusCode::UNKNOWN:
    case grpc::StatusCode::UNIMPLEMENTED:
    case grpc::StatusCode::INTERNAL:
    case grpc::StatusCode::UNAVAILABLE:
    case grpc::StatusCode::DATA_LOSS:
      return true;
    default:
      return false;
  }
}

// For now, we need to dump metrics in legacy format - without 'rate'
// support - in order not to break existing dashboards
struct AsRateAndGauge final {
  utils::statistics::Rate value;
};

void DumpMetric(utils::statistics::Writer& writer,
                const AsRateAndGauge& stats) {
  // old format. Currently it is 'rate' as well, as we are
  // in our final stages of fully moving to RateCounter
  // See https://st.yandex-team.ru/TAXICOMMON-6872
  writer = stats.value;
  // new format, with 'rate' support but into different sensor
  // This sensor will be deleted soon.
  writer["v2"] = stats.value;
}

}  // namespace

MethodStatistics::MethodStatistics(StatisticsDomain domain) : domain_(domain) {}

void MethodStatistics::AccountStarted() noexcept { ++started_; }

void MethodStatistics::AccountStatus(grpc::StatusCode code) noexcept {
  if (static_cast<std::size_t>(code) < kCodesCount) {
    ++status_codes_[static_cast<std::size_t>(code)];
  } else {
    LOG_ERROR() << "Invalid grpc::StatusCode " << utils::UnderlyingValue(code);
  }
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
                const MethodStatistics& stats) {
  if (stats.domain_ == StatisticsDomain::kClient && !stats.started_.Load()) {
    // Don't write client metrics for methods, for which the current service has
    // not performed any requests since the start.
    //
    // gRPC service clients may have tens of methods, of which only a few are
    // actually used by the current service. Not writing extra metrics saves
    // metrics quota and mirrors the behavior for HTTP destinations.
    return;
  }

  writer["timings"] = stats.timings_;

  utils::statistics::Rate total_requests{0};
  utils::statistics::Rate error_requests{0};

  {
    auto status = writer["status"];
    for (const auto& [idx, counter] : utils::enumerate(stats.status_codes_)) {
      const auto code = static_cast<grpc::StatusCode>(idx);
      const auto count = counter.Load();
      total_requests += count;
      if (IsServerError(code)) error_requests += count;
      status.ValueWithLabels(AsRateAndGauge{count},
                             {"grpc_code", ugrpc::ToString(code)});
    }
  }

  const auto network_errors_value = stats.network_errors_.Load();
  // 'abandoned_errors' need not be accounted in eps or rps, because such
  // requests also separately report the error that occurred during the
  // automatic request termination.
  const auto abandoned_errors_value = stats.internal_errors_.Load();
  const auto deadline_cancelled_value = stats.deadline_cancelled_.Load();
  const auto cancelled_value = stats.cancelled_.Load();

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
  writer["active"] = static_cast<std::int64_t>(stats.started_.Load().value) -
                     static_cast<std::int64_t>(total_requests.value);

  writer["rps"] = AsRateAndGauge{total_requests};
  writer["eps"] = error_requests;

  writer["network-error"] = AsRateAndGauge{network_errors_value};
  writer["abandoned-error"] = AsRateAndGauge{abandoned_errors_value};
  writer["cancelled"] = AsRateAndGauge{cancelled_value};

  writer["deadline-propagated"] =
      AsRateAndGauge{stats.deadline_updated_.Load()};
  writer["cancelled-by-deadline-propagation"] =
      AsRateAndGauge{deadline_cancelled_value};
}

std::uint64_t MethodStatistics::GetStarted() const noexcept {
  return started_.Load().value;
}

ServiceStatistics::~ServiceStatistics() = default;

ServiceStatistics::ServiceStatistics(const StaticServiceMetadata& metadata,
                                     StatisticsDomain domain)
    : metadata_(metadata),
      method_statistics_(metadata.method_full_names.size(), domain) {}

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

void DumpMetric(utils::statistics::Writer& writer,
                const ServiceStatistics& stats) {
  const auto service_full_name = stats.metadata_.service_full_name;
  for (const auto& [i, method_full_name] :
       utils::enumerate(stats.metadata_.method_full_names)) {
    const auto method_name =
        method_full_name.substr(stats.metadata_.service_full_name.size() + 1);
    writer.ValueWithLabels(stats.method_statistics_[i],
                           {{"grpc_service", service_full_name},
                            {"grpc_method", method_name},
                            {"grpc_destination", method_full_name}});
  }
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
