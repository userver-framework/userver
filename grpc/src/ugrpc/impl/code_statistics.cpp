#include <userver/ugrpc/impl/code_statistics.hpp>

#include <limits>

#include <userver/logging/log.hpp>
#include <userver/ugrpc/status_codes.hpp>

#include <userver/utils/enumerate.hpp>
#include <userver/utils/statistics/writer.hpp>
#include <userver/utils/underlying_value.hpp>

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

bool IsZeroWritten(grpc::StatusCode status) {
    switch (status) {
        case grpc::StatusCode::OK:
        case grpc::StatusCode::UNKNOWN:
            return true;
        default:
            return false;
    }
}

}  // namespace

void CodeStatistics::Account(grpc::StatusCode code) noexcept {
    if (static_cast<std::size_t>(code) < kCodesCount) {
        ++codes_[static_cast<std::size_t>(code)];
    } else {
        LOG_ERROR() << "Invalid grpc::StatusCode " << utils::UnderlyingValue(code);
    }
}

CodeStatistics::Snapshot::Snapshot(const CodeStatistics& other) noexcept {
    for (std::size_t i = 0; i < kCodesCount; ++i) {
        codes_[i] = other.codes_[i].Load();
    }
}

CodeStatistics::Snapshot& CodeStatistics::Snapshot::operator+=(const Snapshot& other) {
    for (std::size_t i = 0; i < kCodesCount; ++i) {
        codes_[i] += other.codes_[i];
    }
    return *this;
}

CodeStatisticsSummary CodeStatistics::Snapshot::DumpMetricAndGetSummary(utils::statistics::Writer& writer) const {
    auto status_writer = writer["status"];
    CodeStatisticsSummary summary{};

    for (const auto& [idx, count] : utils::enumerate(codes_)) {
        const auto code = static_cast<grpc::StatusCode>(idx);
        summary.total_requests += count;
        if (IsServerError(code)) summary.error_requests += count;

        if (count || IsZeroWritten(code)) {
            status_writer.ValueWithLabels(count, {"grpc_code", ugrpc::ToString(code)});
        }
    }

    return summary;
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
