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
        ++non_standard_codes_;
        LOG_DEBUG() << "Custom grpc::StatusCode " << utils::UnderlyingValue(code);
    }
}

CodeStatistics::Snapshot::Snapshot(const CodeStatistics& other) noexcept {
    for (std::size_t i = 0; i < kCodesCount; ++i) {
        codes_[i] = other.codes_[i].Load();
    }
    non_standard_codes_ = other.non_standard_codes_.Load();
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

    // handle non-standard codes. Non-standard codes are treated as errors,
    // because what else could that be?
    if (non_standard_codes_) {
        summary.total_requests += non_standard_codes_;
        summary.error_requests += non_standard_codes_;

        status_writer.ValueWithLabels(non_standard_codes_, {"grpc_code", "non_standard"});
    }

    return summary;
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
