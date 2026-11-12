#include <userver/utils/statistics/metrics_storage.hpp>

#include <cstddef>
#include <functional>
#include <optional>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>

#include <fmt/format.h>
#include <boost/functional/hash.hpp>

#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/static_registration.hpp>
#include <userver/utils/lazy_prvalue.hpp>
#include <userver/utils/resource_scopes.hpp>
#include <userver/utils/statistics/storage.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

namespace impl {

namespace {

struct MetricKeyHash final {
    std::size_t operator()(const MetricKey& key) const noexcept;
};

using MetricMap = std::unordered_map<MetricKey, std::unique_ptr<MetricWrapperBase>, MetricKeyHash>;

using MetricMetadataMap = std::unordered_map<MetricKey, MetricFactory, MetricKeyHash>;

MetricMetadataMap& GetRegisteredMetrics() {
    static MetricMetadataMap map;
    return map;
}

MetricMap InstantiateMetrics(const std::optional<std::vector<std::string>>& allowed_metric_paths) {
    utils::impl::AssertStaticRegistrationFinished();
    const auto& registered_metrics = GetRegisteredMetrics();

    std::optional<std::unordered_set<std::string>> metric_paths;
    if (allowed_metric_paths.has_value()) {
        metric_paths = std::unordered_set<std::string>(allowed_metric_paths->begin(), allowed_metric_paths->end());
    }

    MetricMap metrics;
    for (const auto& [key, factory] : registered_metrics) {
        if (metric_paths.has_value() && !metric_paths->count(key.path)) {
            continue;
        }

        metrics.emplace(key, factory());
    }
    return metrics;
}

std::size_t MetricKeyHash::operator()(const MetricKey& key) const noexcept {
    auto seed = std::hash<std::type_index>()(key.idx);
    boost::hash_combine(seed, std::hash<std::string>()(key.path));
    return seed;
}

}  // namespace

MetricWrapperBase::~MetricWrapperBase() = default;

void RegisterMetricInfo(const MetricKey& key, MetricFactory factory) {
    utils::impl::AssertStaticRegistrationAllowed("MetricKey registration");
    UASSERT(factory);
    auto [_, ok] = GetRegisteredMetrics().emplace(key, factory);
    UASSERT_MSG(ok, fmt::format("duplicate MetricTag with path '{}'", key.path));
}

}  // namespace impl

struct MetricsStorage::Impl final {
    explicit Impl(const std::optional<std::vector<std::string>>& allowed_metric_paths)
        : metrics(impl::InstantiateMetrics(allowed_metric_paths))
    {}

    impl::MetricMap metrics;
};

MetricsStorage::MetricsStorage()
    : MetricsStorage(std::nullopt)
{}

MetricsStorage::MetricsStorage(const std::optional<std::vector<std::string>>& allowed_metric_paths)
    : impl_(allowed_metric_paths)
{}

MetricsStorage::MetricsStorage(MetricsStorage&&) noexcept = default;

MetricsStorage& MetricsStorage::operator=(MetricsStorage&&) noexcept = default;

MetricsStorage::~MetricsStorage() = default;

impl::MetricWrapperBase& MetricsStorage::GetMetricWrapper(const impl::MetricKey& key) {
    return *impl_->metrics.at(key);
}

void DumpMetric(Writer& writer, const MetricsStorage& storage) {
    for (const auto& [key, metric_ptr] : storage.impl_->metrics) {
        auto& metric = *metric_ptr;
        try {
            std::optional<Writer> metric_writer;
            if (!key.path.empty()) {
                metric_writer.emplace(utils::LazyPrvalue([&] { return writer[key.path]; }));
            }
            Writer& metric_path_writer = metric_writer ? *metric_writer : writer;
            if (metric_path_writer) {
                metric.DumpToWriter(metric_path_writer);
            }
        } catch (const std::exception& e) {
            UASSERT_MSG(false, fmt::format("Failed to write metrics for path '{}': {}", key.path, e.what()));
            LOG_ERROR() << "Failed to write metrics for path '" << key.path << "': " << e;
        }
    }
}

void MetricsStorage::ResetMetrics() {
    LOG_DEBUG() << "resetting custom metric";

    for (auto& [_, value] : impl_->metrics) {
        value->Reset();
    }
}

namespace {

struct MetricsStorageHolder final {
    MetricsStorageHolder(
        utils::ResourceScopeStorage& scopes,
        Storage& storage,
        const std::optional<std::vector<std::string>>& allowed_metric_paths
    )
        : metrics_storage(allowed_metric_paths)
    {
        storage.RegisterWriter(scopes, "", [&metrics = metrics_storage](Writer& writer) { writer = metrics; });
    }

    MetricsStorage metrics_storage;
};

}  // namespace

MetricsStoragePtr MakeBoundMetricsStorage(
    Storage& storage,
    const std::optional<std::vector<std::string>>& allowed_metric_paths
) {
    auto holder = utils::MakeWithResourceScopes<MetricsStorageHolder>(storage, allowed_metric_paths);
    MetricsStorage* metrics = &holder->metrics_storage;
    return MetricsStoragePtr(std::move(holder), metrics);
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
