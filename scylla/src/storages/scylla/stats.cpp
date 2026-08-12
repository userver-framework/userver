#include <storages/scylla/stats.hpp>

#include <userver/storages/scylla/exception.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::stats {

void OperationStatisticsItem::Account(ErrorType error_type, std::chrono::milliseconds elapsed) noexcept {
    ++counters[static_cast<std::size_t>(error_type)];
    const auto ms = elapsed.count();
    timings.GetCurrentCounter().Account(ms);
    timings_sum += utils::statistics::Rate{static_cast<utils::statistics::Rate::ValueType>(ms)};
}

Rate OperationStatisticsItem::GetCounter(ErrorType error_type) const noexcept {
    return counters[static_cast<std::size_t>(error_type)].Load();
}

Rate OperationStatisticsItem::GetTotalQueries() const noexcept {
    Rate result;
    for (const auto& counter : counters) {
        result += counter.Load();
    }
    return result;
}

std::string_view ToString(ErrorType type) {
    switch (type) {
        case ErrorType::kSuccess:
            return "success";
        case ErrorType::kNetwork:
            return "network";
        case ErrorType::kClusterUnavailable:
            return "cluster-unavailable";
        case ErrorType::kAuthFailure:
            return "auth-failure";
        case ErrorType::kBadQueryArgument:
            return "bad-query-arg";
        case ErrorType::kTimeout:
            return "timeout";
        case ErrorType::kServerError:
            return "server-error";
        case ErrorType::kOther:
            return "other";
        case ErrorType::kErrorTypesCount:
            break;
    }
    UINVARIANT(false, "Unexpected ErrorType");
}

ErrorType ClassifyException(const std::exception& ex) noexcept {
    if (dynamic_cast<const TimeoutException*>(&ex)) {
        return ErrorType::kTimeout;
    }
    if (dynamic_cast<const AuthenticationException*>(&ex)) {
        return ErrorType::kAuthFailure;
    }
    if (dynamic_cast<const ClusterUnavailableException*>(&ex)) {
        return ErrorType::kClusterUnavailable;
    }
    if (dynamic_cast<const NetworkException*>(&ex)) {
        return ErrorType::kNetwork;
    }
    if (dynamic_cast<const InvalidQueryArgumentException*>(&ex)) {
        return ErrorType::kBadQueryArgument;
    }
    if (dynamic_cast<const ServerException*>(&ex)) {
        return ErrorType::kServerError;
    }
    return ErrorType::kOther;
}

void DumpMetric(utils::statistics::Writer& writer, const OperationStatisticsItem& item) {
    Rate total_errors;
    for (std::size_t i = 0; i < kErrorTypesCount; ++i) {
        const auto type = static_cast<ErrorType>(i);
        if (type == ErrorType::kSuccess) {
            continue;
        }
        const auto value = item.counters[i].Load();
        total_errors += value;
        writer["errors"].ValueWithLabels(value, {"scylla_error", std::string{ToString(type)}});
    }
    writer["errors-total"].ValueWithLabels(total_errors, {"scylla_error", "total"});
    writer["success"] = item.counters[static_cast<std::size_t>(ErrorType::kSuccess)].Load();
    writer["timings-1min"] = item.timings;
}

void DumpMetric(utils::statistics::Writer& writer, const SessionStatistics& stats) {
    writer["connections"]["created"] = stats.created;
    writer["connections"]["closed"] = stats.closed;
    writer["connections"]["reconnects"] = stats.reconnects;
    writer["ping"] = *stats.ping;
    writer["queries"] = *stats.queries;
}

}  // namespace storages::scylla::stats

USERVER_NAMESPACE_END
