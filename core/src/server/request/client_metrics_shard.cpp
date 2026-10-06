#include <userver/server/request/client_metrics_shard.hpp>

#include <algorithm>

#include <userver/server/request/impl/client_metrics_shard.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request {

void SetClientMetricsShard(std::string_view path, utils::statistics::LabelsSpan labels) {
    UINVARIANT(
        std::ranges::is_sorted(labels, [](const auto& a, const auto& b) { return a.Name() < b.Name(); }),
        "Labels for sharded metrics should be lexicographically sorted"
    );

    impl::ClientMetricsShard shard{std::string{path}, {}};
    shard.labels.reserve(labels.size());
    for (const auto& label : labels) {
        UINVARIANT(
            label.Name() != "http_destination" && label.Name() != "version",
            "Labels 'http_destination' and 'version' are reserved for client metrics"
        );
        shard.labels.emplace_back(label);
    }

    impl::kClientMetricsShard.Set(std::move(shard));
}

void EraseClientMetricsShard() noexcept { impl::kClientMetricsShard.Erase(); }

ClientMetricsShardScope::ClientMetricsShardScope(std::string_view path, utils::statistics::LabelsSpan labels) {
    if (const auto* const old_value = impl::kClientMetricsShard.GetOptional()) {
        old_value_.emplace(*old_value);
    }
    SetClientMetricsShard(path, labels);
}

ClientMetricsShardScope::~ClientMetricsShardScope() {
    if (old_value_) {
        impl::kClientMetricsShard.Set(std::move(*old_value_));
    } else {
        EraseClientMetricsShard();
    }
}

}  // namespace server::request

USERVER_NAMESPACE_END
