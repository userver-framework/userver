#include <userver/ugrpc/impl/statistics.hpp>

#include <userver/logging/log.hpp>
#include <userver/utils/enumerate.hpp>
#include <userver/utils/statistics/writer.hpp>
#include <userver/utils/underlying_value.hpp>

#include <userver/ugrpc/impl/status_codes.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

MethodStatistics::MethodStatistics() {
  for (auto& counter : status_codes_) {
    // TODO remove after atomic value-initialization in C++20
    counter.store(0);
  }
}

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

void DumpMetric(utils::statistics::Writer& writer,
                const MethodStatistics& stats) {
  writer["timings"] = stats.timings_;

  std::uint64_t total_requests = 0;
  std::uint64_t error_requests = 0;

  {
    auto status = writer["status"];
    for (const auto& [idx, counter] : utils::enumerate(stats.status_codes_)) {
      const auto code = static_cast<grpc::StatusCode>(idx);
      const auto count = counter.load();
      total_requests += count;
      if (code != grpc::StatusCode::OK) error_requests += count;
      status.ValueWithLabels(count, {"grpc_code", ugrpc::impl::ToString(code)});
    }
  }

  const auto network_errors_value = stats.network_errors_.load();
  const auto abandoned_errors_value = stats.internal_errors_.load();

  // 'total_requests' and 'error_requests' originally only count RPCs that
  // finished with a status code. 'network_errors' are RPCs that finished
  // abruptly and didn't produce a status code. But these RPCs still need to be
  // included in the totals.
  total_requests += network_errors_value;
  error_requests += network_errors_value;

  writer["active"] = stats.started_.load() - total_requests;
  writer["rps"] = total_requests;
  writer["eps"] = error_requests;

  writer["network-error"] = network_errors_value;
  writer["abandoned-error"] = abandoned_errors_value;
}

ServiceStatistics::~ServiceStatistics() = default;

ServiceStatistics::ServiceStatistics(const StaticServiceMetadata& metadata)
    : metadata_(metadata),
      method_statistics_(metadata.method_full_names.size()) {}

MethodStatistics& ServiceStatistics::GetMethodStatistics(
    std::size_t method_id) {
  return method_statistics_[method_id];
}

const MethodStatistics& ServiceStatistics::GetMethodStatistics(
    std::size_t method_id) const {
  return method_statistics_[method_id];
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
