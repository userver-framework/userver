#include <userver/ugrpc/impl/statistics.hpp>

#include <userver/formats/json/value_builder.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/enumerate.hpp>
#include <userver/utils/statistics/metadata.hpp>
#include <userver/utils/statistics/percentile_format_json.hpp>
#include <userver/utils/statistics/storage.hpp>
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

formats::json::Value MethodStatistics::ExtendStatistics() const {
  formats::json::ValueBuilder result(formats::json::Type::kObject);
  result["timings"]["1min"] =
      utils::statistics::PercentileToJson(timings_.GetStatsForPeriod());
  utils::statistics::SolomonSkip(result["timings"]["1min"]);

  std::uint64_t total_requests = 0;
  std::uint64_t error_requests = 0;
  formats::json::ValueBuilder status(formats::json::Type::kObject);

  for (const auto& [idx, counter] : utils::enumerate(status_codes_)) {
    const auto code = static_cast<grpc::StatusCode>(idx);
    const auto count = counter.load();
    total_requests += count;
    if (code != grpc::StatusCode::OK) error_requests += count;
    status[std::string{ugrpc::impl::ToString(code)}] = count;
  }
  utils::statistics::SolomonChildrenAreLabelValues(status, "grpc_code");

  const auto network_errors_value = network_errors_.load();
  const auto abandoned_errors_value = internal_errors_.load();

  // 'total_requests' and 'error_requests' originally only count RPCs that
  // finished with a status code. 'network_errors' are RPCs that finished
  // abruptly and didn't produce a status code. But these RPCs still need to be
  // included in the totals.
  total_requests += network_errors_value;
  error_requests += network_errors_value;

  result["active"] = started_.load() - total_requests;
  result["rps"] = total_requests;
  result["eps"] = error_requests;
  result["status"] = std::move(status);

  result["network-error"] = network_errors_value;
  result["abandoned-error"] = abandoned_errors_value;

  return result.ExtractValue();
}

ServiceStatistics::~ServiceStatistics() = default;

ServiceStatistics::ServiceStatistics(const StaticServiceMetadata& metadata)
    : metadata_(metadata), method_statistics_(metadata.method_count) {}

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

formats::json::Value ServiceStatistics::ExtendStatistics() const {
  formats::json::ValueBuilder result(formats::json::Type::kObject);
  for (std::size_t i = 0; i < metadata_.method_count; ++i) {
    const auto method_name = metadata_.method_full_names[i].substr(
        metadata_.service_full_name.size() + 1);
    result[std::string{method_name}] = method_statistics_[i].ExtendStatistics();
  }
  utils::statistics::SolomonChildrenAreLabelValues(result, "grpc_method");
  utils::statistics::SolomonLabelValue(result, "grpc_service");
  return result.ExtractValue();
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
