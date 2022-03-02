#include <userver/ugrpc/impl/statistics.hpp>

#include <algorithm>

#include <userver/formats/json/value_builder.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/enumerate.hpp>
#include <userver/utils/statistics/percentile_format_json.hpp>
#include <userver/utils/statistics/storage.hpp>
#include <userver/utils/underlying_value.hpp>

#include <userver/ugrpc/impl/status_codes.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

MethodStatistics::MethodStatistics() = default;

void MethodStatistics::AccountStatus(grpc::StatusCode code) noexcept {
  if (static_cast<std::size_t>(code) < kCodesCount) {
    ++status_codes[static_cast<std::size_t>(code)];
  } else {
    LOG_ERROR_TO(logging::DefaultLoggerOptional())
        << "Invalid grpc::StatusCode " << utils::UnderlyingValue(code);
  }
}

void MethodStatistics::AccountTiming(
    std::chrono::milliseconds timing) noexcept {
  timings.GetCurrentCounter().Account(timing.count());
}

void MethodStatistics::AccountNetworkError() noexcept { ++network_errors; }

void MethodStatistics::AccountInternalError() noexcept { ++internal_errors; }

formats::json::Value MethodStatistics::ExtendStatistics() const {
  formats::json::ValueBuilder result(formats::json::Type::kObject);
  result["timings"]["1min"] =
      utils::statistics::PercentileToJson(timings.GetStatsForPeriod());

  std::uint64_t total_requests = 0;
  std::uint64_t error_requests = 0;
  formats::json::ValueBuilder status(formats::json::Type::kObject);

  for (const auto& [idx, counter] : utils::enumerate(status_codes)) {
    const auto code = static_cast<grpc::StatusCode>(idx);
    const auto count = counter.Load();
    total_requests += count;
    if (code != grpc::StatusCode::OK) error_requests += count;
    status[std::string{ugrpc::impl::ToString(code)}] = count;
  }

  const auto network_errors_value = network_errors.Load();
  const auto internal_errors_value = internal_errors.Load();
  const auto no_code_requests = network_errors_value + internal_errors_value;

  result["rps"] = total_requests + no_code_requests;
  result["eps"] = error_requests + no_code_requests;
  result["status"] = std::move(status);

  result["network-error"] = network_errors_value;
  result["abandoned-error"] = internal_errors_value;

  return result.ExtractValue();
}

ServiceStatistics::~ServiceStatistics() = default;

ServiceStatistics::ServiceStatistics(const StaticServiceMetadata& metadata)
    : metadata_(metadata), method_statistics_(metadata.method_count) {}

MethodStatistics& ServiceStatistics::GetMethodStatistics(
    std::size_t method_id) {
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
  return result.ExtractValue();
}

utils::statistics::Entry ServiceStatistics::Register(
    std::string prefix, utils::statistics::Storage& statistics_storage) {
  return statistics_storage.RegisterExtender(
      {"grpc", std::move(prefix), std::string{metadata_.service_full_name}},
      [this](const auto& /*request*/) { return ExtendStatistics(); });
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
