#pragma once

/// @file userver/server/request/client_metrics_shard.hpp
/// @brief @copybrief server::request::SetClientMetricsShard

#include <optional>
#include <string_view>

#include <userver/server/request/impl/client_metrics_shard.hpp>
#include <userver/utils/statistics/labels.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request {

/// @brief Set the client metrics shard (path + labels) for requests
/// performed by the current task and its child tasks.
///
/// When set, supporting clients additionally accumulate their metrics on a new
/// subpath with the provided labels. It is a client-side counterpart of
/// @ref server::request::RequestContext::SetHandlerMetricsShard.
///
/// The setting lives until the end of the current task and is inherited by
/// child tasks.
///
/// Supported by:
/// * HTTP client: `timings`, `errors` and `reply-statuses` destination metrics
///   are accumulated on `httpclient.<path>.*` with the `http_destination` label
///   and the provided labels. Requests without destination metrics (e.g. when
///   `destination-metrics-auto-max-size` of http-client-core is reached)
///   are not accounted in the shard.
///
/// @note Labels should be lexicographically sorted by name and must not
/// contain `http_destination` and `version`.
/// @see ClientMetricsShardScope
void SetClientMetricsShard(std::string_view path, utils::statistics::LabelsSpan labels);

/// @brief Remove the client metrics shard set by @ref SetClientMetricsShard
/// for requests performed by the current task and its child tasks.
void EraseClientMetricsShard() noexcept;

/// @brief Sets the client metrics shard (see @ref SetClientMetricsShard)
/// within its scope and restores the previous one on destruction.
class [[nodiscard]] ClientMetricsShardScope final {
public:
    ClientMetricsShardScope(std::string_view path, utils::statistics::LabelsSpan labels);

    ClientMetricsShardScope(ClientMetricsShardScope&&) = delete;
    ClientMetricsShardScope& operator=(ClientMetricsShardScope&&) = delete;
    ~ClientMetricsShardScope();

private:
    std::optional<impl::ClientMetricsShard> old_value_;
};

}  // namespace server::request

USERVER_NAMESPACE_END
