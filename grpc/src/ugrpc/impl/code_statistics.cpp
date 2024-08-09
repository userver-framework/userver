#include <userver/ugrpc/impl/code_statistics.hpp>

#include <limits>

#include <userver/logging/log.hpp>
#include <userver/ugrpc/status_codes.hpp>

#include <userver/utils/enumerate.hpp>
#include <userver/utils/statistics/writer.hpp>
#include <userver/utils/underlying_value.hpp>

#include <ugrpc/impl/rate_and_gauge.hpp>

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

}  // namespace

void CodeStatistics::Account(grpc::StatusCode code) noexcept {
  if (static_cast<std::size_t>(code) < CodeStatistics::kCodesCount) {
    codes_[static_cast<std::size_t>(code)]++;
  } else {
    LOG_ERROR() << "Invalid grpc::StatusCode " << utils::UnderlyingValue(code);
  }
}

CodeStatistics::Snapshot::Snapshot(const CodeStatistics& other) noexcept {
  for (std::size_t i = 0; i < codes_.size(); ++i) {
    codes_[i] = other.codes_[i].Load();
  }
}

CodeStatistics::Snapshot& CodeStatistics::Snapshot::operator+=(
    const Snapshot& other) {
  for (std::size_t i = 0; i < codes_.size(); ++i) {
    codes_[i] += other.codes_[i];
  }
  return *this;
}

CodeStatisticsSummary CodeStatistics::Snapshot::DumpMetricHelper(
    utils::statistics::Writer& writer) const {
  auto status = writer["status"];

  CodeStatisticsSummary cnt{};
  for (const auto& [idx, count] : utils::enumerate(codes_)) {
    const auto code = static_cast<grpc::StatusCode>(idx);
    cnt.total_requests += count;
    if (IsServerError(code)) cnt.error_requests += count;
    status.ValueWithLabels(AsRateAndGauge{count},
                           {"grpc_code", ugrpc::ToString(code)});
  }
  return cnt;
}

void DumpMetric(utils::statistics::Writer& writer,
                const CodeStatistics::Snapshot& snapshot) {
  snapshot.DumpMetricHelper(writer);
}

static_assert(utils::statistics::kHasWriterSupport<CodeStatistics::Snapshot>);

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
