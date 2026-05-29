#pragma once

#include <optional>

#include <server/middlewares/handler_metrics.hpp>
#include <userver/logging/level.hpp>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/utils/optional_ref.hpp>
#include <userver/utils/statistics/labels.hpp>
#include <utils/statistics/impl/statistics_key.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {
class HttpHandlerStatistics;
class HttpHandlerStatisticsScope;
}  // namespace server::handlers

namespace server::request::impl {

class DeadlinePropagationContext final {
public:
    void SetCancelledByDeadline();
    bool IsCancelledByDeadline() const;

    void SetForcedLogLevel(logging::Level level);
    const std::optional<logging::Level>& GetForcedLogLevel() const;

private:
    bool is_cancelled_by_deadline_{false};
    std::optional<logging::Level> forced_log_level_{};
};

class InternalRequestContext final {
public:
    InternalRequestContext();
    InternalRequestContext(InternalRequestContext&&) noexcept;
    InternalRequestContext(const InternalRequestContext&) = delete;

    void SetConfigSnapshot(dynamic_config::Snapshot&& config_snapshot);
    const dynamic_config::Snapshot& GetConfigSnapshot() const;
    void ResetConfigSnapshot();

    void SetHandlerMetricsShard(std::string_view sensor_path_part, utils::statistics::LabelsSpan labels);
    void SaveHttpHandlerStatisticsScope(handlers::HttpHandlerStatisticsScope& statistics_scope);
    void RemoveHttpHandlerStatisticsScope();

    DeadlinePropagationContext& GetDPContext();

private:
    std::optional<dynamic_config::Snapshot> config_snapshot_;
    handlers::HttpHandlerStatisticsScope* statistics_scope_{nullptr};
    DeadlinePropagationContext dp_context_{};
};

}  // namespace server::request::impl

USERVER_NAMESPACE_END
